#!/usr/bin/env bash

rm ctl ack
mkfifo ctl ack

perf stat -j -o "can/res-${1}.json" --control fifo:ctl \
  -e instructions,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,l2_cache_req_stat.dc_hit_in_l2,l2_cache_req_stat.dc_access_in_l2  \
  -- build/payload_tree.debra -json-file "can/config-${1}.json" -result-file "can/ops-${1}.json"
