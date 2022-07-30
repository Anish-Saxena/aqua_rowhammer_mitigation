## AQUA: Scalable Rowhammer Mitigation by Quarantining Aggressor Rows at Runtime
Authors: Gururaj Saileshwar and Moinuddin Qureshi, Georgia Institute of Technology.  
Appears in USENIX Security 2021.   

### Citation
Gururaj Saileshwar and Moinuddin Qureshi. "MIRAGE: Mitigating Conflict-Based Cache Attacks with a Practical Fully-Associative Design". In 30th USENIX Security Symposium (USENIX Security 21). 2021.

### Introduction

This artifact covers the performance analysis of Aqua and RRS Rowhammer mitigations. 

### Requirements For Performance Evaluations in Gem5 CPU Simulator:
   - **SW Dependencies:** Gem5 Dependencies - gcc, Python-3.6.3, scons-3.
     - Tested with gcc v6.4.0 and scons-3.0.5
     - Scons-3.0.5 download [link](https://sourceforge.net/projects/scons/files/scons/3.0.5/scons-3.0.5.tar.gz/download). To install, `tar -zxvf scons-3.0.5.tar.gz` and `cd scons-3.0.5; python setup.py install` (use `--prefix=<PATH>` for local install).
   - **Benchmark Dependencies:** [SPEC-2017](https://www.spec.org/cpu2017/) Installed.
   - **HW Dependencies:** 
     - A 36 CPU Core or more system, to finish experiments in ~6 days. 

### Steps for Gem5 Evaluation
Here you will recreate results in Figure 3, 6, 7, 9, 10, and 11, by executing the following steps:
- **Compile Gem5:** `cd gem5 ; scons -j50 build/X86/gem5.opt`
- **Set Paths** in `scripts/env.sh`. You will set the following :
    - `GEM5_PATH`: the full path of the gem5 directory (current directory).
    - `SPEC_PATH`: the path to your SPEC-CPU2017 installation. 
    - `CKPT_PATH`: the path to a new folder where the checkpoints will be created next.
    - Please source the paths as: `source scripts/env.sh` after modifying the file.
- **Test Creating and Running Checkpoints:** For each program the we need to create a checkpoint of the program state after the initialization phase of the program is complete, which will be used to run the simulations with different hardware configurations. 
    - To test the checkpointing process, run `cd scripts; ./ckptscript_test.sh perlbench 4 2017;`: this will create a checkpoint after 1Mn instructions (should complete in a couple of minutes). Once it completes, run `./runscript_test.sh perlbench Test 4 2017`: this will run the baseline design for 1Mn instructions from the checkpoint.
      * In case the `ckptscript_test.sh` fails with the error `$SPEC_PATH/SPEC2017_inst/benchspec/CPU/500.perlbench_r/run/run_base_refrate_gem5_se-m64.0000/perlbench_r_base.gem5_se-m64: No such file or directory`, it indicates the script is unable to find the run-directory for perlbench. Please follow the steps outlined in [README_SPEC_INSTALLATION.md](./README_SPEC_INSTALLATION.md) to ensure the run-directories are properly set up for all the SPEC-benchmarks.
    - To check if the run is successfully complete, check `less ../output/multiprogram_8Gmem_100K.C4/Test/Baseline/perlbench/runscript.log`. The last line should have `Exiting .. because a thread reached the max instruction count`.
- **Run All Experiments:** for all the benchmarks, run `./run_all_exp.sh`. This will run the following scripts:
    - `./run.perf.4C.sh` - This creates checkpoints and runs the experiments for the performance-results with 8MB LLC (shared among 4-cores). Specifically it runs:
      * **Create Checkpoint:** For each benchmark, the checkpoints will be created using `./ckptscript.sh <BMARK> 4`. 
      	- By default, `ckptscript.sh` is run for 42 programs in parallel (14 single-program, 14 multi-core and  14 mixed workloads). Please modify run.perf.4C.sh if your system cannot support 28 - 42 parallel threads.
      	- For each program, the execution is forwarded by 10 Billion Instructions (by when the initialization of the program should have completed) and then the architectural state (e.g. registers, memory) is checkpointed. Subsequently, when each HW-config is simulated, these checkpoints will be reloaded.
      	- This process can take 12 hours for each benchmark. Hence, all the benchmarks are run in parallel by default.
      	- Please see `../configs/example/spec06_config.py` for list of benchmarks supported.
      * **Run experiments**: Once all the checkpoints are created, the experiments will be run using `./runscript.sh <BMARK> <RUN-NAME> <SCHEME>`, where each HW config (Baseline, Scatter-Cache, MIRAGE) is simulated for each benchmark.
      	- The arguments for `runscript.sh` are as follows:
          -  RUN-NAME: Any string that will be used to identify this run, and the name for the results-folder of this run.
          -  SCHEME: [Baseline, scatter-cache, skew-vway-rand]. (skew-vway-rand is MIRAGE).
          -  NUM_CORES: Number of cores (default is 4).
          -  LLCSZ: Size of the LLC (default is 8MB).
          -  ENCRLAT: Encryptor Latency (default is 3 cycles).
      	- Each program is simulated for 1 billion instructions. This takes ~8 hours per benchmark, per scheme. Benchmarks in 2-3 schemes are run in parallel for a total of up to 84 parallel Gem5 runs at a time (please modify run.perf.4C.sh if your system cannot support upto 80 parallel threads).
      * **Generate results:** `cd stats_scripts; ./data_perf.sh`. This will compare the normalized performance (using weighted speedup metric) vs baseline.
        - The normalized peformance results will be stored in `stats_scripts/data/perf.stat`. 
        - Script to collect the LLC misses-per-thousand-instructions (MPKI) for each of the schemes is also available in `stats_scripts/data_mpki.sh`.
    - `./run.sensitivity.cachesz.sh` - This runs the evaluations for sensitivity to LLC-Size from 2MB to 64MB (shared between 4-cores)
      * Experiments are run using the script `./runscript.sh`
      * Results for normalized Perf vs. LLCSz can be generated using `cd stats_scripts; ./data_LLCSz.sh`. 
      * Results are stored in `stats_scripts/data/perf.LLCSz.stat`.
    - `./run.sensitivity.encrlat.sh` - This runs the evaluations for Encryption-latencies from 1 to 5 (used in cache-indexing).
      * Experiments are run using the script `./runscript.sh`
      * Results for normalized Perf vs. EncrLat can be generated using `cd stats_scripts; ./data_EncrLat.sh`. 
      * Results are stored in `stats_scripts/data/perf.EncLat.stat`.
- **Visualize the results:** Graphs can be generated using jupyter notebook `graphs/plot_graphs.ipynb` for Performance, LLCSz vs Perf., EncrLat vs Perf.
- **Note on Simulation Time:** Running all experiments takes almost 3-4 days on a system supporting 72 threads. 
    - To shorten experiment run time, you may reduce instruction count in `runscript.sh`to 500 Million.
    - You can also run only `./run.perf.4C.sh` and skip the sensitivity analysis.
    - You can also run many more parallel gem5 sims if your system supports it by modifying the sleep-loops in `run.perf.4C.sh` and `run.sensitivity.*.sh`.

### Acknowledgements

This artifact has been adapted from the artifact repository of [MIRAGE (USENIX Security 2021)](https://github.com/gururaj-s/mirage).