CC=gcc

all: sim.o
	$(CC) sim.o -o sim
	
sim: sim_pipeline.c
	$(CC) sim_pipeline.c -c sim.o
	
clean:
	rm sim.o sim
