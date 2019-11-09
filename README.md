Fast binary coverage using Qemu and a Forkserver
============

You want to inspect the coverage produced by your fuzzer, but kcov/gcov are slow and require special builds of the
target? You misplaced the sourcecode and can't even build the target with coverage instrumentation?

**cov.py to the rescue!**

Based on AFL's Qemu mode, cov.py quickly produces edge coverage information on binary targets. It uses the AFL's
forkserver to speed up coverage calculation massively and generates precise trace files.

Setup
=====
```
git@github.com:eqv/aflq_fast_cov.git
cd aflq_fast_cov.git
sh build_qemu.sh
```

Usage
====
```bash
$ python cov.py -h
```
```
usage: cov.py [-h] -i input_folder -o output_folder -m wordize [-t [timeout]]
              ...

Fast Forkserver Base Binary Coverage

positional arguments:
  ...               Command to run, @@ is replaced by input file

optional arguments:
  -h, --help        show this help message and exit
  -i input_folder   Where to look for input files
  -o output_folder  Where to store the trace files
  -m wordize        Architecture to use for QEMU
  -t [timeout]      Timeout in seconds

```
```bash
$ python cov.py -i workdir/in -o workdir/out -m64 workdir/test
```
```
Running workdir/test with timeout 1.000000 on 3 inputs

run workdir/in/a
run workdir/in/c
run workdir/in/b

```

Output Format
============
`cov.py` create one or more `input_$(input_basename)_thread_$(threadid).qemu` trace files per input file and thread used by the target.
```
#  input: in/test1 -.        .- Thread: 10702
#                   v        v
$ cat out/input_test1_thread_10702.qemu
```

Each `.qemu` trace file contains all CFG edges __in the order they were encountered__ by the given thread. 
The format is `src_bb_entry_addr, dst_bb_entry_addr, dst_bb_size`. The addresses are in hex, the size in decimal. The
basic block "ffffffffffffffff" encodes any basic block outside of the trace region.


```
$ cat out/input_test1_thread_10702.qemu
ffffffffffffffff,400550,41 # jump to the basic block at 0x400550 with size 41 from outside of the trace region
400550,400520,6 # jump to the basic block at 0x400520 with size 41 from the cofi at the end of 0x400550
400520,400526,10
400526,4004e0,12
4004e0,ffffffffffffffff,67 # program exits trace range, (e.g. call a library)
ffffffffffffffff,400770,49 # library returns to the program
400770,4004b8,16
...
```
