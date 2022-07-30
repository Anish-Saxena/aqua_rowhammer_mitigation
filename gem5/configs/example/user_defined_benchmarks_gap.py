import gap_benchmarks
import custom_benchmarks
import m5
from  m5.objects import *

## At-most 4-Core mix-workloads.
#mix_wls = {
#   'mix1' : ['leela',    'exchange2',       'bwaves',   'roms'],
#   'mix2' : ['xz',    'exchange2',  'omnetpp',        'perlbench'],
#   'mix3' : ['exchange2',    'roms',  'imagick',      'xz'],
#   'mix4' : ['namd',    'roms',      'cactuBSSN',         'leela'],
#}


def create_process(benchmark_name):    
    print( 'Selected benchmark')
    if benchmark_name == 'cc':
        print( '--> cc')
        process = gap_benchmarks.cc
    elif benchmark_name == 'bc':
        print( '--> bc')
        process = gap_benchmarks.bc
    elif benchmark_name == 'bfs':
        print( '--> bfs')
        process = gap_benchmarks.bfs
    elif benchmark_name == 'sssp':
        print( '--> sssp')
        process = gap_benchmarks.sssp
    elif benchmark_name == 'pr':
        print( '--> pr')
        process = gap_benchmarks.pr
    elif benchmark_name == 'tc':
        print( '--> tc')
        process = gap_benchmarks.tc
    else:
        print( "No recognized GAP benchmark selected! Exiting.")
        sys.exit(1)
    return  process

def create_proc (benchmark_name,id):
    import re
    #Check if mix-benchmark
    procName = ""
    if(re.match(r"^mix[0-9]+$",benchmark_name)):
       # Do mix
       #procName = mix_wls[benchmark_name][id]
       #No Suppoort
       print( "No support for GAP mix workload! Exiting.")
       sys.exit(1)
    else :       
       # Do rate-mode
       procName = benchmark_name
    print ("Benchmark-", str(id), " ", procName)
    curr_proc = create_process(procName)
    new_proc = Process(pid = 100 +id)
    new_proc.executable  = curr_proc.executable
    new_proc.cmd  = curr_proc.cmd
    new_proc.input = curr_proc.input
    new_proc.cwd   = curr_proc.cwd 
    print("CMD:",new_proc.cmd)
    print("CWD:",new_proc.cwd)
    return new_proc
