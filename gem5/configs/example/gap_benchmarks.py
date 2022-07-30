import m5
from m5.objects import *
import os

## Paths
GAP_PATH        = os.environ['GAP_PATH']
RUN_DIR_prefix  = '/gapbs_amd/'
INPUT_DIR_prefix = 'benchmark/graphs/raw/'

#cc
cc = Process() 
cc.cwd = GAP_PATH + RUN_DIR_prefix 
cc.executable =  cc.cwd + 'cc' 
cc.cmd = [cc.executable] + ['-n','16','-f',cc.cwd+INPUT_DIR_prefix+'USA-road-d.USA.gr']

#bc
bc = Process() 
bc.cwd = GAP_PATH + RUN_DIR_prefix 
bc.executable =  bc.cwd + 'bc' 
bc.cmd = [bc.executable] + ['-n','16','-f',bc.cwd+INPUT_DIR_prefix+'USA-road-d.USA.gr']

#tc
tc = Process() 
tc.cwd = GAP_PATH + RUN_DIR_prefix 
tc.executable =  tc.cwd + 'tc' 
tc.cmd = [tc.executable] + ['-n','3','-sf',tc.cwd+INPUT_DIR_prefix+'USA-road-d.USA.gr']

#bfs
bfs = Process() 
bfs.cwd = GAP_PATH + RUN_DIR_prefix 
bfs.executable =  bfs.cwd + 'bfs' 
bfs.cmd = [bfs.executable] + ['-n','64','-f',bfs.cwd+INPUT_DIR_prefix+'USA-road-d.USA.gr']


#sssp
sssp = Process() 
sssp.cwd = GAP_PATH + RUN_DIR_prefix 
sssp.executable =  sssp.cwd + 'sssp' 
sssp.cmd = [sssp.executable] + ['-n','64','-f',sssp.cwd+INPUT_DIR_prefix+'USA-road-d.USA.gr']


#pr
pr = Process() 
pr.cwd = GAP_PATH + RUN_DIR_prefix 
pr.executable =  pr.cwd + 'pr' 
pr.cmd = [pr.executable] + ['-n','16','-i','4','-f',pr.cwd+INPUT_DIR_prefix+'USA-road-d.USA.gr']
