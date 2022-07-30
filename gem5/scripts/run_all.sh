#!/bin/bash

./create_checkpoints.sh;
./run_baseline_experiments.sh &
./run_aqua_experiments.sh &
./run_rrs_experiments.sh