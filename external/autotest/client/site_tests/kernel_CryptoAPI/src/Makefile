TARGET = crypto_load_mod
CFLAGS = -Wall -O2

all: $(TARGET)

%: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)

