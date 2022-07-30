cd ..
mkdir -p stats_scripts/data 

OUTPUT_DIR=$GEM5_PATH/stats/

### Normalized Performance based on Weighted-Speedup Metric ###
# SPEC-18 workloads
perl getdata.pl -n 0  -w spec17_single -ipc 4 -ws -b $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C1/O3.BASELINE.50Bn_FF.1C/   \
    -d  $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE.50Bn_FF.4C/ $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/$2/ \
    | sed 's/[_A-Z]*\///' | column -t -o ',' > stats_scripts/data/spec17_perf.stat ; 

# # MIX-16 workloads
# perl getdata.pl -n 0 -nh  -w spec17_mix -ipc 4 -ws -b $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C1/O3.BASELINE.50Bn_FF.1C/ \
#     -d  $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE.50Bn_FF.4C/ $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/$2/ \
#     | sed 's/[_A-Z]*\///' | column -t -o ',' >> stats_scripts/data/spec17_perf.stat;
# echo ".  0  0  0" | column -t >> stats_scripts/data/spec17_perf.stat;
 
# Avg - SPEC-18
perl getdata.pl -gmean -n 0 -nh -ns -gmean -w spec17_single -ipc 4 -ws -b $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C1/O3.BASELINE.50Bn_FF.1C/   \
    -d  $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE.50Bn_FF.4C/ $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/$2/  \
    | sed 's/Gmean/SPEC-18/' | column -t -o ',' >> stats_scripts/data/spec17_perf.stat;
    
# # Avg - MIX-16
# perl getdata.pl -gmean -n 0 -nh -ns -gmean -w spec17_mix -ipc 4 -ws -b $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C1/O3.BASELINE.50Bn_FF.1C/   \
#     -d  $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE.50Bn_FF.4C/ $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/$2/ \
#     | sed 's/Gmean/MIX-16/' | column -t -o ',' >> stats_scripts/data/spec17_perf.stat;

# # # Avg - ALL-34
# perl getdata.pl -gmean -n 0 -nh -ns -gmean -w spec17_all -ipc 4 -ws -b $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C1/O3.BASELINE.50Bn_FF.1C/   \
#     -d  $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE.50Bn_FF.4C/ $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/$2/  \
#     | sed 's/Gmean/ALL-34/' | column -t -o ',' >> stats_scripts/data/spec17_perf.stat;
# Format
cat stats_scripts/data/spec17_perf.stat

### Uncomment for Normalized Performance based on Raw-IPC Metric ### 
# perl getdata.pl -n 0 -gmean -w spec17_all -ipc 4  -d  $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE.50Bn_FF/   $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE.50Bn_FF/scatter-cache \
# $OUTPUT_DIR/multiprogram_16GBmem_$1.SPEC2017.C4/O3.BASELINE.50Bn_FF/skew-vway-rand  | sed 's/[_A-Z]*\///' | sed 's/[_A-Z]*\/scatter-cache/scatter-cache/' | sed 's/[_A-Z]*\/skew-vway-rand/MIRAGE/' | column -t ;
