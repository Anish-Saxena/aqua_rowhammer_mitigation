cd ..
mkdir -p stats_scripts/data 

## Normalized Performance based on Weighted-Speedup Metric ###
## Inverted for Slowdown ##
echo "RTH 2K 1K 500"  > stats_scripts/data/perf_RTH.stat

# Rate
perl getdata.pl -gmean -n 0 -nh -ns -gmean -w spec17_single  -ipc 4 -ws -printmask 0-1-1-1  \
    -b ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C1/AE.BASELINE.1C \
     -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.BASELINE.4C \
     ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.2K.4C \
     ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
     ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.500.4C \
    | sed 's/Gmean/Spec-Rate-18/' | column -t >> stats_scripts/data/perf_RTH.stat;

perl getdata.pl -gmean -n 0 -nh -ns -gmean -w spec17_mix  -ipc 4 -ws -printmask 0-1-1-1  \
    -b ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C1/AE.BASELINE.1C \
     -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.BASELINE.4C \
     ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.2K.4C \
     ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
     ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.500.4C \
    | sed 's/Gmean/Spec-Mix-16/' | column -t >> stats_scripts/data/perf_RTH.stat;

perl getdata.pl -gmean -n 0 -nh -ns -gmean -w spec17_all  -ipc 4 -ws -printmask 0-1-1-1  \
    -b ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C1/AE.BASELINE.1C \
     -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.BASELINE.4C \
     ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.2K.4C \
     ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
     ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.500.4C \
    | sed 's/Gmean/All-34/' | column -t >> stats_scripts/data/perf_RTH.stat;

# Format
cat stats_scripts/data/perf_RTH.stat
