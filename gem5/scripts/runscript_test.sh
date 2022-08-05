#!/bin/bash

############ NOTE: This script sets up the following variables and then runs the command below ############

# To be modified as required
if [ $# -gt 3 ]; then
    BENCHMARK=$1  #select benchmark
    RUN_CONFIG=$2 #specify output folder name
    NUM_CORES=$3 #select number of cores
    SPEC_VERSION=$4 # select spec version as 2017
    RH_DEFENSE=$5
else
    echo "Your command line contains <2 arguments"
    exit
fi

#RUN CONFIG
MAX_INSTS=1000000                      # Number of instructions to be simulated
STATS_CONFIG="multiprogram_16GBmem_1Mn"
INST_TAKE_CHECKPOINT=1000000           # Instruction count after which checkpoint was taken
CHECKPOINT_CONFIG="multiprogram_16GBmem_1Mn"    # Name of directory inside CKPT_PATH

############ DIRECTORY PATHS TO BE EXPORTED #############

#Need to export GEM5_PATH
if [ -z ${GEM5_PATH+x} ];
then
    echo "GEM5_PATH is unset";
    exit
else
    echo "GEM5_PATH is set to '$GEM5_PATH'";
fi
#Need to export SPEC17_PATH
if [ -z ${SPEC17_PATH+x} ];
then
    echo "SPEC17_PATH is unset";
    exit
else
    echo "SPEC17_PATH is set to '$SPEC17_PATH'";
fi
#Need to export CKPT_PATH
if [ -z ${CKPT_PATH+x} ];
then
    echo "CKPT_PATH is unset";
    exit
else
    echo "CKPT_PATH is set to '$CKPT_PATH'";
fi
if [ -z "${RH_DEFENSE}" ];
then
    echo "RH_DEFENSE is DISABLED";
else
    echo "RH_DEFENSE: ${RH_DEFENSE} is ENABLED";
    RH_DEFENSE_PARAMS="${@:5}";
fi
################## DIRECTORY NAMES (CHECKPOINT, OUTPUT, RUN DIRECTORY)  ###################
#Set up based on path variables & configuration

# Ckpt Dir
CKPT_OUT_DIR=$CKPT_PATH/${CHECKPOINT_CONFIG}.SPEC${SPEC_VERSION}.C${NUM_CORES}/$BENCHMARK-1-ref-x86
echo "checkpoint directory: " $CKPT_OUT_DIR

# Output Dir
OUTPUT_DIR=$GEM5_PATH/stats/${STATS_CONFIG}.SPEC${SPEC_VERSION}.C${NUM_CORES}/$RUN_CONFIG/${SCHEME}/$BENCHMARK
echo "output directory: " $OUTPUT_DIR
if [ -d "$OUTPUT_DIR" ]
then
    rm -r $OUTPUT_DIR
fi
mkdir -p $OUTPUT_DIR

# File log used for stdout
SCRIPT_OUT=$OUTPUT_DIR/runscript.log

#Report directory names 
echo "Command line:"                                | tee $SCRIPT_OUT
echo "$0 $*"                                        | tee -a $SCRIPT_OUT

# Launch Gem5:
$GEM5_PATH/build/X86/gem5.opt \
    --outdir=$OUTPUT_DIR \
    $GEM5_PATH/configs/example/se_rq_spec_config_multicore.py \
    --benchmark=$BENCHMARK \
    --benchmark_stdout=$OUTPUT_DIR/$BENCHMARK.out \
    --benchmark_stderr=$OUTPUT_DIR/$BENCHMARK.err \
    --spec-version=$SPEC_VERSION ${RH_DEFENSE_PARAMS} \
    --num-cpus=$NUM_CORES --mem-size=16GB --mem-type=DDR4_2400_16x4 --mem-ranks=1 \
    --checkpoint-dir=$CKPT_OUT_DIR \
    --checkpoint-restore=$INST_TAKE_CHECKPOINT --at-instruction \
    --cpu-type DerivO3CPU \
    --caches --l2cache \
    --l1d_size=32kB --l1i_size=32kB --l2_size=256kB \
    --l1d_assoc=8  --l1i_assoc=8 --l2_assoc=16 \
    --maxinsts=$MAX_INSTS \
    --prog-interval=300Hz \
    >> $SCRIPT_OUT 2>&1 &

    # --debug-flags=RH_DEFENSE,DRAM,MemCtrl \
    # DerivO3CPU