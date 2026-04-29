#!/usr/bin/env bash

rm ctl ack
mkfifo ctl ack

perf stat --control fifo:ctl \
  -e instructions,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses  \
  -- build/payload_tree.debra -json-file thesis/diffsizes/one.json
