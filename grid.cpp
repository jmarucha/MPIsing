#include <mpi.h>
#include <iostream>

#ifdef PNG
#include <png++/png.hpp>
#endif

#include "grid.h"

#define ROOT 0

grid::grid(int w, int h, int total_iters, double j, double field):
w(w),h(h) {
    full_grid = new int[w*h];
    MPI_Comm_size(MPI_COMM_WORLD, &psize);
    MPI_Comm_rank(MPI_COMM_WORLD, &prank);

    block_sizes = new int[psize];
    iter_number = new int[psize];
    byte_block_size = new int[psize];
    byte_block_displacement = new int[psize];
    int displacement = 0; 
    for (int irank = 0; irank < psize; ++irank) {
        block_sizes[irank] = h/psize + (irank < h % psize ? 1 : 0);
        iter_number[irank] = block_sizes[irank]*total_iters/psize + (irank < h % psize ? 1 : 0);

        byte_block_size[irank] = block_sizes[irank]*w;
        byte_block_displacement[irank] = displacement;
        displacement += byte_block_size[irank];
    }
    processed_subgrid = new subgrid(w, block_sizes[prank], j, field);
}
grid::~grid() {
    delete[] block_sizes;
    delete[] iter_number;
    delete[] full_grid;
    delete[] byte_block_size;
    delete[] byte_block_displacement;
}

void grid::round() {
    for (int i = 0; i < iter_number[prank]; ++i) {
        processed_subgrid->tick();
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

void grid::gather() {
    MPI_Gatherv(
        processed_subgrid->grid,
        w*block_sizes[prank],
        MPI_INT,
        full_grid,
        byte_block_size,
        byte_block_displacement, MPI_INT, ROOT, MPI_COMM_WORLD);
}

void grid::print() {
    if (prank == ROOT) {
            for (int i = 0; i < h; ++i) {
                for (int j = 0; j < w; ++j) {
                    std::cout << (full_grid[j + w*i] ? "█" : " ");
                }
                std::cout << '\n';
            }
            std::cout << '\n';
        }
        std::cout << std::flush;
}

#ifdef PNG
void grid::save_PNG(const char * str) {
    if (prank != ROOT) return; 
    png::image<png::index_pixel_1> image(w, h);
    png::palette pal(2);
    pal[0] = png::color(0xDC, 0x6E, 0x53);
    pal[1] = png::color(0xCD, 0xE6, 0xD0);
    for (png::uint_32 y = 0; y < image.get_height(); ++y)
    {
        for (png::uint_32 x = 0; x < image.get_width(); ++x)
        {
            image[y][x] = full_grid[w*y+x];
            // non-checking equivalent of image.set_pixel(x, y, ...);
        }
    }
    image.set_palette(pal);
    image.write(str);
}
#endif