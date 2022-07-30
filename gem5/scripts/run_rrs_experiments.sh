#!/bin/bash

######----------------------------------------------------- ######
######------------- RUN FOR 4/1-CORE EXPERIMENTS ---------- ######
######----------------------------------------------------- ######

for bmk in perlbench gcc bwaves mcf cactuBSSN namd povray lbm wrf\
 x264 blender deepsjeng imagick leela nab exchange2 roms xz parest\
 mix1 mix2 mix3 mix4 mix5 mix6 mix7 mix8 mix9 mix10\
 mix11 mix12 mix13 mix14 mix15 mix16; do 
    #************* RRS scalability configs *************#

    # RRS for TRH=1K
    ./runscript.sh $bmk AE.RRS.1K.4C 4 2017 \
     --rh_defense --rh_mitigation=RRS --rh_actual_threshold=1000;

    # RRS for TRH=2K
    ./runscript.sh $bmk AE.RRS.2K.4C 4 2017 \
     --rh_defense --rh_mitigation=RRS --rh_actual_threshold=2000;

    # RRS for TRH=4K
    ./runscript.sh $bmk AE.RRS.4K.4C 4 2017 \
     --rh_defense --rh_mitigation=RRS --rh_actual_threshold=4000;

    ## Wait for a core to be available
    exp_count=`ps aux | grep -i "gem5" | grep -v "grep" | wc -l`
    while [ $exp_count -gt 68 ]
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