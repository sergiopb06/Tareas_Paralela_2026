CC = gcc
CFLAGS = -Wall -pthread

TARGETS = barbershop \
          barbershop_FIFO \
          barbershop_hilzers 


all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)

.PHONY: all clean