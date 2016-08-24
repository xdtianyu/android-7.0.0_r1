CFLAGS=-O0 -g
# Force to use gcc to build it. Clang upstream bug is at
# https://llvm.org/bugs/show_bug.cgi?id=25416
CC=${CHOST}-gcc

BINS=iTLB_benchmark
OBJS=iTLB_benchmark.o iTLB_benchmark_function.o
OUTPUTS=$(BINS) $(OBJS) iTLB_benchmark_function.c

all: $(OUTPUTS)

iTLB_benchmark: iTLB_benchmark.o iTLB_benchmark_function.o

iTLB_benchmark_function.c: generateBenchmarkFunction.sh
	./generateBenchmarkFunction.sh 4096 1024 > iTLB_benchmark_function.c

clean:
	rm -rf $(OUTPUTS)
