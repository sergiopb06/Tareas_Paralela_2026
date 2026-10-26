CC = gcc
CFLAGS = -Wall -pthread

TARGETS = smokers \
          generalized_smokers
		  
all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)

.PHONY: all clean