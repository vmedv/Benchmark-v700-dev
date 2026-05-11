#!/usr/bin/env bash

v_nm_local() {
  mkfifo ctl ack
  bash -lc 'exec {ctl_fd}<>ctl; exec {ack_fd}<>ack; PERF_CTL_FD=$ctl_fd PERF_ACK_FD=$ack_fd \
    perf stat -j -o stats-local.json --control fd:${ctl_fd},${ack_fd} -D -1 \
      -e instructions,cache-references,cache-misses,branches,branch-misses \
      -- numactl -i 0 env LD_PRELOAD=./lib/libmimalloc.so PERF_CTL_FD=$ctl_fd PERF_ACK_FD=$ack_fd \
      build/payload_tree.debra -json-file thesis/numa/local.json -result-file res-local.json; status=$?; \
      exec {ack_fd}>&-; exec {ctl_fd}>&-; exit $status'
  rm ctl ack
}

v_nm_localloc() {
  mkfifo ctl ack
  bash -lc 'exec {ctl_fd}<>ctl; exec {ack_fd}<>ack; PERF_CTL_FD=$ctl_fd PERF_ACK_FD=$ack_fd \
    perf stat -j -o stats-localloc.json --control fd:${ctl_fd},${ack_fd} -D -1 \
      -e instructions,cache-references,cache-misses,branches,branch-misses \
      -- numactl -i 0 env LD_PRELOAD=./lib/libmimalloc.so PERF_CTL_FD=$ctl_fd PERF_ACK_FD=$ack_fd \
      build/payload_tree.debra -json-file thesis/numa/dist.json -result-file res-localloc.json; status=$?; \
      exec {ack_fd}>&-; exec {ctl_fd}>&-; exit $status'
  rm ctl ack
}

v_nm_dist() {
  mkfifo ctl ack
  bash -lc 'exec {ctl_fd}<>ctl; exec {ack_fd}<>ack; PERF_CTL_FD=$ctl_fd PERF_ACK_FD=$ack_fd \
    perf stat -j -o stats-dist.json --control fd:${ctl_fd},${ack_fd} -D -1 \
      -e instructions,cache-references,cache-misses,branches,branch-misses \
      -- numactl -i 0,1 env LD_PRELOAD=./lib/libmimalloc.so PERF_CTL_FD=$ctl_fd PERF_ACK_FD=$ack_fd \
      build/payload_tree.debra -json-file thesis/numa/dist.json -result-file res-dist.json; status=$?; \
      exec {ack_fd}>&-; exec {ctl_fd}>&-; exit $status'
  rm ctl ack
}
