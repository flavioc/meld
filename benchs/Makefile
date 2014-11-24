
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
BP = belief-propagation-400
SPLASH_BP = splash-bp-400
QUEENS = 8queens-13
SIMPLE = simple16 simple32 simple64
MINMAX = min-max-tictactoe-big
SHORTEST = shortest-usairports500 shortest-uspowergrid \
			  shortest-celegans shortest-1000 \
			  shortest-oclinks
COORD_SHORTEST = shortest-usairports500-coord shortest-uspowergrid-coord \
					  shortest-celegans-coord shortest-1000-coord \
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
splash-bp:
	@bash $(SCRIPT) $(SPLASH_BP)
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
	python ../scripts/generate_8queens.py 9 h > progs/data/8queens-data9.meld
	python ../scripts/generate_8queens.py 10 h > progs/data/8queens-data10.meld
	python ../scripts/generate_8queens.py 11 h > progs/data/8queens-data11.meld
	python ../scripts/generate_8queens.py 12 h > progs/data/8queens-data12.meld
	python ../scripts/generate_8queens.py 13 h > progs/data/8queens-data13.meld
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

update-ht:
	python ../scripts/generate_heat_grid.py 80 28 0.8 1.0 1.0 100.0 5.0 > progs/data/heat-transfer-80.meld
	git add progs/data/heat-transfer-80.meld
	touch progs/new-heat-transfer-80.meld
	python ../scripts/generate_heat_grid.py 120 42 0.8 1.0 1.0 100.0 5.0 > progs/data/heat-transfer-120.meld
	git add progs/data/heat-transfer-120.meld
	touch progs/new-heat-transfer-120.meld
	python ../scripts/generate_heat_grid.py 200 70 0.8 1.0 1.0 100.0 5.0 > progs/data/heat-transfer-200.meld
	git add progs/data/heat-transfer-200.meld
	touch progs/new-heat-transfer-200.meld

plots:
	@bash ../scripts/plots/plot_instrumentation.sh instrumentation_files

clean:
	rm -f $(RESULTS_FILE)
	rm -rf output
