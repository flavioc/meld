
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

TYPE = even
export RESULTS_FILE=$(PWD)/results.csv

GGC = greedy-graph-coloring-2000 greedy-graph-coloring-search_engines
PAGERANK = pagerank-search_engines pagerank-5000
HT = new-heat-transfer-80
SHORTEST = shortest-5000-all shortest-usairports500
BP = belief-propagation-200
QUEENS = 8queens-13 8queens-12
SIMPLE = simple16 simple32 simple64
MINMAX = min-max-tictactoe

THREAD_PROGRAMS = $(SIMPLE) $(QUEENS) $(BP) \
	$(GGC) $(PAGERANK) $(HT) $(MINMAX) $(SHORTEST)

SCRIPT = run_series.sh th $(TYPE) 1 $(NPROCS) $(RUNS)

thread:
	@bash $(SCRIPT) $(THREAD_PROGRAMS)

ggc:
	@bash $(SCRIPT) $(GGC)
pagerank:
	@bash $(SCRIPT) $(PAGERANK)
ht:
	@bash $(SCRIPT) $(HT)
simple:
	@bash $(SCRIPT) $(SIMPLE)
bp:
	@bash $(SCRIPT) $(BP)
queens:
	@bash $(SCRIPT) $(QUEENS)
minmax:
	@bash $(SCRIPT) $(MINMAX)

coord-shortest:
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
