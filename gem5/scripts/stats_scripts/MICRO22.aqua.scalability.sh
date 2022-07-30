cd ..
mkdir -p stats_scripts/data 

## Normalized Performance based on Weighted-Speedup Metric ###

# 18 SPEC workloads
perl getdata.pl -n 0  -w spec17_single -ipc 4 -ws -printmask 0-1-1-1  -b ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C1/AE.BASELINE.1C \
    -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.BASELINE.4C \
    ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.2K.4C \
    ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.500.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.2K.4C/2K/' \
    | sed  's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C/1K/' \
    | sed  's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.500.4C/500/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/aqua_scalability.stat ; 

# # # MIX-16 workloads
perl getdata.pl -n 0 -nh  -w spec17_mix -ipc 4 -ws -printmask 0-1-1-1  -b ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C1/AE.BASELINE.1C \
    -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.BASELINE.4C \
    ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.2K.4C \
    ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.500.4C \ 
    | sed 's/[_A-Z]*\///' | column -t  >> stats_scripts/data/aqua_scalability.stat ; 

echo ".  0  0  0" | column -t >> stats_scripts/data/aqua_scalability.stat ; 

# # Avg - ALL-34
perl getdata.pl -gmean -n 0 -nh -ns -gmean -w spec17_all  -ipc 4 -ws -printmask 0-1-1-1  -b ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C1/AE.BASELINE.1C \
    -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.BASELINE.4C \
    ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.2K.4C \
    ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.500.4C \ 
    | sed 's/Gmean/Gmean-34/' | column -t >> stats_scripts/data/aqua_scalability.stat;

# Format
cat stats_scripts/data/aqua_scalability.stat


### Uncomment for Normalized Performance based on Raw-IPC Metric ### 
# perl getdata.pl -n 0 -gmean -w spec17_all -ipc 4  -d  $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE/   $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE/scatter-cache \
# $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE/skew-vway-rand  | sed 's/[_A-Z]*\///' | sed 's/[_A-Z]*\/scatter-cache/scatter-cache/' | sed 's/[_A-Z]*\/skew-vway-rand/MIRAGE/' | column -t ;
