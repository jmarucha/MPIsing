#include<iostream>
#include<sstream>
#include<mpi.h>
#include<cstdlib>
#include<iomanip>
#include<ctime>
#include <argp.h>

#include "subgrid.h"
#include "grid.h"

const char *argp_program_version =
  "MPIsing 1.0";

/* Program documentation. */
static char doc[] = "MPIsing - program for simulating Ising model on clusters";

enum opt_groups {
    GENERAL, GRID, ROUNDS, PARAMS
};

static struct argp_option options[] = {
  {"verbose", 'v', 0, GENERAL, "Produce verbose output" },
  {"width", 'w', "W", GRID, "Grid width"},
  {"height", 'h', "H", GRID, "Grid height"},
  {"flips", 'f', "FLIPS", ROUNDS, "Flips per round"},
  {"rounds", 'r', "ROUNDS", ROUNDS, "Total rounds"},
  {0}
};

typedef struct arguments
{
    int w, h;
    int rounds, flips;
    int verbose;
} arguments;



error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    arguments* args = (arguments*) state->input;
    int num_arg = 0;
    if (arg)
        num_arg = atoi(arg);
  switch (key)
    {
    case 'v':
        args->verbose = 1;
        break;
    case 'w':
        if (num_arg)
            args->w = num_arg;
        else {
            fprintf(stderr, "Not a valid number '%s', using default %d.\n", arg, args->w);
        }
        break;
    case 'h':
        if (num_arg)
            args->h = num_arg;
        else {
            fprintf(stderr, "Not a valid number '%s', using default %d.\n", arg, args->h);
        }
        break;
    case 'f':
        if (num_arg)
            args->flips = num_arg;
        else {
            fprintf(stderr, "Not a valid number '%s', using default %d.\n", arg, args->flips);
        }
        break;
    case 'r':
        if (num_arg)
            args->rounds = num_arg;
        else {
            fprintf(stderr, "Not a valid number '%s', using default %d.\n", arg, args->rounds);
        }
        break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

int main(int argc, char** argv) {
    error_t err;
    arguments args = {/* w & h */ 1000, 1000,
         /* rounds, flips per round */ 10000, 100};
    argp_parse (&argp, argc, argv, 0, 0, (void*) &args);
    int psize, prank;
    
    err = MPI_Init(&argc, &argv);
    if (err) {
        fputs("Cannot init MPI - try mpirun -n 16 MPIsing", stderr);
        exit(1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &psize);
    MPI_Comm_rank(MPI_COMM_WORLD, &prank);
    if (args.verbose)
        fprintf(stderr, "Running process %d of %d\n", prank, psize);
    std::srand(std::time(0)+prank);

    for (float b = 0; b < 20.0; b+=0.05) {
        grid g(args.w, args.h, args.flips, 1.76, 0.);
        for (int i = 0; i < args.rounds; ++i) {
            g.round();
        }
        int mag = g.magnetization();
        if (prank == 0) {
            printf("%f, %d\n", b, mag);
        }
    }
    MPI_Finalize();
    return 0;
}