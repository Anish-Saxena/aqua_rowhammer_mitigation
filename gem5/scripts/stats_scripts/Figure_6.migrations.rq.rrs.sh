cd ..
mkdir -p stats_scripts/data 

## Calculating Mitigations for RRS ###

# All SPEC workloads
perl getdata.pl -noxxxx -w spec17_all -amean -mstat 2 -dstat sim_seconds -nstat "rrs_clean_install|rrs_clean_reswap|rrs_only_unswap|rrs_dirty_reswap"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.RRS.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.RRS.1K.4C/RRS/' | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.SRAM_TABLES.1K.4C/AQUA/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/migrations1_rrs.stat ;

perl getdata.pl -noxxxx -w spec17_all -amean -mstat 2 -dstat sim_seconds -nstat "rrs_clean_reswap|rrs_dirty_reswap|rrs_only_unswap"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.RRS.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.RRS.1K.4C/RRS/' | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.SRAM_TABLES.1K.4C/AQUA/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/migrations2_rrs.stat ;

perl getdata.pl -noxxxx -w spec17_all -amean -mstat 2 -dstat sim_seconds -nstat "rrs_dirty_reswap"  \
    -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.RRS.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.RRS.1K.4C/RRS/' | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.SRAM_TABLES.1K.4C/AQUA/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/migrations3_rrs.stat ;

paste stats_scripts/data/migrations1_rrs.stat stats_scripts/data/migrations2_rrs.stat stats_scripts/data/migrations3_rrs.stat \
    | column -s $'\t' -t | awk '{print $1,($2+$4+$6)*0.064}' | column -t > stats_scripts/data/migrations_rrs.stat

#cat stats_scripts/data/migrations_rrs.stat
rm stats_scripts/data/migrations1_rrs.stat stats_scripts/data/migrations2_rrs.stat stats_scripts/data/migrations3_rrs.stat
    
# Format
# cat stats_scripts/data/migrations_rrs.stat


## Calculating Mitigations for RQ ###

# All SPEC workloads
perl getdata.pl -noxxxx -w spec17_all  -amean -nstat "rh_move" -dstat sim_seconds \
    -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.SRAM_TABLES.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.RRS.1K.4C/RRS/' | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.SRAM_TABLES.1K.4C/AQUA/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/migrations1_rq.stat ;

perl getdata.pl -noxxxx -w spec17_all  -amean -nstat "rh_move_to_qr_remove|rh_move_within_qr_remove"  -dstat sim_seconds  \
    -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.SRAM_TABLES.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.RRS.1K.4C/RRS/' | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.SRAM_TABLES.1K.4C/AQUA/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/migrations2_rq.stat ;

paste stats_scripts/data/migrations1_rq.stat stats_scripts/data/migrations2_rq.stat  \
    | column -s $'\t' -t | awk '{print $1,($2+$4)*0.064}' | column -t > stats_scripts/data/migrations_rq.stat

# Format
#cat stats_scripts/data/migrations_rq.stat
# rm -rf stats_scripts/data/migrations_rq.stat stats_scripts/data/migrations_rrs.stat

#Join
paste stats_scripts/data/migrations_rrs.stat stats_scripts/data/migrations_rq.stat \
    | column -s $'\t' -t | awk '{print $1,$2, $4}' | column -t > stats_scripts/data/migrations_rrs_rq.stat

#Print
cat stats_scripts/data/migrations_rrs_rq.stat
