
<!-- README.md is generated from README.Rmd. Please edit that file -->

# rllama

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
<!-- badges: end -->

`rllama` wraps [`llama.cpp`](https://github.com/ggerganov/llama.cpp) - a
Large Language Model (LLM) running on the CPU. ([Reference
paper](https://arxiv.org/abs/2302.13971))

This is a minimally-viable-product which accepts input and produces
output, but the quality, interface, capabilities and configurability are
all very (very!) basic.

## `whisper.cpp`

- Code snapshot:
  - 25 May 2023
  - master-66874d4

# Future (contributions welcomed)

- Better penalty system for avoiding recent tokens.
- Cross-platform compilation settings (optimisation flags etc)
- Implement context memory across calls to multiple calls to `llama()`
  with the same context.

## Installation

You can install from [GitHub](https://github.com/coolbutuseless/rllama)
with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/rllama')
```

There only dependencies are:

- being able to compile R packages on your system.
- downloading a model.

### Downloading a model

To get started, I suggest grabbing the Vicuna model
`ggml-vic7b-q5_0.bin` from
[here](https://huggingface.co/eachadea/ggml-vicuna-7b-1.1/tree/main).

This is a small model (\~5GB) with 7Billion parameters quantized to
5bits per parameter.

Any other model supported by `llama.cpp` should work. Check out the list
of supported models on the [`llama.cpp` github
page](https://github.com/ggerganov/llama.cpp).

### Model versions

**Note:** This package uses the `llama.cpp` code from about the 25 May
2023.

The quantization formats (e.g. Q4, Q5 and Q8) have all changed within
the last month.  
Any older model files you have will probably **not** work with the
latest `llama.cpp`. You’ll either have to requantize your models, or
just download one in the appropriate format (e.g. from
[here](https://huggingface.co/eachadea/ggml-vicuna-7b-1.1/tree/main)).

### Platform notes:

This package has only been tested on macOS so please let me know of any
issues. PRs welcomed.

## Using `rllama`

``` r
library(rllama)

# Initialise llama.cpp with built-in model
ctx <- llama_init("/Users/mike/projectsdata/llama.cpp/ggml-vic7b-q5_0.bin")
llama(ctx, prompt = "The apple said to the banana", n = 400)
#> , "You're not as smart as I am."
#> The banana replied, "That's okay. I'm just a fruit and you're a computer program. What do you expect?"
#> The apple said, "I can do things like tell jokes and play games that you can't."
#> The banana said, "Well, I can be used for making smoothies and baking cakes."
#> The apple said, "That may be true but at least I have some intelligence."
```

## Licenses

- This R package is MIT licensed. See file: LICENSE
- The included [`llama.cpp`](https://github.com/ggerganov/whisper.cpp)
  code is MIT licensed. See file `LICENSE-llama.cpp.txt`

## Acknowledgements

- R Core for developing and maintaining the language.
- CRAN maintainers, for patiently shepherding packages onto CRAN and
  maintaining the repository
