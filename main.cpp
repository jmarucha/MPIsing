#include<iostream>
#include<sstream>
#include<mpi.h>
#include<cstdlib>
#include<iomanip>
#include<ctime>
#include <argp.h>

#include "subgrid.h"
#include "grid.h"
#include "errors.h"

const char *argp_program_version =
  "MPIsing 1.0";

/* Program documentation. */
static char doc[] = "MPIsing - program for simulating Ising model on clusters";


int psize, prank;

enum opt_groups {
    OPT_GROUP_GENERAL,
    OPT_GROUP_GRID,
    OPT_GROUP_ROUNDS,
    OPT_GROUP_PARAMS,
    OPT_GROUP_PNG,
    OPT_GROUP_OTHER,
};

enum opt_codes {
    OPT_SAVE_PNG,
    OPT_PNG_OUTPUT_DIR,
    OPT_PNG_EVERY_NTH_ROUND,
    OPT_HELP,
    OPT_J = 'j',
    OPT_MU = 'm'
};

static struct argp_option options[] = {
  {"verbose", 'v', 0, 0, "Produce verbose output", OPT_GROUP_GENERAL},
  {"width", 'w', "INT", 0, "Grid width", OPT_GROUP_GRID},
  {"height", 'h', "INT", 0, "Grid height", OPT_GROUP_GRID},
  {"flips", 'f', "INT", 0, "Flips per round", OPT_GROUP_ROUNDS},
  {"rounds", 'r', "INT", 0, "Total rounds", OPT_GROUP_ROUNDS},
  {"save-png", OPT_SAVE_PNG, 0, 0, "Save png every few rounds", OPT_GROUP_PNG},
  {"every-nth-round", OPT_PNG_EVERY_NTH_ROUND, "INT", 0, "Set PNG writing interval (in rounds)", OPT_GROUP_PNG},
  {"output-dir", OPT_PNG_OUTPUT_DIR, "DIR", 0, "PNGs output directory", OPT_GROUP_PNG},
  {"help", OPT_HELP, 0, 0, "Show this help message", OPT_GROUP_OTHER},
  {"usage", OPT_HELP, 0, OPTION_ALIAS, 0, OPT_GROUP_OTHER},
  {"j", OPT_J, "FLOAT", 0, "Spin-spin coupling j", OPT_GROUP_PARAMS},
  {"mu", OPT_MU, "FLOAT", 0, "External magnetic field mu", OPT_GROUP_PARAMS},
  {0}
};

typedef struct arguments
{
    long w, h;
    double j, mu;
    long rounds, flips;
    int verbose, save_png;
    char output_dir[256];
    long every_nth_round;
} arguments;


error_t try_seti(const char* arg, long int* target) {
    if (arg) {
        char* end_ptr = NULL;
        long res = strtol(arg, &end_ptr, 10);
        if (*end_ptr == '\0') {
            *target = res;
            return 0;
        }
    }
    if (prank == 0) 
        fprintf(stderr, "Expected number, got '%s'.\n", arg);
    return ARGP_ERR_UNKNOWN;
}


error_t try_setd(const char* arg, double* target) {
    if (arg) {
        char* end_ptr = NULL;
        double res = strtod(arg, &end_ptr);
        if (*end_ptr == '\0') {
            *target = res;
            return 0;
        }
    }
    if (prank == 0) 
        fprintf(stderr, "Expected number, got '%s'.\n", arg);
    return ARGP_ERR_UNKNOWN;
}

enum parse_errors {
    __ARGP_ERR_UNKNOWN = ARGP_ERR_UNKNOWN,
    HELP_MSG
};

error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    arguments* args = (arguments*) state->input;
  switch (key)
    {
    case 'v':
        args->verbose = 1;
        break;
    case 'w':
        return try_seti(arg, &args->w);
    case 'h':
        return try_seti(arg, &args->h);
    case 'f':
        return try_seti(arg, &args->flips);
    case 'r':
        return try_seti(arg, &args->rounds);
    case OPT_J:
        return try_setd(arg, &args->j);
    case OPT_MU:
        return try_setd(arg, &args->mu);
    case OPT_SAVE_PNG:
        args->save_png = 1;
        break;
    case OPT_PNG_OUTPUT_DIR:
        strncpy(args->output_dir, arg, 256);
        break;
    case OPT_PNG_EVERY_NTH_ROUND:
        return try_seti(arg, &args->every_nth_round);
    case OPT_HELP:
        return HELP_MSG;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

int main(int argc, char** argv) {
    error_t err;
    arguments args = {
        w: 1000, h: 1000,
        j: 1.76, mu: 0.0,
        rounds: 10000, flips: 100,
        output_dir: ".",
        every_nth_round: 1,
    };

    err = MPI_Init(&argc, &argv);
    if (err) {
        fputs("Cannot initialize MPI.\n", stderr);
        exit(MPISING_ERR_MPI_INIT);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &psize);
    MPI_Comm_rank(MPI_COMM_WORLD, &prank);

    err = argp_parse (&argp, argc, argv, ARGP_SILENT, 0, (void*) &args);
    if (err) {
        if (prank == 0) {
            if (err == HELP_MSG) {
                argp_help(&argp, stdout, ARGP_HELP_STD_HELP, argv[0]);
            } else {
                argp_help(&argp, stderr, ARGP_HELP_STD_ERR, argv[0]);
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
        return (err == HELP_MSG) ? 0 : MPISING_ERR_ARG_PARSE;
    }

    if (args.verbose)
        fprintf(stderr, "Running process %d of %d\n", prank, psize);
    std::srand(std::time(0)+prank);

    grid g(args.w, args.h, args.flips, args.j, args.mu);
    for (int i = 0; i < args.rounds; ++i) {
        g.round();
        if (args.save_png && (i % args.every_nth_round == 0)) {
            g.gather();
            char filename[256+32];
            sprintf(filename, "%s/ROUND=%d.png", args.output_dir, i);
            g.save_png(filename);
        }
    }
    int mag = g.magnetization();
    if (prank == 0) {
        printf("%f, %d\n", args.mu, mag);
    }

    MPI_Finalize();
    return 0;
}