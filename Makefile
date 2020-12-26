CC = g++
#OPT = -O3 -m32
OPT = -g -m32
WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB)

# List all your .cc files here (source files, excluding header files)
SIM_SRC = cache.cc main.cc
#cache.cc cache.h b_pred.cc b_pred.h
# List corresponding compiled object files here (.o files)
SIM_OBJ = cache.o main.o
#cache.o b_pred.o
#################################

# default rule

all: sim
	@echo "my work is done here..."


# rule for making sim_cache

sim: $(SIM_OBJ)
	$(CC) -g -o sim $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH SIM_CACHE-----------"


# generic rule for converting any .cc file to any .o file
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc


# type "make clean" to remove all .o files plus the sim_cache binary

clean:
	rm -f *.o sim


# type "make clobber" to remove all .o files (leaves sim_cache binary)

clobber:
	rm -f *.o


