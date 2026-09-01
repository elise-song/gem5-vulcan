CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -Werror
LDFLAGS ?= -static

.PHONY: all clean

all: sched-affinity

sched-affinity: sched-affinity.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	$(RM) sched-affinity
