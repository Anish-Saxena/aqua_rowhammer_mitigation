cd ..
mkdir -p stats_scripts/data 

## Calculating Mitigations for RRS ###

# All SPEC workloads
perl getdata.pl -noxxxx -w spec17_all -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_btv_true_neg"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/O3.25Bn.RQ.16KB_BTV_16KB_CACHE_V2.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*_16KB_CACHE_V2.1K.4C/BF-0/' \
    | sed 's/mix11/mix10/' | sed 's/mix13/mix11/' | sed 's/mix14/mix12/' | sed 's/mix16/mix13/' | sed 's/mix17/mix14/' | sed 's/mix18/mix15/' | sed 's/mix19/mix16/' \
    | sed 's/[_A-Z]*\///' \
    | column -t > stats_scripts/data/fpt_lookup_btv.stat ;

perl getdata.pl -noxxxx -w spec17_all -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_hit"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/O3.25Bn.RQ.16KB_BTV_16KB_CACHE_V2.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*_16KB_CACHE_V2.1K.4C/Cache-Hit/' \
    | sed 's/mix11/mix10/' | sed 's/mix13/mix11/' | sed 's/mix14/mix12/' | sed 's/mix16/mix13/' | sed 's/mix17/mix14/' | sed 's/mix18/mix15/' | sed 's/mix19/mix16/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/fpt_lookup_cache.stat ;

perl getdata.pl -noxxxx -w spec17_all -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_partial_hit_orr_set"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/O3.25Bn.RQ.16KB_BTV_16KB_CACHE_V2.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*_16KB_CACHE_V2.1K.4C/Singleton/' \
    | sed 's/mix11/mix10/' | sed 's/mix13/mix11/' | sed 's/mix14/mix12/' | sed 's/mix16/mix13/' | sed 's/mix17/mix14/' | sed 's/mix18/mix15/' | sed 's/mix19/mix16/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/fpt_lookup_orr.stat ;

perl getdata.pl -noxxxx -w spec17_all -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_partial_hit_orr_unset|rh_cache_miss"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/O3.25Bn.RQ.16KB_BTV_16KB_CACHE_V2.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*_16KB_CACHE_V2.1K.4C/DRAM-Acco/' \
    | sed 's/mix11/mix10/' | sed 's/mix13/mix11/' | sed 's/mix14/mix12/' | sed 's/mix16/mix13/' | sed 's/mix17/mix14/' | sed 's/mix18/mix15/' | sed 's/mix19/mix16/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/fpt_lookup_dram.stat ;

paste stats_scripts/data/fpt_lookup_btv.stat stats_scripts/data/fpt_lookup_cache.stat \
    stats_scripts/data/fpt_lookup_orr.stat stats_scripts/data/fpt_lookup_dram.stat \
    | column -s $'\t' -t | awk '{print $1, $2, $4, $6, $8}' | column -t > stats_scripts/data/test.stat ;

echo ".  0  0  0  0" | column -t >> stats_scripts/data/test.stat ; 

# All SPEC workloads
perl getdata.pl -noxxxx -nh -ns -amean -w spec17_all -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_btv_true_neg"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/O3.25Bn.RQ.16KB_BTV_16KB_CACHE_V2.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*_16KB_CACHE_V2.1K.4C/BF-0/' \
    | sed 's/mix11/mix10/' | sed 's/mix13/mix11/' | sed 's/mix14/mix12/' | sed 's/mix16/mix13/' | sed 's/mix17/mix14/' | sed 's/mix18/mix15/' | sed 's/mix19/mix16/' \
    | sed 's/[_A-Z]*\///' | sed 's/Amean/AMEAN/' | column -t > stats_scripts/data/fpt_lookup_btv.stat ;

perl getdata.pl -noxxxx -nh -ns -amean -w spec17_all  -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_hit"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/O3.25Bn.RQ.16KB_BTV_16KB_CACHE_V2.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*_16KB_CACHE_V2.1K.4C/Cache-Hit/' \
    | sed 's/mix11/mix10/' | sed 's/mix13/mix11/' | sed 's/mix14/mix12/' | sed 's/mix16/mix13/' | sed 's/mix17/mix14/' | sed 's/mix18/mix15/' | sed 's/mix19/mix16/' \
    | sed 's/[_A-Z]*\///' | sed 's/Amean/AMEAN/'  | column -t > stats_scripts/data/fpt_lookup_cache.stat ;

perl getdata.pl -noxxxx -nh -ns -amean -w spec17_all  -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_partial_hit_orr_set"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/O3.25Bn.RQ.16KB_BTV_16KB_CACHE_V2.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*_16KB_CACHE_V2.1K.4C/Singleton/' \
    | sed 's/mix11/mix10/' | sed 's/mix13/mix11/' | sed 's/mix14/mix12/' | sed 's/mix16/mix13/' | sed 's/mix17/mix14/' | sed 's/mix18/mix15/' | sed 's/mix19/mix16/' \
    | sed 's/[_A-Z]*\///' | sed 's/Amean/AMEAN/'  | column -t > stats_scripts/data/fpt_lookup_orr.stat ;

perl getdata.pl -noxxxx -nh -ns -amean -w spec17_all  -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_partial_hit_orr_unset|rh_cache_miss"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/O3.25Bn.RQ.16KB_BTV_16KB_CACHE_V2.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*_16KB_CACHE_V2.1K.4C/DRAM-Acco/' \
    | sed 's/mix11/mix10/' | sed 's/mix13/mix11/' | sed 's/mix14/mix12/' | sed 's/mix16/mix13/' | sed 's/mix17/mix14/' | sed 's/mix18/mix15/' | sed 's/mix19/mix16/' \
    | sed 's/[_A-Z]*\///' | sed 's/Amean/AMEAN/'  | column -t > stats_scripts/data/fpt_lookup_dram.stat ;

paste stats_scripts/data/fpt_lookup_btv.stat stats_scripts/data/fpt_lookup_cache.stat \
    stats_scripts/data/fpt_lookup_orr.stat stats_scripts/data/fpt_lookup_dram.stat \
    | column -s $'\t' -t | awk '{print $1, $2, $4, $6, $8}' | column -t >> stats_scripts/data/test.stat ;

#cat stats_scripts/data/migrations_rrs.stat
# rm stats_scripts/data/fpt_lookup_cache.stat stats_scripts/data/migrations2_rrs.stat stats_scripts/data/migrations3_rrs.stat
    
# Format
# cat stats_scripts/data/migrations_rrs.stat


## Calculating Mitigations for RQ ###

# # All SPEC workloads
# perl getdata.pl -noxxxx -w spec17_all  -amean -nstat "rh_move" -dstat sim_seconds \
#     -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.SRAM_TABLES.1K.4C \
#     | sed 's/[_A-Z0-9]*[\/]*O3.25Bn.RQ.16KB_BTV_16KB_CACHE_V2.1K.4C/AQUA/' \
#     | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/migrations1_rq.stat ;

# perl getdata.pl -noxxxx -w spec17_all  -amean -nstat "rh_move_to_qr_remove|rh_move_within_qr_remove"  -dstat sim_seconds  \
#     -d ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.SRAM_TABLES.1K.4C \
#     | sed 's/[_A-Z0-9]*[\/]*O3.25Bn.RQ.16KB_BTV_16KB_CACHE_V2.1K.4C/AQUA/' \
#     | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/migrations2_rq.stat ;

# paste stats_scripts/data/migrations1_rq.stat stats_scripts/data/migrations2_rq.stat  \
#     | column -s $'\t' -t | awk '{print $1,($2+$4)*0.064}' | column -t > stats_scripts/data/migrations_rq.stat

# # Format
# #cat stats_scripts/data/migrations_rq.stat
# # rm -rf stats_scripts/data/migrations_rq.stat stats_scripts/data/migrations_rrs.stat

# #Join
# paste stats_scripts/data/migrations_rrs.stat stats_scripts/data/migrations_rq.stat \
#     | column -s $'\t' -t | awk '{print $1,$2, $4}' | column -t > stats_scripts/data/migrations_rrs_rq.stat

# #Print
# cat stats_scripts/data/migrations_rrs_rq.stat
