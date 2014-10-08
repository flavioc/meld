
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

THREAD_PROGRAMS = simple32 8queens-13 belief-propagation-200 \
	new-heat-transfer-80 \
	greedy-graph-coloring-2000 min-max-tictactoe \
	shortest-5000-all shortest-usairports500

thread:
	@rm -f $(RESULTS_FILE)
	@for test in $(THREAD_PROGRAMS); do \
		bash threads_even.sh code/$$test.m th even 1 $(NPROCS) $(RUNS); \
	done

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
	rm -rf output
