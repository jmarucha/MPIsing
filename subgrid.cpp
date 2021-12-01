#include "subgrid.h"
#include <cstdlib>
#include <mpi.h>
#include <cmath>
#include <iostream>

subgrid::subgrid(int w, int h, double j, double field):
w(w),h(h),j(j),field(field) {
    grid = new int[w*h];
    ghost_top = new int[w];
    ghost_bottom = new int[w];
    for (int i = 0; i < w*h; ++i) {
        grid[i] = std::rand()%2;
    }
    MPI_Comm_size(MPI_COMM_WORLD, &psize);
    MPI_Comm_rank(MPI_COMM_WORLD, &prank);

    // communication setup 
    // initial synchronization of ghosts
    next = (prank+1)%psize;
    prev = (prank+psize-1)%psize;
    MPI_Send_init(grid, w, MPI_INT, prev, 0, MPI_COMM_WORLD, requests.top_send());
    MPI_Send_init(grid+w*(h-1), w, MPI_INT, next, 0, MPI_COMM_WORLD, requests.bot_send());

    MPI_Recv_init(ghost_top, w, MPI_INT, prev, 0, MPI_COMM_WORLD, requests.top_recv());
    MPI_Recv_init(ghost_bottom, w, MPI_INT, next, 0, MPI_COMM_WORLD, requests.bot_recv());

    MPI_Startall(requests.n(), requests.list);
    MPI_Waitall(requests.n(), requests.list, MPI_STATUS_IGNORE);
}
subgrid::~subgrid() {
    conclude();
    delete[] grid;
    delete[] ghost_top;
    delete[] ghost_bottom;
    for (int i = 0; i < requests.n(); ++i)
        MPI_Request_free(requests.list + i);
}

void subgrid::tick() {
    bool shall_flip = false;
    int random_x = rand()%w;
    int random_y = rand()%h;
    // std::cout << "X: " << random_x << " Y: " << random_y << '\n';

    int& p = point(random_x, random_y);
    int& left = point(random_x-1, random_y);
    int& right = point(random_x+1, random_y);
    int& top = point(random_x, random_y-1);
    int& bottom = point(random_x, random_y+1);
    //array takes value of 0 and 1, not -1/2 1/2,
    //so offset -1/2 of 1/2 per spin must be added
    double neighbour_sum = (double)(left+right+top+bottom)-2.;
    // std::cout << neighbour_sum << '\n';
    double old_value = (double)p - .5;
    double new_value = (double)(!p) - .5;

    double old_energy = -neighbour_sum*old_value*j + old_value*field;
    double new_energy = -neighbour_sum*new_value*j + new_value*field;

    // std::cout << old_energy << " --> " << new_energy << '\n';
    if (new_energy < old_energy) shall_flip = true;
    else {
        double prob = std::exp(old_energy-new_energy);
        // std::cout << "prob of transition: " << prob << '\n';
        double random_number = (double)rand() / RAND_MAX;
        if (random_number < prob) shall_flip = true;
    }

    if (shall_flip) {
        p = !p;
        //send top ghost
        if (random_y == 0) {
            int operation_completed;
            MPI_Test(requests.top_send(), &operation_completed, MPI_STATUS_IGNORE);
            if (!operation_completed) {
                MPI_Cancel(requests.top_send());
                MPI_Wait(requests.top_send(), MPI_STATUS_IGNORE);
            }
            MPI_Start(requests.top_send());
        }
        //send bottom ghost
        if (random_y == h-1) {
            int operation_completed;
            MPI_Test(requests.bot_send(), &operation_completed, MPI_STATUS_IGNORE);
            if (!operation_completed) {
                MPI_Cancel(requests.bot_send());
                MPI_Wait(requests.bot_send(), MPI_STATUS_IGNORE);
            }
            MPI_Start(requests.bot_send());
        }
    }

    // update ghosts
    int update_ghost;
    MPI_Iprobe(prev, 0, MPI_COMM_WORLD, &update_ghost, MPI_STATUS_IGNORE);
    if (update_ghost) {
        MPI_Start(requests.top_recv());
        MPI_Wait(requests.top_recv(), MPI_STATUS_IGNORE);
    }
    MPI_Iprobe(next, 0, MPI_COMM_WORLD, &update_ghost, MPI_STATUS_IGNORE);
    if (update_ghost) {
        MPI_Start(requests.bot_recv());
        MPI_Wait(requests.bot_recv(), MPI_STATUS_IGNORE);
    }
}


int& subgrid::point(int x, int y) {
    if (x == -1) return grid[w*y + w -1]; //left PBC;
    if (x == w) return grid[w*y]; //right PBC;
    if (psize > 1) { //multiprocessing
        if (y == -1) return ghost_top[x]; //ghost_line;
        if (y == h) return ghost_bottom[x]; //ghost_line;
    } else {
        if (y == -1) return grid[w*(h-1)+x]; //top PBC;
        if (y == h) return grid[x]; //bottom PBC;
    }
    return grid[w*y+x];
}

void subgrid::conclude() {
    if (psize == 1) return;
    int update_ghost;
    MPI_Iprobe(prev, 0, MPI_COMM_WORLD, &update_ghost, MPI_STATUS_IGNORE);
    if (update_ghost) {
        MPI_Start(requests.top_recv());
        MPI_Wait(requests.top_recv(), MPI_STATUS_IGNORE);
    }
    MPI_Iprobe(next, 0, MPI_COMM_WORLD, &update_ghost, MPI_STATUS_IGNORE);
    if (update_ghost) {
        MPI_Start(requests.bot_recv());
        MPI_Wait(requests.bot_recv(), MPI_STATUS_IGNORE);
    }
    int operation_completed;
    //cancel top ghost
    MPI_Test(requests.top_send(), &operation_completed, MPI_STATUS_IGNORE);
    if (!operation_completed) {
        MPI_Cancel(requests.top_send());
        MPI_Wait(requests.top_send(), MPI_STATUS_IGNORE);
    }
    //cancel bottom ghost
    MPI_Test(requests.bot_send(), &operation_completed, MPI_STATUS_IGNORE);
    if (!operation_completed) {
        MPI_Cancel(requests.bot_send());
        MPI_Wait(requests.bot_send(), MPI_STATUS_IGNORE);
    }
}