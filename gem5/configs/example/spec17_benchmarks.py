import m5
from m5.objects import *
import os

## Paths
SPEC_PATH       = os.environ['SPEC17_PATH']
RUN_DIR_prefix  = '/SPEC2017_inst/benchspec/CPU/'
RUN_DIR_postfix = '_r/run/run_base_refrate_gem5_se-m64.0000/'
x86_suffix = '_r_base.gem5_se-m64'

#temp
#binary_dir = spec_dir
#data_dir = spec_dir

#500.perlbench
perlbench = Process() # Update June 7, 2017: This used to be LiveProcess()
perlbench.cwd = SPEC_PATH + RUN_DIR_prefix + '500.perlbench' + RUN_DIR_postfix 
perlbench.executable =  perlbench.cwd + 'perlbench' + x86_suffix
perlbench.cmd = [perlbench.executable] + ['-I'+perlbench.cwd+'lib', perlbench.cwd+'checkspam.pl', '2500', '5', '25', '11', '150', '1', '1', '1', '1']

#502.gcc
gcc = Process() # Update June 7, 2017: This used to be LiveProcess()
gcc.cwd = SPEC_PATH + RUN_DIR_prefix + "502.gcc" + RUN_DIR_postfix
gcc.executable = gcc.cwd + 'cpugcc' + x86_suffix
gcc.cmd = [gcc.executable] + [gcc.cwd+'gcc-pp.c', '-O3', '-finline-limit=0', '-fif-conversion', '-fif-conversion2',
                                 '-o', gcc.cwd+'gcc-pp.opts-O3_-finline-limit_0_-fif-conversion_-fif-conversion2.s']

#503.bwaves
bwaves = Process() # Update June 7, 2017: This used to be LiveProcess()
bwaves.cwd = SPEC_PATH + RUN_DIR_prefix + "503.bwaves" + RUN_DIR_postfix
bwaves.executable = bwaves.cwd + 'bwaves' + x86_suffix
bwaves.cmd = [bwaves.executable] + [bwaves.cwd+'bwaves_1']
bwaves.input = bwaves.cwd + 'bwaves_1.in'

#505.mcf
mcf = Process() # Update June 7, 2017: This used to be LiveProcess()
mcf.cwd = SPEC_PATH + RUN_DIR_prefix + "505.mcf" + RUN_DIR_postfix
mcf.executable =  mcf.cwd + 'mcf' + x86_suffix
mcf.cmd = [mcf.executable] + [mcf.cwd+'inp.in']

#507.cactuBSSN
cactuBSSN = Process() # Update June 7, 2017: This used to be LiveProcess()
cactuBSSN.cwd = SPEC_PATH + RUN_DIR_prefix + "507.cactuBSSN" + RUN_DIR_postfix
cactuBSSN.executable = cactuBSSN.cwd + 'cactusBSSN' + x86_suffix
cactuBSSN.cmd = [cactuBSSN.executable] + [cactuBSSN.cwd+'spec_ref.par']

#508.namd =>  => PROBABLY_DOES_NOT_WORK
namd = Process() # Update June 7, 2017: This used to be LiveProcess()
namd.cwd = SPEC_PATH + RUN_DIR_prefix + "508.namd" + RUN_DIR_postfix
namd.executable = namd.cwd + 'namd' + x86_suffix
namd.cmd = [namd.executable] + ['--input', namd.cwd+'apoa1.input', '--output', namd.cwd+'apoa1.ref.output', '--iterations', '65']

#510.parest
parest = Process() # Update June 7, 2017: This used to be LiveProcess()
parest.cwd = SPEC_PATH + RUN_DIR_prefix + "510.parest" + RUN_DIR_postfix
parest.executable = parest.cwd + 'parest' + x86_suffix
parest.cmd = [parest.executable] + [parest.cwd+'ref.prm']

#511.povray
povray = Process() # Update June 7, 2017: This used to be LiveProcess()
povray.cwd = SPEC_PATH + RUN_DIR_prefix + "511.povray" + RUN_DIR_postfix
povray.executable = povray.cwd + 'povray' + x86_suffix
povray.cmd = [povray.executable] + [povray.cwd+'SPEC-benchmark-ref.ini']

#519.lbm
lbm = Process() # Update June 7, 2017: This used to be LiveProcess()
lbm.cwd = SPEC_PATH + RUN_DIR_prefix + "519.lbm" + RUN_DIR_postfix
lbm.executable = lbm.cwd + 'lbm' + x86_suffix
lbm.cmd = [lbm.executable] + ['3000', 'reference.dat', '0', '0', lbm.cwd+'100_100_130_ldc.of']
#calculix.output = out_dir + 'calculix.out'

#520.omnetpp
omnetpp = Process() # Update June 7, 2017: This used to be LiveProcess()
omnetpp.cwd = SPEC_PATH + RUN_DIR_prefix + "520.omnetpp" + RUN_DIR_postfix
omnetpp.executable = omnetpp.cwd+'omnetpp' + x86_suffix
omnetpp.cmd = [omnetpp.executable] + ['-c', 'General', '-r', '0']

#521.wrf
wrf = Process() # Update June 7, 2017: This used to be LiveProcess()
wrf.cwd = SPEC_PATH + RUN_DIR_prefix + "521.wrf" + RUN_DIR_postfix
wrf.executable = wrf.cwd+'wrf' + x86_suffix
wrf.cmd = [wrf.executable]

#523.xalancbmk
xalancbmk = Process() # Update June 7, 2017: This used to be LiveProcess()
xalancbmk.cwd = SPEC_PATH + RUN_DIR_prefix + "523.xalancbmk" + RUN_DIR_postfix
xalancbmk.executable = xalancbmk.cwd + 'cpuxalan' + x86_suffix
xalancbmk.cmd = [xalancbmk.executable] + ['-v',xalancbmk.cwd+'t5.xml',xalancbmk.cwd+'xalanc.xsl']

#525.x264
x264 = Process() # Update June 7, 2017: This used to be LiveProcess()
x264.cwd = SPEC_PATH + RUN_DIR_prefix + "525.x264" + RUN_DIR_postfix
x264.executable = x264.cwd + 'x264' + x86_suffix
x264.cmd = [x264.executable] + ['--pass', '1', '--stats', x264.cwd+'x264_stats.log', '--bitrate', '1000', '--frames', '1000',
                 '-o', x264.cwd+'BuckBunny_New.264', x264.cwd+'BuckBunny.yuv', '1280x720']

#526.blender
blender = Process() # Update June 7, 2017: This used to be LiveProcess()
blender.cwd = SPEC_PATH + RUN_DIR_prefix + "526.blender" + RUN_DIR_postfix
blender.executable = blender.cwd + 'blender' + x86_suffix
blender.cmd = [blender.executable] + [blender.cwd+'sh3_no_char.blend', '--render-output', 'sh3_no_char_', '--threads',
                 '1', '-b', '-F', 'RAWTGA', '-s', '849', '-e', '849', '-a']

#527.cam4 => PROBABLY_DOES_NOT_WORK
cam4 = Process() # Update June 7, 2017: This used to be LiveProcess()
cam4.cwd = SPEC_PATH + RUN_DIR_prefix + "527.cam4" + RUN_DIR_postfix
cam4.executable = cam4.cwd + 'cam4' + x86_suffix
cam4.cmd = [cam4.executable]

#531.deepsjeng
deepsjeng = Process() # Update June 7, 2017: This used to be LiveProcess()
deepsjeng.cwd = SPEC_PATH + RUN_DIR_prefix + "531.deepsjeng" + RUN_DIR_postfix
deepsjeng.executable = deepsjeng.cwd + 'deepsjeng' + x86_suffix
deepsjeng.cmd = [deepsjeng.executable] + [deepsjeng.cwd+'ref.txt']

#538.imagick
imagick = Process() # Update June 7, 2017: This used to be LiveProcess()
imagick.cwd = SPEC_PATH + RUN_DIR_prefix + "538.imagick" + RUN_DIR_postfix
imagick.executable = imagick.cwd + 'imagick' + x86_suffix
imagick.cmd = [imagick.executable] + ['-limit', 'disk', '0', imagick.cwd+'refrate_input.tga', '-edge', '41', '-resample',
                 '181%', '-emboss', '31', '-colorspace', 'YUV', '-mean-shift', '19x19+15%', '-resize', '30%',
                  imagick.cwd + 'refrate_output.tga']

#541.leela => PROBABLY_DOES_NOT_WORK
leela = Process() # Update June 7, 2017: This used to be LiveProcess()
leela.cwd = SPEC_PATH + RUN_DIR_prefix + "541.leela" + RUN_DIR_postfix
leela.executable = leela.cwd + 'leela' + x86_suffix
leela.cmd = [leela.executable] + [leela.cwd+'ref.sgf']

#544.nab
nab = Process() # Update June 7, 2017: This used to be LiveProcess()
nab.cwd = SPEC_PATH + RUN_DIR_prefix + "544.nab" + RUN_DIR_postfix
nab.executable = nab.cwd + 'nab' + x86_suffix
nab.cmd = [nab.executable] + ['1am0', '1122214447', '122']

#548.exchange2
exchange2 = Process() # Update June 7, 2017: This used to be LiveProcess()
exchange2.cwd = SPEC_PATH + RUN_DIR_prefix + "548.exchange2" + RUN_DIR_postfix
exchange2.executable = exchange2.cwd + 'exchange2' + x86_suffix
exchange2.cmd = [exchange2.executable] + ['6']

#549.fotonik3d
fotonik3d = Process() # Update June 7, 2017: This used to be LiveProcess()
fotonik3d.cwd = SPEC_PATH + RUN_DIR_prefix + "549.fotonik3d" + RUN_DIR_postfix
fotonik3d.executable = fotonik3d.cwd + 'fotonik3d' + x86_suffix
fotonik3d.cmd = [fotonik3d.executable]

#554.roms
roms = Process() # Update June 7, 2017: This used to be LiveProcess()
roms.cwd = SPEC_PATH + RUN_DIR_prefix + "554.roms" + RUN_DIR_postfix
roms.executable = roms.cwd + 'roms' + x86_suffix
roms.cmd = [roms.executable]
roms.input = roms.cwd+'ocean_benchmark2.in.x'

#557.xz
xz = Process() # Update June 7, 2017: This used to be LiveProcess()
xz.cwd = SPEC_PATH + RUN_DIR_prefix + "557.xz" + RUN_DIR_postfix
xz.executable = xz.cwd + 'xz' + x86_suffix
xz.cmd = [xz.executable] + [xz.cwd+'cld.tar.xz', '160', 
'19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c13e1c0c9e6aaea8dff3efb4ad6b0456697718cede6bd5454852652806a657bb56e07d61128434b474',
'59796407', '61004416', '6']