#!/bin/bash

######----------------------------------------------------- ######
######------------- CHECKPOINTS FOR 4/1-CORE EXPERIMENTS -- ######
######----------------------------------------------------- ######

# We left out cam4, xalancbmk, fotonik3d, omnetpp, and x264
###### 1-Core SPEC2017 Experiments #######
echo "Creating 1-Core Checkpoints for 18 Benchmarks"
for bmk in perlbench gcc bwaves mcf cactuBSSN namd povray lbm wrf\
  blender deepsjeng imagick leela nab exchange2 roms xz parest; do 
    ./ckptscript.sh $bmk 1 2017; 
done

# ####### 4-Core SPEC2017 Experiments #######
echo "Creating 4-Core Checkpoints for 34 Benchmarks"
for bmk in perlbench gcc bwaves mcf cactuBSSN namd povray lbm wrf\
 x264 blender deepsjeng imagick leela nab exchange2 roms xz parest\
 mix1 mix2 mix3 mix4 mix5 mix6 mix7 mix8 mix9 mix10\
 mix11 mix12 mix13 mix14 mix15 mix16; do 
    ./ckptscript.sh $bmk 4 2017; 
done

wait 