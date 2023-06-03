
llama_params <- list(
  n_ctx        =   512,  # int n_ctx;        // text context
  n_gpu_layers =     0,  # int n_gpu_layers; // number of layers to store in VRAM
  seed         =    -1,  # int seed;         // RNG seed, -1 for random
  f16_kv       =  TRUE,  # bool f16_kv;      // use fp16 for KV cache
  logits_all   = FALSE,  # bool logits_all;  // the llama_eval() call computes all logits, not just the last one
  vocab_only   = FALSE,  # bool vocab_only;  // only load the vocabulary, no weights
  use_mmap     =  TRUE,  # bool use_mmap;    // use mmap if possible
  use_mlock    = FALSE,  # bool use_mlock;   // force system to keep model in RAM
  embedding    = FALSE   # bool embedding;   // embedding mode only
)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Fetch a copy of the default model parameters 
#' 
#' \describe{
#'   \item{n_ctx       = 512   }{text context}
#'   \item{gpu_layers  = 0     }{number of layers to store in VRAM}
#'   \item{seed        = -1    }{RNG seed, -1 for random}
#'   \item{f16_kv      = true  }{use fp16 for KV cache}
#'   \item{logits_all  = false }{the llama_eval() call computes all logits, not just the last one}
#'   \item{vocab_only  = false }{only load the vocabulary, no weights}
#'   \item{use_mmap    = true  }{use mmap if possible}
#'   \item{use_mlock   = false }{force system to keep model in RAM}
#'   \item{embedding   = false }{embedding mode only}
#' }
#' @return named list of values
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
llama_default_params <- function() {
  llama_params
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise llama.cpp by loading a model
#' 
#' @param model_path path to llama.cpp model. 
#' @param params parameters for the execution of llama.  See \code{llama_default_params()}
#'        for details.  Note: You possibly don't want/need to play with any of 
#'        these unless you really know what you're doing.
#' 
#' @return llama context (\code{ctx})
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
llama_init <- function(model_path, params = list()) {
  user_params <- modifyList(llama_params, params, keep.null = TRUE)
  .Call(llama_init_, model_path, user_params)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Ask llama to respond to the given prompt
#' 
#' @param prompt character string
#' @param ctx llama context (created using \code{llama_init()})
#' @param n maximum number of tokens to emit.
#' @param repeat_penalty penalty applied to tokens to avoid repetition. 
#'        Default: 1.05.   Use \code{1} for no penalty, and higher numbers to
#'        avoid repetition more.
#' @param greedy logical. Simply take the highest probability token as the 
#'        next token. default: TRUE.    If FALSE, then use a temperature 
#'        based selection technique.
#' @param temp default: 0.8.  Controls the probabilistic(?) non-greedy selection
#'        of the next token.  Only used if \code{greedy = FALSE}
#' @param verbose logical. Output tokens as they are generated. Default: TRUE
#' 
#' @return Invisibly returns the entire response as a single string
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
llama <- function(ctx, prompt, n = 50, repeat_penalty = 1.05, greedy = TRUE, temp=0.8, verbose = TRUE) {
  # there's some requirement to have a leading " " on the prompt.  Not
  # entirely sure why.  Something to do with the tokenizer? Mike 2023-05-31
  prompt <- paste0(" ", prompt)
  response <- .Call(llama_, ctx, prompt, n, as.numeric(repeat_penalty), isTRUE(greedy), as.numeric(temp), isTRUE(verbose))
  
  invisible(response)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# inline testing
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
if (FALSE) {
  ctx <- llama_init(model_path = "/Users/mike/projectsdata/llama.cpp/ggml-vic7b-q5_0.bin")
  llama(ctx, "Jenny Bryan saw someone use setwd() and then she", n = 500, greedy = FALSE)
  
  
  ctx <- llama_init(model_path = "/Users/mike/projectsdata/llama.cpp/ggml-vic7b-uncensored-q5_1.bin")
  llama(ctx, "Jenny Bryan saw someone use setwd() and then she", n = 500, greedy = FALSE)
  
  
  
  
  
  
  
  
}
