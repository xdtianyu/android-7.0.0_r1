CFLAGS += -Wall
EXEC=resumption seccomp_bpf_tests sigsegv

all: $(EXEC)

clean:
	rm -f $(EXEC)

seccomp_bpf_tests: seccomp_bpf_tests.c test_harness.h
	$(CC) seccomp_bpf_tests.c -o seccomp_bpf_tests $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -pthread

resumption: resumption.c test_harness.h
	$(CC) $^ -o $@ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -ggdb3

sigsegv: sigsegv.c test_harness.h
	$(CC) $^ -o $@ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -ggdb3

run_tests: $(EXEC)
	./seccomp_bpf_tests
	./resumption
	./sigsegv

.PHONY: clean run_tests
