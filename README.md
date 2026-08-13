# MPIsing

This was a coursework, but it might be useful as MPI test suite, providing meaningful workload.

## Mathematical model

TBD

## Usage

Via mpirun: `mpirun -n [number of processes] MPIsing [OPTION...]`

```
  -f, --flips=FLIPS          Flips per round
  -h, --height=H             Grid height
  -r, --rounds=ROUNDS        Total rounds
  -v, --verbose              Produce verbose output
  -w, --width=W              Grid width
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version
```

Or slurm, or whatever your cluster uses.

## Requirements

This requires libpng++, and some form of MPI. For building on Mint or Ubuntu

```bash
apt install libpng++dev
apt install mpich libmpich-dev  # form mpich
apt install openmpi-bin libopenmpi-dev
```

Then,

```bash
make
```