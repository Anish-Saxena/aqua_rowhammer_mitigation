cd ..
mkdir -p stats_scripts/data 

OUTPUT_DIR=$GEM5_PATH/stats/

perl getdata.pl  -w spec17_all -amean -nstat system.l3cache.overall_misses::total -dstat sim_inst -mstat 1000  \
-d  $OUTPUT_DIR/multiprogram_16GBmem_250Mn.SPEC2017.C4/O3.25Bn.RRS_NO_DELAY.1K.4C/ \
| column -t -o ',' > stats_scripts/data/mpki.stat

cat stats_scripts/data/mpki.stat
