CC = gcc
CFLAGS = -Wall -pthread

TARGETS = readers-writers \
          readers-writers_no-starve \
          readers-writers_writer-priority \
		  no-starve_mutex

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)

.PHONY: all clean