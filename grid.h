#ifndef GRID
#define GRID
#include "subgrid.h"

struct grid {
    int psize;
    int prank;
    const int w, h;
    int* block_sizes;
    int* iter_number;

    int* byte_block_size;
    int* byte_block_displacement;

    subgrid* processed_subgrid;
    int* full_grid;

    grid(int w, int h, int total_iters, double j, double field);
    ~grid();

    void print();
    void round();
    void gather();
    int magnetization();
    #ifdef PNG
    void save_PNG(const char*);
    #endif
};
#endif