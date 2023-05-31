


#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "llama.h"


int N_THREADS = 4;
int N_TOK_PREDICT = 30;
int TOP_K = 40;
double TOP_P = 0.9f;
double TEMP = 0.4f;
double REPEAT_PENALTY = 1.2f;
int BATCH_SIZE = 8;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Information neede dto be retained by R between calls.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  struct llama_context *ctx;     // the actual llama.cpp context
  llama_token_data *candidates;  // Allocate this once and keep.
  unsigned int n_vocab;          // length of 'candidates'.  A property of the model.
} rctx;



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack an external pointer to a C 'pa_stream_struct *'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
rctx * external_ptr_to_rllama(SEXP rdata_) {
    if (!inherits(rdata_, "rctx")) {
        error("Expecting 'rdata' to be an 'rctx' ExternalPtr");
    }
    
    rctx *rdata = TYPEOF(rdata_) != EXTPTRSXP ? NULL : (rctx *)R_ExternalPtrAddr(rdata_);
    if (rdata == NULL) {
      error("rctx pointer is invalid/NULL.");
    }
    
    return rdata;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Finalizer for an 'rctx struct' object.
//
// This function will be called when portaudio stream object gets 
// garbage collected.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void rllama_finalizer(SEXP rdata_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  rctx *rdata = (rctx *)R_ExternalPtrAddr(rdata_);
  
  free(rdata->candidates);
  
  struct llama_context *ctx = rdata->ctx;
  if (ctx == 0) {
    Rprintf("NULL llama_context in finalizer");
  } else {
    llama_free(ctx);
  }

  R_ClearExternalPtr(rdata_);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse the model file and create the llama context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP llama_init_(SEXP model_path_, SEXP user_params_) {

  const char *model_path = CHAR(STRING_ELT(model_path_, 0));

  rctx *rdata;
  rdata = (rctx *)calloc(1, sizeof(rctx));
  
  llama_init_backend();
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise a set of parameters and overwrite with the user specified params
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  struct llama_context_params lparams = llama_context_default_params();
  lparams.n_ctx        = asInteger(VECTOR_ELT(user_params_, 0));
  lparams.n_gpu_layers = asInteger(VECTOR_ELT(user_params_, 1));
  lparams.seed         = asInteger(VECTOR_ELT(user_params_, 2));
  lparams.f16_kv       = asLogical(VECTOR_ELT(user_params_, 3));
  lparams.logits_all   = asLogical(VECTOR_ELT(user_params_, 4));
  lparams.vocab_only   = asLogical(VECTOR_ELT(user_params_, 5));
  lparams.use_mmap     = asLogical(VECTOR_ELT(user_params_, 6));
  lparams.use_mlock    = asLogical(VECTOR_ELT(user_params_, 7));
  lparams.embedding    = asLogical(VECTOR_ELT(user_params_, 8));
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise model from file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  struct llama_context *ctx = llama_init_from_file(model_path, lparams);
  
  if (ctx == NULL) {
    error("Couldn't initialize model / model load");
  }
  rdata->ctx = ctx;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise 'candidate' structure for tracking probabilities for each 
  // vocab entry in the model
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  rdata->n_vocab = llama_n_vocab(ctx);
  rdata->candidates = calloc(rdata->n_vocab, sizeof(llama_token_data));
  for (int i = 0; i < rdata->n_vocab; i++) {
    rdata->candidates[i].id = i;  // llama_token id
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Wrap the pointer to the struct as an ExternalPointer to give back to R
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP rdata_ = R_MakeExternalPtr(rdata, R_NilValue, R_NilValue);
  PROTECT(rdata_);
  R_RegisterCFinalizer(rdata_, rllama_finalizer);
  Rf_setAttrib(rdata_, R_ClassSymbol, Rf_mkString("rctx"));
  UNPROTECT(1);

  return rdata_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Run llama on the prompt
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP llama_(SEXP rdata_, SEXP prompt_, SEXP n_, SEXP repeat_penalty_, SEXP greedy_, SEXP temp_, 
            SEXP verbose_) {

  // hardcoded due to laziness on my part
  llama_token tokens[10000];
  char response[10000] = "";
  
  const char *prompt = CHAR(STRING_ELT(prompt_, 0));
  unsigned int n = asInteger(n_);
  if (n > 2000 || n < 1) {
    error("Imporbable 'n' requested: %i", n);
  }
  
  rctx *rdata = external_ptr_to_rllama(rdata_);
  struct llama_context *ctx = rdata->ctx;
  if (ctx == NULL) {
    error("Couldn't unpack a llama_context * from the rctx struct");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Convert the provided text into tokens.
  //
  // The tokens pointer must be large enough to hold the resulting tokens.
  // Returns the number of tokens on success, no more than n_max_tokens
  // Returns a negative number on failure - the number of tokens that would have been returned
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int num_input_tokens = llama_tokenize(
    ctx,     // struct llama_context * ctx
    prompt,  // const char * text
    tokens,  // llama_token * tokens
    512,     // int   n_max_tokens
    true     // bool   add_bos
  );
  
  if (num_input_tokens <= 0) {
    error("Error when tokenizing input: %i\n", num_input_tokens);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Feed the input tokens into the model
  //
  // TODO: Look into batching these rather than one-at-a-time.
  //       I.e. current code adds tokens one at a time. probably really slow?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i=0; i < num_input_tokens; i++) {
    llama_eval(
      ctx,         // llama context
      tokens + i,  // Pointer to this latest token 
      1,           // number of tokens to consume - starting at the above tokens pointer
      i,           // n_past: number of tokens to use from previous eval calls
      N_THREADS    // Number of threads
    );
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise 'candidate' structure for tracking probabilities for each 
  // vocab entry in the model
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned int output_idx;
  for (output_idx = 0; output_idx < n; output_idx++) {
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Set up the candidate probabilities 
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    float *logits = llama_get_logits(ctx);
    
    for (int i = 0; i < rdata->n_vocab; i++) {
      rdata->candidates[i].id    = i;           // do every time as it gets sorted by some sampling techniques
      rdata->candidates[i].logit = logits[i];   // float log-odds of the token
      rdata->candidates[i].p     = 0;           // probability of token. 
    }
    
    llama_token_data_array candidates_p = {
      rdata->candidates,  // All candidates
      rdata->n_vocab,     // Length of candidates 
      false               // Sorted?
    };
    
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Apply penalties to tokens to reduce the chance they are selected again
    //  * occurred within a recent window
    //  * occurred anywhere in the returned tokens
    // TODO: this code is pretty rough. Idea taken from llama.cpp/main.cpp
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    for (int ti = 0; ti <  num_input_tokens + output_idx; ti++) {
      float penalty = asReal(repeat_penalty_);
      llama_token i = tokens[ti];
      if (rdata->candidates[i].logit <= 0) {
        rdata->candidates[i].logit *= penalty;
      } else {
        rdata->candidates[i].logit /= penalty;
      }
    }
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Somehow pick the next token
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    llama_token id;
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Be greedy
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (asLogical(greedy_)) {
      id = llama_sample_token_greedy(ctx, &candidates_p);
    } else {
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      // Pick token based upon temperature
      // code from: llama.cpp/main.cpp
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      const float   temp            = asReal(temp_);
      const int32_t top_k           = 40;
      const float   top_p           = 0.95;
      const float   tfs_z           = 1.00;
      const float   typical_p       = 1.00;
      
      llama_sample_top_k      (ctx, &candidates_p, top_k, 1);
      llama_sample_tail_free  (ctx, &candidates_p, tfs_z, 1);
      llama_sample_typical    (ctx, &candidates_p, typical_p, 1);
      llama_sample_top_p      (ctx, &candidates_p, top_p, 1);
      llama_sample_temperature(ctx, &candidates_p, temp);
      id = llama_sample_token (ctx, &candidates_p);
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // End of stream
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (id == llama_token_eos()) {
      if (asLogical(verbose_)) {
        Rprintf("\n\n");
      }
      break;
    } 
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Token output, and add to to string to return to user
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    const char *str = llama_token_to_str(ctx, id);
    if (asLogical(verbose_)) {
      Rprintf("%s", str);
    }
    strncat(response, str, strlen(str));
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Add new token to token history
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    tokens[num_input_tokens + output_idx] = id;
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Update step: Evaluate this selected token in the current model context
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~    
    // int n_past = rdata->n_tokens;
    int n_past = num_input_tokens + output_idx;
    n_past = n_past > 500 ? 500 : n_past;
    
    // Rprintf("llama_eval of input tokens with n_past = %i\n", n_past);
    llama_eval(
      ctx,      // llama context
      tokens + num_input_tokens + output_idx,  // Pointer to this latest token
      1,        // number of tokens to consume - starting at the above tokens pointer
      n_past,   // n_past: number of tokens to use from previous eval calls
      N_THREADS // Number of threads
    );
    
    
  }

  return mkString(response);
}






