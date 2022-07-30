cd ..
mkdir -p stats_scripts/data 

OUTPUT_DIR=$GEM5_PATH/stats/

### Normalized Performance based on Weighted-Speedup Metric ###
# SPEC-14 workloads
perl getdata.pl -n 0  -w spec_14 -ipc 4 -ws -b $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C1/BASELINE.4MBLLC.1C/   \
    -d  $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/BASELINE.4MBLLC.4C/ $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/RRS.4MBLLC$11K.4C/ $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/RQ.4MBLLC.$2.1K.4C/ \
    | sed 's/[_A-Z]*\///' | column -t -o ',' > stats_scripts/data/perf.stat ; 

# MIX-14 workloads
perl getdata.pl -n 0 -nh  -w mix_14 -ipc 4 -ws -b $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C1/BASELINE.4MBLLC.1C/ \
    -d  $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/BASELINE.4MBLLC.4C/ $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/RRS.4MBLLC$11K.4C/ $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/RQ.4MBLLC.$2.1K.4C/ \
    | column -t -o ',' >> stats_scripts/data/perf.stat;
echo ".  0  0  0" | column -t >> stats_scripts/data/perf.stat;
 
# Avg - SPEC-14
perl getdata.pl -gmean -n 0  -nh -ns -gmean -w spec_14 -ipc 4 -ws -b $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C1/BASELINE.4MBLLC.1C/   \
    -d  $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/BASELINE.4MBLLC.4C/ $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/RRS.4MBLLC$11K.4C/ $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/RQ.4MBLLC.$2.1K.4C/  \
    | sed 's/Gmean/SPEC-14/' | column -t -o ',' >> stats_scripts/data/perf.stat;
# Avg - MIX-14
perl getdata.pl -gmean -n 0 -nh -ns -gmean -w mix_14 -ipc 4 -ws -b $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C1/BASELINE.4MBLLC.1C/   \
    -d  $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/BASELINE.4MBLLC.4C/ $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/RRS.4MBLLC$11K.4C/ $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/RQ.4MBLLC.$2.1K.4C/ \
    | sed 's/Gmean/MIX-14/' | column -t -o ',' >> stats_scripts/data/perf.stat;
# Avg - ALL-28
perl getdata.pl -gmean -n 0 -nh -ns -gmean -w spec_mix_28 -ipc 4 -ws -b $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C1/BASELINE.4MBLLC.1C/   \
    -d  $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/BASELINE.4MBLLC.4C/ $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/RRS.4MBLLC$11K.4C/ $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/RQ.4MBLLC.$2.1K.4C/  \
    | sed 's/Gmean/ALL-28/' | column -t -o ',' >> stats_scripts/data/perf.stat;
# Format
cat stats_scripts/data/perf.stat

### Uncomment for Normalized Performance based on Raw-IPC Metric ### 
# perl getdata.pl -n 0 -gmean -w spec_mix_28 -ipc 4  -d  $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/BASELINE/   $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/BASELINE/scatter-cache \
# $OUTPUT_DIR/multiprogram_16GBmem_100Mn.SPEC2006.C4/BASELINE/skew-vway-rand  | sed 's/[_A-Z]*\///' | sed 's/[_A-Z]*\/scatter-cache/scatter-cache/' | sed 's/[_A-Z]*\/skew-vway-rand/MIRAGE/' | column -t ;
