
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

# Future (contributions welcomed)

- Better penalty system for avoiding recent tokens.
- Cross-platform compilation settings (optimisation flags etc)

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
of supported models on the \[`llama.cpp` github
page\]([`llama.cpp`](https://github.com/ggerganov/llama.cpp)

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
