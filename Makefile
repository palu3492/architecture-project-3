CC=gcc

all: sim_pipeline.o
	$(CC) sim_pipeline.o -o sim_pipeline
	
sim: sim_pipeline.c
	$(CC) sim_pipeline.c -c sim_pipeline.o
	
clean:
	rm sim_pipeline.o sim_pipeline
