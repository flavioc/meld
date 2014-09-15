
RUNS = 1
NPROCS = 1
OS = $(shell uname -s)

ifeq ($(OS),Linux)
  NPROCS := $(shell grep -c ^processor /proc/cpuinfo)
endif
ifeq ($(OS),Darwin)
  NPROCS := $(shell sysctl hw.ncpu | awk '{print $$2}')
endif # $(OS)

all: compiled

run:
	@bash run.sh

serial:
	@for file in `ls code/*.m`; do \
		bash serial.sh $$file $(RUNS); \
	done

thread:
	@bash threads_even.sh code/simple.m th even 1 $(NPROCS) $(RUNS)

compiled:
	@mkdir -p code
	@meld-compile-directory progs code
	@rm -f code/*.ast code/*.code

clean:
	rm -rf code
