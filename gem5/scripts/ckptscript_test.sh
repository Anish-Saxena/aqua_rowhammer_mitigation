#!/bin/bash

############ CHECKPOINT CONFIGURATION #############
# (Modify as needed)
if [ $# -gt 2 ]; then
    BENCHMARK=$1  #select benchmark
    NUM_CORES=$2 #select number of cores
    SPEC_VERSION=$3 # select spec version as 2017
else
    echo "Your command line contains <2 arguments"
    exit   
fi

#RUN CONFIG
CHECKPOINT_CONFIG="multiprogram_16GBmem_1Mn"
INST_TAKE_CHECKPOINT=1000000

MAX_INSTS=$((INST_TAKE_CHECKPOINT + 1)) #simulate till checkpoint instruction

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


################## DIRECTORY NAMES (CHECKPOINT, OUTPUT, RUN DIRECTORY)  ###################
#Set up based on path variables & configuration

# Ckpt Dir
CKPT_OUT_DIR=$CKPT_PATH/${CHECKPOINT_CONFIG}.SPEC${SPEC_VERSION}.C${NUM_CORES}/$BENCHMARK-1-ref-x86
echo "checkpoint directory: " $CKPT_OUT_DIR
mkdir -p $CKPT_OUT_DIR

# Output Dir
OUTPUT_DIR=$CKPT_PATH/output/${CHECKPOINT_CONFIG}.SPEC${SPEC_VERSION}.C${NUM_CORES}/checkpoint_out/$BENCHMARK
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
echo "================= Hardcoded directories ==================" | tee -a $SCRIPT_OUT
echo "GEM5_PATH:                                     $GEM5_PATH" | tee -a $SCRIPT_OUT
echo "SPEC17_PATH:                                     $SPEC17_PATH" | tee -a $SCRIPT_OUT
echo "==================== Script inputs =======================" | tee -a $SCRIPT_OUT
echo "BENCHMARK:                                    $BENCHMARK" | tee -a $SCRIPT_OUT
echo "OUTPUT_DIR:                                   $OUTPUT_DIR" | tee -a $SCRIPT_OUT
echo "==========================================================" | tee -a $SCRIPT_OUT
##################################################################


#################### LAUNCH GEM5 SIMULATION ######################
echo ""

echo "" | tee -a $SCRIPT_OUT
echo "" | tee -a $SCRIPT_OUT
echo "--------- Here goes nothing! Starting gem5! ------------" | tee -a $SCRIPT_OUT
echo "" | tee -a $SCRIPT_OUT
echo "" | tee -a $SCRIPT_OUT

# Launch Gem5:
$GEM5_PATH/build/X86/gem5.opt \
    --outdir=$OUTPUT_DIR \
    $GEM5_PATH/configs/example/se_rq_spec_config_multicore.py \
    --redirects /lib64=/home/gattaca4/gururaj/LOCAL_LIB/python/anaconda3/lib \
    --benchmark=$BENCHMARK \
    --benchmark_stdout=$OUTPUT_DIR/$BENCHMARK.out \
    --benchmark_stderr=$OUTPUT_DIR/$BENCHMARK.err \
    --spec-version=$SPEC_VERSION \
    --num-cpus=$NUM_CORES --mem-size=16GB --mem-type=DDR4_2400_16x4 --mem-ranks=1 \
    --checkpoint-dir=$CKPT_OUT_DIR \
    --take-checkpoint=$INST_TAKE_CHECKPOINT --at-instruction \
    --maxinsts=$MAX_INSTS \
    --prog-interval=300Hz
    # >> $SCRIPT_OUT 2>&1 &

#     --mem-type=SimpleMemory \