CXX=mpic++
CC=mpic++
LD=${CXX}
# -Wall -Wextra -Werror -pedantic
CXXFLAGS+= -std=c++2a -O2 -DPNG
LDFLAGS+=-lm -lstdc++ $(CXXFLAGS) -lpng16

OBJS=main.o subgrid.o grid.o

.PHONY: all
all: MPIsing

MPIsing: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm *.o *~

run:
	mpiexec -n 16 ./MPIsing
debug:
	mpirun -n 2 xterm -e gdb ./MPIsing 
