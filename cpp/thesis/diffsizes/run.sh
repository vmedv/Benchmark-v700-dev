#!/usr/bin/env bash

perf mem -t load record -c50 \
  # -e cache-misses,cache-references,ref-cycles                       \
  build/payload_tree.debra                                          \
  -json-file thesis/diffsizes/one.json
  # -result-json thesis/diffsizes/one.res.json
