
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

TYPE = even
export RESULTS_FILE=$(PWD)/results.csv

GGC = greedy-graph-coloring-2000 greedy-graph-coloring-search_engines
PAGERANK = pagerank-search_engines pagerank-5000
HT = new-heat-transfer-80
HT_COORD = new-heat-transfer-80-coord
BP = belief-propagation-200
QUEENS = 8queens-13
SIMPLE = simple16 simple32 simple64
MINMAX = min-max-tictactoe-big
SHORTEST = shortest-usairports500 shortest-uspowergrid \
			  shortest-celegans shortest-1000-all \
			  shortest-oclinks
COORD_SHORTEST = shortest-usairports500-coord shortest-uspowergrid-coord \
					  shortest-celegans-coord shortest-1000-all-coord \
					  shortest-oclinks-coord

THREAD_PROGRAMS = $(QUEENS) $(GGC) $(PAGERANK) $(HT) $(MINMAX) $(SHORTEST)

SCRIPT = run_series.sh th $(TYPE) 1 $(NPROCS) $(RUNS)

thread:
	@bash $(SCRIPT) $(THREAD_PROGRAMS)

ggc:
	@bash $(SCRIPT) $(GGC)
pagerank:
	@bash $(SCRIPT) $(PAGERANK)
ht:
	@bash $(SCRIPT) $(HT)
coord-ht:
	@bash $(SCRIPT) $(HT_COORD)
simple:
	@bash $(SCRIPT) $(SIMPLE)
bp:
	@bash $(SCRIPT) $(BP)
queens:
	@bash $(SCRIPT) $(QUEENS)
minmax:
	@bash $(SCRIPT) $(MINMAX)
shortest:
	@bash $(SCRIPT) $(SHORTEST)
coord-shortest:
	@bash $(SCRIPT) $(COORD_SHORTEST)

compiled:
	@mkdir -p code
	@meld-compile-directory progs code
	@rm -f code/*.ast code/*.code

update-queens:
	python ../scripts/generate_8queens.py 9 v > progs/data/8queens-data9.meld
	python ../scripts/generate_8queens.py 10 v > progs/data/8queens-data10.meld
	python ../scripts/generate_8queens.py 11 v > progs/data/8queens-data11.meld
	python ../scripts/generate_8queens.py 12 v > progs/data/8queens-data12.meld
	python ../scripts/generate_8queens.py 13 v > progs/data/8queens-data13.meld
	touch progs/8queens-9.meld
	touch progs/8queens-10.meld
	touch progs/8queens-11.meld
	touch progs/8queens-12.meld
	touch progs/8queens-13.meld
	git add progs/data/8queens-data9.meld
	git add progs/data/8queens-data10.meld
	git add progs/data/8queens-data11.meld
	git add progs/data/8queens-data12.meld
	git add progs/data/8queens-data13.meld

update-quicksort:
	sed -e "s/down(@0, .*/down(@0, `python ../scripts/generate_list.py 70000`)./g" -i "" progs/quicksort.meld

plots:
	@bash ../scripts/plots/plot_instrumentation.sh instrumentation_files

clean:
	rm -f $(RESULTS_FILE)
	rm -rf output
