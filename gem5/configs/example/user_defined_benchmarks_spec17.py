import spec17_benchmarks
import custom_benchmarks
import m5
from  m5.objects import *

# We left out cam4, xalancbmk, fotonik3d, and omnetpp
mix_wls = {
   'mix1' : ['leela',    'exchange2',       'bwaves',   'roms'],
   'mix2' : ['xz',    'namd',  'parest',        'perlbench'],
   'mix3' : ['exchange2',    'roms',  'imagick',      'xz'],
   'mix4' : ['namd',    'deepsjeng',      'cactuBSSN',         'leela'],
   'mix5' : ['parest',    'wrf',      'gcc',   'cactuBSSN'],
   'mix6' : ['nab',      'lbm',    'imagick',       'povray'],
   'mix7' : ['povray',    'roms',        'nab',  'bwaves'],
   'mix8' : ['xz',  'leela',      'parest',     'deepsjeng'],
   'mix9' : ['deepsjeng',  'blender',    'gcc',         'wrf'],
   'mix10': ['wrf',     'povray',    'mcf',         'nab'],
   'mix11': ['cactuBSSN',    'lbm',    'mcf',         'xz'],
   'mix12': ['roms',  'mcf',     'imagick',         'blender'],
   'mix13': ['lbm',  'mcf',     'wrf',         'exchange2'],
   'mix14': ['gcc',  'imagick',     'lbm',         'wrf'],
   'mix15': ['cactuBSSN',  'xz',     'roms',         'namd'],
   'mix16': ['imagick',  'blender',     'cactuBSSN',         'gcc'],
   'mix17': ['blender',    'x264',  'gcc',       'exchange2'],
   'mix18': ['leela',     'x264',        'mcf',     'parest'],
   'mix19': ['cactuBSSN',  'mcf',     'x264',         'exchange2'],
   'mix20': ['deepsjeng',  'mcf',     'namd',         'roms'],
   'mix21': ['povray',  'namd',     'nab',         'roms'],
   'mix22': ['x264',  'deepsjeng',     'blender',         'namd'],
   'mix23': ['exchange2',  'deepsjeng',     'parest',         'nab'],
   'mix24': ['povray',  'x264',     'nab',         'deepsjeng']
}


def create_process(benchmark_name):    
    print( 'Selected benchmark')
    if benchmark_name == 'perlbench':
        print( '--> perlbench')
        process = spec17_benchmarks.perlbench
    elif benchmark_name == 'gcc':
        print( '--> gcc')
        process = spec17_benchmarks.gcc
    elif benchmark_name == 'bwaves':
        print( '--> bwaves')
        process = spec17_benchmarks.bwaves
    elif benchmark_name == 'mcf':
        print( '--> mcf')
        process = spec17_benchmarks.mcf
    elif benchmark_name == 'cactuBSSN':
        print( '--> cactuBSSN')
        process = spec17_benchmarks.cactuBSSN
    elif benchmark_name == 'namd':
        print( '--> namd')
        process = spec17_benchmarks.namd
    elif benchmark_name == 'parest':
        print( '--> parest')
        process = spec17_benchmarks.parest
    elif benchmark_name == 'povray':
        print( '--> povray')
        process = spec17_benchmarks.povray
    elif benchmark_name == 'lbm':
        print( '--> lbm')
        process = spec17_benchmarks.lbm
    elif benchmark_name == 'omnetpp':
        print( '--> omnetpp')
        process = spec17_benchmarks.omnetpp
    elif benchmark_name == 'wrf':
        print( '--> wrf')
        process = spec17_benchmarks.wrf
    elif benchmark_name == 'xalancbmk':
        print( '--> xalancbmk')
        process = spec17_benchmarks.xalancbmk
    elif benchmark_name == 'x264':
        print( '--> x264')
        process = spec17_benchmarks.x264  
    elif benchmark_name == 'blender':
        print( '--> blender')
        process = spec17_benchmarks.blender
    elif benchmark_name == 'cam4':
        print( '--> cam4')
        process = spec17_benchmarks.cam4
    elif benchmark_name == 'deepsjeng':
        print( '--> deepsjeng')
        process = spec17_benchmarks.deepsjeng
    elif benchmark_name == 'imagick':
        print( '--> imagick')
        process = spec17_benchmarks.imagick
    elif benchmark_name == 'leela':
        print( '--> leela')
        process = spec17_benchmarks.leela
    elif benchmark_name == 'nab':
        print( '--> nab')
        process = spec17_benchmarks.nab
    elif benchmark_name == 'exchange2':
        print( '--> exchange2')
        process = spec17_benchmarks.exchange2
    elif benchmark_name == 'fotonik3d':
        print( '--> fotonik3d')
        process = spec17_benchmarks.fotonik3d
    elif benchmark_name == 'roms':
        print( '--> roms')
        process = spec17_benchmarks.roms
    elif benchmark_name == 'xz':
        print( '--> xz')
        process = spec17_benchmarks.xz
    else:
        print( "No recognized SPEC2017 benchmark selected! Exiting.")
        sys.exit(1)
    return  process


def create_proc (benchmark_name,id):
    import re
    #Check if mix-benchmark
    procName = ""
    if(re.match(r"^mix[0-9]+$",benchmark_name)):
       # Do mix
       procName = mix_wls[benchmark_name][id]
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
