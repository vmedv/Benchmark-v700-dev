#!/usr/bin/env bash

[ -d can ] && rm -rf can/*
mkdir can

for i in {1..26}; do
  let SIZE=2**${i}
  echo "HEIGHT: $i, SIZE: $SIZE"
  jq \
    --argjson size $SIZE \
    '.prefill.stopCondition.commonOperationLimit = $size | .range = $size' \
    thesis/diffsizes/balanced.json > "can/config-${SIZE}.json"
  $(pwd)/thesis/diffsizes/run.sh $SIZE > /dev/null
done
