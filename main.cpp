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

    double start_time, end_time;
    start_time = MPI_Wtime();

    grid g(1000, 1000, 10000, 1.76, 0.);
    for (int i = 0; i < 100; ++i) {
        std::stringstream name;
        name << "images/" << std::setw(5) << std::setfill('0');
        name << i << ".png";
        #ifdef PNG
        g.gather();
        g.save_PNG(name.str().c_str());
        #endif
        g.round();
    }

    end_time = MPI_Wtime();
    if (prank == 0) {
        std::cout << psize << ", " << end_time-start_time << std::endl;
    }
    MPI_Finalize();
    return 0;
}