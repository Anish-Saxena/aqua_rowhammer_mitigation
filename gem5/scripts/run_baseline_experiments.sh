#!/bin/bash

######----------------------------------------------------- ######
######------------- RUN FOR 4/1-CORE EXPERIMENTS ---------- ######
######----------------------------------------------------- ######


# baseline single-core SPEC17 workloads
for bmk in perlbench gcc bwaves mcf cactuBSSN namd povray lbm wrf\
  blender deepsjeng imagick leela nab exchange2 roms xz parest; do 
    ./runscript.sh $bmk AE.BASELINE.1C 1 2017; 
done

# baseline multi-core SPEC17 workloads
for bmk in perlbench gcc bwaves mcf cactuBSSN namd povray lbm wrf\
 x264 blender deepsjeng imagick leela nab exchange2 roms xz parest\
 mix1 mix2 mix3 mix4 mix5 mix6 mix7 mix8 mix9 mix10\
 mix11 mix12 mix13 mix14 mix15 mix16; do 
    ./runscript.sh $bmk AE.BASELINE.4C 4 2017; 
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