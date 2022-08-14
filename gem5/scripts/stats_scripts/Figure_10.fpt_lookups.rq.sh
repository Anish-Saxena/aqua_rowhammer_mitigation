cd ..
mkdir -p stats_scripts/data 

## Calculating Mitigations for RRS ###

# All SPEC workloads
perl getdata.pl -noxxxx -w spec17_all -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_btv_true_neg"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C/BF-0/' \
    | sed 's/[_A-Z]*\///' \
    | column -t > stats_scripts/data/fpt_lookup_btv.stat ;

perl getdata.pl -noxxxx -w spec17_all -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_hit"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C/Cache-Hit/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/fpt_lookup_cache.stat ;

perl getdata.pl -noxxxx -w spec17_all -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_partial_hit_orr_set"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C/Singleton/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/fpt_lookup_orr.stat ;

perl getdata.pl -noxxxx -w spec17_all -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_partial_hit_orr_unset|rh_cache_miss"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C/DRAM-Acco/' \
    | sed 's/[_A-Z]*\///' | column -t > stats_scripts/data/fpt_lookup_dram.stat ;

paste stats_scripts/data/fpt_lookup_btv.stat stats_scripts/data/fpt_lookup_cache.stat \
    stats_scripts/data/fpt_lookup_orr.stat stats_scripts/data/fpt_lookup_dram.stat \
    | column -s $'\t' -t | awk '{print $1, $2, $4, $6, $8}' | column -t > stats_scripts/data/hitloc_rq_drit.stat ;

echo ".  0  0  0  0" | column -t >> stats_scripts/data/hitloc_rq_drit.stat ; 

# All SPEC workloads
perl getdata.pl -noxxxx -nh -ns -amean -w spec17_all -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_btv_true_neg"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C/BF-0/' \
    | sed 's/[_A-Z]*\///' | sed 's/Amean/AMEAN/' | column -t > stats_scripts/data/fpt_lookup_btv.stat ;

perl getdata.pl -noxxxx -nh -ns -amean -w spec17_all  -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_hit"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C/Cache-Hit/' \
    | sed 's/[_A-Z]*\///' | sed 's/Amean/AMEAN/'  | column -t > stats_scripts/data/fpt_lookup_cache.stat ;

perl getdata.pl -noxxxx -nh -ns -amean -w spec17_all  -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_partial_hit_orr_set"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C/Singleton/' \
    | sed 's/[_A-Z]*\///' | sed 's/Amean/AMEAN/'  | column -t > stats_scripts/data/fpt_lookup_orr.stat ;

perl getdata.pl -noxxxx -nh -ns -amean -w spec17_all  -dstat "rh_btv_true_pos|rh_btv_false_pos|rh_btv_true_neg" -nstat "rh_cache_partial_hit_orr_unset|rh_cache_miss"  \
    -d   ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C \
    | sed 's/[_A-Z0-9]*[\/]*AE.AQUA.MEMORY_MAPPED_TABLES.1K.4C/DRAM-Acco/' \
    | sed 's/[_A-Z]*\///' | sed 's/Amean/AMEAN/'  | column -t > stats_scripts/data/fpt_lookup_dram.stat ;

paste stats_scripts/data/fpt_lookup_btv.stat stats_scripts/data/fpt_lookup_cache.stat \
    stats_scripts/data/fpt_lookup_orr.stat stats_scripts/data/fpt_lookup_dram.stat \
    | column -s $'\t' -t | awk '{print $1, $2, $4, $6, $8}' | column -t >> stats_scripts/data/hitloc_rq_drit.stat ;

cat stats_scripts/data/hitloc_rq_drit.stat
