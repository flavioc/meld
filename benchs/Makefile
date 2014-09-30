
RUNS = 1
NPROCS = 1
OS = $(shell uname -s)

ifeq ($(OS),Linux)
  NPROCS := $(shell grep -c ^processor /proc/cpuinfo)
endif
ifeq ($(OS),Darwin)
  NPROCS := $(shell sysctl hw.ncpu | awk '{print $$2}')
endif # $(OS)

export RESULTS_FILE=$(PWD)/results.csv

all: compiled

run:
	@bash run.sh

serial:
	@for file in `ls code/*.m`; do \
		bash serial.sh $$file $(RUNS); \
	done

thread:
	@rm -f $(RESULTS_FILE)
	@bash threads_even.sh code/simple32.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/8queens-13.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/belief-propagation-400.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/greedy-graph-coloring-2000.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/min-max-tictactoe.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-5000-all.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-usairports500.m th even 1 $(NPROCS) $(RUNS)

coord-shortest:
	@rm -f $(RESULTS_FILE)
	@bash threads_even.sh code/shortest-1000.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-1000.m thp even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-1000-all.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-1000-all.m thp even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-celegans.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-celegans.m thp even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-cross-parker.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-cross-parker.m thp even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-oclinks.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-oclinks.m thp even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-usairports500.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-usairports500.m thp even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-uspowergrid.m th even 1 $(NPROCS) $(RUNS)
	@bash threads_even.sh code/shortest-uspowergrid.m thp even 1 $(NPROCS) $(RUNS)

compiled:
	@mkdir -p code
	@meld-compile-directory progs code
	@rm -f code/*.ast code/*.code

clean:
	rm -f $(RESULTS_FILE)
