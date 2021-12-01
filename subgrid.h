#ifndef SUBGRID
#define SUBGRID
#include <mpi.h>
typedef int PRANK;

struct subgrid {
    const int w;
    const int h;
    double j;
    double field;
    int* grid;
    int* ghost_top;
    int* ghost_bottom;
    int& point(int x, int y);
    void tick();
    void conclude();
    subgrid(int w, int h, double j, double field);
    ~subgrid();

    MPI_Request* bottom_send;
    MPI_Request* bottom_receive;

    MPI_Request* top_send;
    MPI_Request* top_receive;

    struct  {
        MPI_Request list[4];
        inline int n() {return 4;}
        inline MPI_Request* top_send() {return list+0;}
        inline MPI_Request* bot_send() {return list+1;}
        inline MPI_Request* top_recv() {return list+2;}
        inline MPI_Request* bot_recv() {return list+3;}
    } requests;

    int psize;
    PRANK prank;
    PRANK prev;
    PRANK next;
    int& left_neigh(int x, int y);
    int& right_neigh(int x, int y);
    int& top_neigh(int x, int y);
    int& bottom_neigh(int x, int y);
};
#endif