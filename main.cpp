#include<iostream>
#include<sstream>
#include<mpi.h>
#include<cstdlib>
#include<iomanip>
#include<ctime>

#include "subgrid.h"
#include "grid.h"


const int w = 10, h = 10;

int main(int argc, char** argv) {
    int psize, prank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &psize);
    MPI_Comm_rank(MPI_COMM_WORLD, &prank);
    // std::cout << prank << '/' << psize << std::endl;
    std::srand(std::time(0)+prank);

    for (float b = 0; b < 20.0; b+=0.05) {
        grid g(1000, 1000, 100000, 1.76, 0.);
        for (int i = 0; i < 1000; ++i) {
            g.round();
        }
        int mag = g.magnetization();
        if (prank == 0) {
            std::cout << b << ", " << mag << std::endl;
        }
    }
    MPI_Finalize();
    return 0;
}