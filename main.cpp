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
    OPT_GROUP_PARAMS_PNG,
    OPT_GROUP_OTHER,
};

enum opt_codes {
    OPT_SAVE_PNG,
    OPT_PNG_OUTPUT_DIR,
    OPT_PNG_EVERY_NTH_ROUND,
    OPT_HELP
};

static struct argp_option options[] = {
  {"verbose", 'v', 0, 0, "Produce verbose output", OPT_GROUP_GENERAL},
  {"width", 'w', "W", 0, "Grid width", OPT_GROUP_GRID},
  {"height", 'h', "H", 0, "Grid height", OPT_GROUP_GRID},
  {"flips", 'f', "FLIPS", 0, "Flips per round", OPT_GROUP_ROUNDS},
  {"rounds", 'r', "ROUNDS", 0, "Total rounds", OPT_GROUP_ROUNDS},
  {"save-png", OPT_SAVE_PNG, 0, 0, "Save png every few rounds", OPT_GROUP_PARAMS_PNG},
  {"every-nth-round", OPT_SAVE_PNG, "N", 0, "Set PNG writing interval (in rounds)", OPT_GROUP_PARAMS_PNG},
  {"output-dir", OPT_PNG_OUTPUT_DIR, "DIR", 0, "PNGs output directory", OPT_GROUP_PARAMS_PNG},
  {"help", OPT_HELP, 0, 0, "Show this help message", OPT_GROUP_OTHER},
  {"usage", OPT_HELP, 0, OPTION_ALIAS, 0, OPT_GROUP_OTHER},
  {0}
};

typedef struct arguments
{
    int w, h;
    int rounds, flips;
    int verbose, save_png;
    char output_dir[256];
} arguments;


error_t set_or_default(int num, const char* arg, int* target) {
    if (num) {
        *target = num;
        return 0;
    } else {
        if (prank == 0) 
            fprintf(stderr, "Expected number, got '%s'.\n", arg);
        return ARGP_ERR_UNKNOWN;
    }
}

enum parse_errors {
    __ARGP_ERR_UNKNOWN = ARGP_ERR_UNKNOWN,
    HELP_MSG
};

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
        return set_or_default(num_arg, arg, &args->w);
    case 'h':
        return set_or_default(num_arg, arg, &args->h);
    case 'f':
        return set_or_default(num_arg, arg, &args->flips);
    case 'r':
        return set_or_default(num_arg, arg, &args->rounds);
    case OPT_SAVE_PNG:
        args->save_png = 1;
        break;
    case OPT_PNG_OUTPUT_DIR:
        strncpy(args->output_dir, arg, 256);
        break;
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
        rounds: 10000, flips: 100,
    };
    strcpy(args.output_dir, ".");
    
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

    // for (float b = 0; b < 20.0; b+=0.05) {
    float b = 0.5;
        grid g(args.w, args.h, args.flips, 1.76, 0.);
        for (int i = 0; i < args.rounds; ++i) {
            g.round();
            if (args.save_png) {
                g.gather();
                char filename[256+32];
                sprintf(filename, "%s/ROUND=%d.png", args.output_dir, i);
                g.save_png(filename);

            }
        }
        int mag = g.magnetization();
        if (prank == 0) {
            printf("%f, %d\n", b, mag);
        }
    // }
    MPI_Finalize();
    return 0;
}