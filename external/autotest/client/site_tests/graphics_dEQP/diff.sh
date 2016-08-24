#!/bin/bash

gpus=(baytrail broadwell haswell ivybridge sandybridge)

for gpu in ${gpus[*]}
do
  rm expectations/${gpu}/*.json
  cat expectations/${gpu}/* | sort > /tmp/${gpu}.sorted
  cat expectations/${gpu}/* | sort | uniq > /tmp/${gpu}.sorted_uniq
  diff /tmp/${gpu}.sorted /tmp/${gpu}.sorted_uniq > ${gpu}.diff
done

