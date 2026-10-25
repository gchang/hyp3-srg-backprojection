# Changes from upstream

Upstream: https://github.com/ASFHyP3/srg, tag `v0.4.3`, commit cc4bb67bae168f9f5e1c9a86147e96449fd9b980 (2026-06-04). The Stanford Radar Group (SRG) SAR processor by the Stanford Radar Group (Howard Zebker's group), used in the NASA-funded LAVAS project and delivered to ASF as source tarballs; ASF maintains the repository and builds it into the HyP3 plugin hyp3-srg (https://github.com/ASFHyP3/hyp3-srg). The repository has no license file. Inside it, `sbas/svd.f90` is distributed under the GNU LGPL, and `snaphu_v2.0b0.0.0/` is SNAPHU by Curtis W. Chen, Copyright Stanford University, under the terms in `snaphu_v2.0b0.0.0/README`. All notices are kept as they are.

This copy starts from upstream's file tree at cc4bb67 without its git history. It is unchanged except for the items below. Left out: `.github/` (ASF's CI workflows, issue templates and dependabot settings, which belong to the upstream repository).

## 2026-10-25: long work-folder paths

`sentinel_raw_process_cpu` (and the GPU `sentinel_raw_process`) is called with the full path of the measurement file inside the `.SAFE` folder. That name already takes about 145 characters, and the program copied it into 200-character buffers (`fname`, `basename`, `SAFEname`), so with a work folder path longer than about 60 characters it overflowed them and crashed with a segmentation fault before decoding anything. The same names, with `.geo`, `.orbtiming` and `.positionburst` added, then went through 200- and 300-character Fortran strings, and `initdk()` in `io.c` cut every file name it opened at 99 characters, so a long DEM or SLC path was opened under a shortened name.

All of these are now 1024 characters:
- `sentinel/sentinel_raw_process_cpu.c` and `sentinel/sentinel_raw_process.c`: `fname`, `basename`, `SAFEname`. The program stops with a message if its argument is still too long, and `SAFEname` is zero-filled before `strncpy` copies into it (`strncpy` does not terminate it).
- `sentinel/processsubcpu.f90`, `processsub.f90`, `sentineltimingsub.f90`, `getburststatevectors.f90`, `backprojectcpusub.f90`, `backprojectgpusub.f90`: `SAFEname` and the file-name strings built from it or read from `params`. The Fortran `SAFEname` length must equal the C buffer, because Fortran reads the whole declared length.
- `sentinel/createslc.f90` (output name), `DEM/createspecialdem.f90` and `DEM/geoid2008_ellipsoid_interpolate.f90` (input and output DEM paths and the geoid grid path).
- `sentinel/io.c` and `util/io.c` (`initdk`), `sentinel/filelen.c` and `util/filelen.c` (`filelen`, which now copies the name up to the first blank instead of a fixed 200 bytes).

Only buffer sizes change; the processing and the output are the same.

## 2026-10-25: snaphu builds with GCC 15 and later

GCC 15 and later compile C as C23 by default, where a function pointer declared with an empty parameter list means `(void)`. `snaphu_tile.c` assigns prototyped functions to such pointers (lines 401-402 and 637-638), which is now an error, so snaphu did not build; build scripts that do not stop on errors went on without it. `snaphu_v2.0b0.0.0/src/Makefile` now adds `-std=gnu17` with `override CFLAGS +=`, so it also applies when a build script passes its own `CFLAGS` (for example `make CFLAGS=-O3`). Same code, older language standard; snaphu's output is unchanged. snaphu is used by the time series processing only.

## 2026-10-25: a failed processing step stops the scene script

`sentinel/sentinel_scene_cpu.py` and `sentinel/sentinel_scene_multigpu.py` kept the return code of each `os.system` call but never checked it and always exited 0. When `sentinel_raw_process` crashed or ran out of memory, the calling workflow went on, merged empty SLC files and failed much later with a missing `.orbtiming` file, which hid the cause. Both scripts now exit with an error naming the failed command when the orbit state vectors, the empty SLC or the raw processing step fails. Successful runs are unchanged.

## Notes for running it

- The processor is normally built and run through hyp3-srg: its `scripts/build_proc.sh`, run inside this folder, builds the CPU programs (and the GPU ones with `USEGPU=true`), and its `back_projection` workflow calls `sentinel/sentinel_scene_cpu.py` and `util/merge_slcs.py` with `PROC_HOME` set to this folder. Stanford's own `build_proc` script expects a local FFTW build under `fft/` and the CUDA compiler.
- The CPU processor holds all three IW sub-swaths of a frame in memory before processing them, about 16.5 GB (estimate) for the frames checked below, whatever the DEM area. Plan on a 32 GB machine.
- Each run downloads the EGM2008 geoid grid into `DEM/`, so this folder must stay writable.

## Check

On 2026-10-25, on Linux x86_64 with conda-forge GCC and gfortran 16.2.0, FFTW 3.3.11 and Python 3.12, built and run through hyp3-srg v0.10.2 (`scripts/build_proc.sh` with `USEGPU=false`, then `python -m hyp3_srg ++process back_projection`), in work folders with 231-character paths:

- The whole CPU build, snaphu included, completes with no errors; the hyp3-srg tests pass (13 of 13).
- The hyp3-srg README example (the two Sentinel-1 frames of 2023-12-29, `--bounds -115.30 36.05 -115.05 36.25` over Las Vegas) no longer crashes on the long path: the processor reads the 366-character input name and decodes the frame. On a 14 GB machine it then runs out of memory holding the three sub-swaths, and the run now stops right there with `failed: $PROC_HOME/sentinel/sentinel_raw_process_cpu ...` instead of going on with empty images.
- To check the values within 14 GB, a test build that decodes and processes only sub-swath IW2 (the box lies wholly in IW2; a local test patch, not part of this copy) ran the same example and the next pass (2024-01-10) to the end on 4 cores: 3.6 minutes with the inputs already downloaded and 4.6 minutes including the 3.2 GB download, 8.5 GB peak. The two geocoded SLCs give the same comparison numbers, to every printed digit, as the same test build of the unchanged upstream code run in a short folder: against the official ARIA S1 GUNW for the pair, median phase difference 0.12 rad (mean resultant length 0.970); against interferograms from the official OPERA CSLC-S1 bursts, 0.25 rad; amplitude correlation with OPERA r = 0.84.
