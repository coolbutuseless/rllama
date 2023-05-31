
// #define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

extern SEXP llama_init_(SEXP path_);
extern SEXP llama_(SEXP rdata_, SEXP prompt_);

static const R_CallMethodDef CEntries[] = {
  {"llama_init_"   , (DL_FUNC) &llama_init_   , 2},
  {"llama_"        , (DL_FUNC) &llama_        , 7},
  {NULL , NULL, 0}
};


void R_init_rllama(DllInfo *info) {
  R_registerRoutines(
    info,      // DllInfo
    NULL,      // .C
    CEntries,  // .Call
    NULL,      // Fortran
    NULL       // External
  );
  R_useDynamicSymbols(info, FALSE);
}
