CFLAGS=-O0 -g

BINS=iTLB_benchmark dTLB_benchmark noploop
OBJS=iTLB_benchmark.o dTLB_benchmark.o iTLB_benchmark_function.o
OUTPUTS=$(BINS) $(OBJS) iTLB_benchmark_function.c noploop_branch.txt

all: $(OUTPUTS)

noploop: noploop.c

noploop_branch.txt: noploop
	./find_loop_instructions.py $< > $@

iTLB_benchmark: iTLB_benchmark.o iTLB_benchmark_function.o

dTLB_benchmark: dTLB_benchmark.o

iTLB_benchmark_function.c: generateBenchmarkFunction.sh
	./generateBenchmarkFunction.sh > iTLB_benchmark_function.c

clean:
	rm -rf $(OUTPUTS)
