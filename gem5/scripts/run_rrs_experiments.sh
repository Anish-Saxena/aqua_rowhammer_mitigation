#!/bin/bash

######----------------------------------------------------- ######
######------------- RUN FOR 4/1-CORE EXPERIMENTS ---------- ######
######----------------------------------------------------- ######

for bmk in perlbench gcc bwaves mcf cactuBSSN namd povray lbm wrf\
 x264 blender deepsjeng imagick leela nab exchange2 roms xz parest\
 mix1 mix2 mix3 mix4 mix5 mix6 mix7 mix8 mix9 mix10\
 mix11 mix12 mix13 mix14 mix15 mix16; do 
    #************* RRS base config BEGIN *************#
    # RRS for TRH=1K
    ./runscript.sh $bmk AE.RRS.1K.4C 4 2017 \
     --rh_defense --rh_mitigation=RRS --rh_actual_threshold=1000;
    #************* RRS base config END *************#

    #************* RRS scalability configs BEGIN *************#
    # RRS for TRH=2K
    ./runscript.sh $bmk AE.RRS.2K.4C 4 2017 \
     --rh_defense --rh_mitigation=RRS --rh_actual_threshold=2000;
    # RRS for TRH=4K
    ./runscript.sh $bmk AE.RRS.4K.4C 4 2017 \
     --rh_defense --rh_mitigation=RRS --rh_actual_threshold=4000;
    #************* RRS scalability configs END *************#

    ## Wait for a core to be available
    exp_count=`ps aux | grep -i "gem5" | grep -v "grep" | wc -l`
    while [ $exp_count -gt ${MAX_GEM5_PARALLEL_RUNS} ]
    do
        sleep 300
        exp_count=`ps aux | grep -i "gem5" | grep -v "grep" | wc -l`
        echo
    done
done

exp_count=`ps aux | grep -i "gem5" | grep -v "grep" | wc -l`
while [ $exp_count -gt 0 ]
do
    sleep 300
    exp_count=`ps aux | grep -i "gem5" | grep -v "grep" | wc -l`    
done