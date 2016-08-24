EXEC=ftrace-clock-monotonic

all: $(EXEC)

clean:
	rm -f $(EXEC)

$(EXEC): ftrace-clock-monotonic.c
	$(CC) $^ -o $@ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

.PHONY: clean
