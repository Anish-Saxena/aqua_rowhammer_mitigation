After installing SPEC-2017 on the disk, for running the SPECint-CPU2017 benchmarks with gem5, you need to create the run-directories for each SPEC benchmark with the following steps (these steps are only needed once for each benchmark):  
- Copy an example configuration to another file, for example, `cp config/Example-gcc-linux-x86.cfg config/gem5_se_test.cfg`.
- Open the `gem5_se_test.cfg` configuration file and change its label. For example, `define label gem5_se`.
- We **strongly recommend** changing the compiler optimization for microarchitecture and setting it to `athlon64` to ensure gem5 doesn't encounter unknown instructions. Specifically, we used `OPTIMIZE = -g -O3 -march=athlon64 -fno-unsafe-math-optimizations -fno-tree-loop-vectorize`.
- Run the following command: `runcpu --config=gem5_se_test --action=build 500.perlbench_r`. This command should build a benchmark (if it does not exist), create run directories and copy the executable & inputs to run folder and then run the benchmark. 
    - A helpful resource for understanding this process can be the [SPEC2017 installation page](https://www.spec.org/cpu2017/Docs/install-guide-unix.html), which outlines the steps to install and test SPEC benchmarks.
- Check `$SPEC17_PATH//SPEC2017_inst/benchspec/CPU/500.perlbench_r/run/`: there should be a run folder (like `run_base_refrate_gem5_se-m64.0000`) containing the executable and input files (e.g. `checkspam.pl`) for the benchmark.
- The exact name of the run-folder might be different based on your OS/CPU/compiler. If so, please follow the steps:
    * Open `spec17_benchmarks.py` file in `gem5/configs/example` directory.
    * Update the `RUN_DIR_postfix` variable as with run-folder (like `run_base_refrate_gem5_se-m64.0000`) and `x86_suffix` variable with executable binary postfix (like `_r_base.gem5_se-m64`).
- Lastly, you need to make sure gem5 runs with the correct executable names for benchmarks (this can also vary on different SPEC installations). Check the name of the executable in the run-folder. On our system, the binary name is `perlbench_r_base.gem5_se-m64`. Note the suffix after `perlbench` (e.g. `_r_base.gem5_se-m64`).
    * Open `spec17_benchmarks.py` file in `gem5/configs/example` directory.
    * Update the `x86_suffix` variable with the suffix on your system.  
- After these steps,`./ckptscript_test.sh perlbench 4 2017` should hopefully work: the script should be able to `cd` into the correct run-directory for the SPEC-benchmark and run the correct executable name with the right input parameters. 
    - You can check the command used in `cpts/output/multiprogram_16GBmem_1Mn.SPEC2017.C4/checkpoint_out/perlbench/runscript.log`.
- Once perlbench is successfully tested, to generate the run directories for all the other benchmarks, use: `runcpu --config=gem5_se_test --size=ref --copies=1  --noreportable  --iterations=1 intrate`.  
- **Note**: After all benchmark run-directories are created, please make sure run-directories of all benchmarks have the same naming format.    


After completing these steps, all the SPEC benchmnark run-directories should be set up. Now, you can try the next step using `runscript_test.sh` in [README.md](./README.md) to test gem5 functionality and then run all the experiments.



