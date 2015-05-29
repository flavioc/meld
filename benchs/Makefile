
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

TYPE = even
export RESULTS_FILE=$(PWD)/results.csv

GGC = greedy-graph-coloring-2000 greedy-graph-coloring-search_engines
PAGERANK = pagerank-search_engines pagerank-5000
HT = new-heat-transfer-120 new-heat-transfer-120-coord0 new-heat-transfer-120-coord
BP = belief-propagation-400
SPLASH_BP = splash-bp-400
QUEENS = 8queens-14 8queens-14-top 8queens-14-top-static 8queens-14-bottom 8queens-14-bottom-static
SIMPLE = simple16 simple32 simple64
MINMAX = min-max-tictactoe
SEARCH_RANDOM = search-parallel-random search-threads-random
SEARCH_FACEBOOK = search-parallel-facebook search-threads-facebook
SEARCH_TWITTER = search-parallel-twitter search-threads-twitter
SEARCH_POKEC = search-parallel-pokec search-threads-pokec
SHORTEST_USAIRPORTS = shortest-usairports500 shortest-usairports500-coord
SHORTEST_OCLINKS = shortest-oclinks shortest-oclinks-coord
SHORTEST_USPOWERGRID = shortest-uspowergrid shortest-uspowergrid-coord
SHORTEST_EMAIL = shortest-email shortest-email-coord
SHORTEST = $(SHORTEST_EMAIL) \
			  shortest-twitter \
			  shortest-twitter-coord \
			  $(SHORTEST_USPOWERGRID)

THREAD_PROGRAMS = $(QUEENS) $(GGC) $(PAGERANK) $(HT) $(MINMAX) $(SHORTEST)

SCRIPT = run_series.sh th $(TYPE) 1 $(NPROCS) $(RUNS)
SERIAL = run_serial.sh $(RUNS)

thread:
	@bash $(SCRIPT) $(THREAD_PROGRAMS)

serial:
	@bash $(SERIAL) belief-propagation-50 belief-propagation-200 \
		belief-propagation-300 belief-propagation-400
	@bash $(SERIAL) new-heat-transfer-80 new-heat-transfer-120
	@bash $(SERIAL) min-max-tictactoe
	@bash $(SERIAL) 8queens-11 8queens-12 8queens-13 8queens-14
	@bash $(SERIAL) shortest-usairports500 shortest-oclinks \
		shortest-uspowergrid shortest-email
ggc:
	@bash $(SCRIPT) $(GGC)
search-random:
	@bash $(SCRIPT) $(SEARCH_RANDOM)
search-facebook:
	@bash $(SCRIPT) $(SEARCH_FACEBOOK)
search-twitter:
	@bash $(SCRIPT) $(SEARCH_TWITTER)
search-pokec:
	@bash $(SCRIPT) $(SEARCH_POKEC)
powergrid-50000:
	@bash $(SCRIPT) powergrid-50000 powergrid-50000-coord
powergrid-100000:
	@bash $(SCRIPT) powergrid-100000 powergrid-100000-coord
pagerank:
	@bash $(SCRIPT) $(PAGERANK)
ht:
	@bash $(SCRIPT) $(HT)
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
shortest-usairports:
	@bash $(SCRIPT) $(SHORTEST_USAIRPORTS)
shortest-oclinks:
	@bash $(SCRIPT) $(SHORTEST_OCLINKS)
shortest-uspowergrid:
	@bash $(SCRIPT) $(SHORTEST_USPOWERGRID)
shortest-email:
	@bash $(SCRIPT) $(SHORTEST_EMAIL)

run:
	@bash $(SCRIPT) $(TARGET)

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
	python ../scripts/generate_8queens.py 14 h > progs/data/8queens-data14.meld
	git add progs/data/8queens-data9.meld
	git add progs/data/8queens-data10.meld
	git add progs/data/8queens-data11.meld
	git add progs/data/8queens-data12.meld
	git add progs/data/8queens-data13.meld
	git add progs/data/8queens-data14.meld
	touch progs/8queens-9.meld
	touch progs/8queens-10.meld
	touch progs/8queens-11.meld
	touch progs/8queens-12.meld
	touch progs/8queens-13.meld
	touch progs/8queens-14.meld
	python ../scripts/generate_8queens.py 9 h b > progs/data/8queens-data9-bottom.meld
	python ../scripts/generate_8queens.py 10 h b > progs/data/8queens-data10-bottom.meld
	python ../scripts/generate_8queens.py 11 h b > progs/data/8queens-data11-bottom.meld
	python ../scripts/generate_8queens.py 12 h b > progs/data/8queens-data12-bottom.meld
	python ../scripts/generate_8queens.py 13 h b > progs/data/8queens-data13-bottom.meld
	python ../scripts/generate_8queens.py 14 h b > progs/data/8queens-data14-bottom.meld
	git add progs/data/8queens-data9-bottom.meld
	git add progs/data/8queens-data10-bottom.meld
	git add progs/data/8queens-data11-bottom.meld
	git add progs/data/8queens-data12-bottom.meld
	git add progs/data/8queens-data13-bottom.meld
	git add progs/data/8queens-data14-bottom.meld
	touch progs/8queens-9-bottom.meld
	touch progs/8queens-10-bottom.meld
	touch progs/8queens-11-bottom.meld
	touch progs/8queens-12-bottom.meld
	touch progs/8queens-13-bottom.meld
	touch progs/8queens-14-bottom.meld
	python ../scripts/generate_8queens.py 4 h bs > progs/data/8queens-data4-bottom-static.meld
	python ../scripts/generate_8queens.py 5 h bs > progs/data/8queens-data5-bottom-static.meld
	python ../scripts/generate_8queens.py 6 h bs > progs/data/8queens-data6-bottom-static.meld
	python ../scripts/generate_8queens.py 7 h bs > progs/data/8queens-data7-bottom-static.meld
	python ../scripts/generate_8queens.py 8 h bs > progs/data/8queens-data8-bottom-static.meld
	python ../scripts/generate_8queens.py 9 h bs > progs/data/8queens-data9-bottom-static.meld
	python ../scripts/generate_8queens.py 10 h bs > progs/data/8queens-data10-bottom-static.meld
	python ../scripts/generate_8queens.py 11 h bs > progs/data/8queens-data11-bottom-static.meld
	python ../scripts/generate_8queens.py 12 h bs > progs/data/8queens-data12-bottom-static.meld
	python ../scripts/generate_8queens.py 13 h bs > progs/data/8queens-data13-bottom-static.meld
	python ../scripts/generate_8queens.py 14 h bs > progs/data/8queens-data14-bottom-static.meld
	git add progs/data/8queens-data4-bottom-static.meld
	git add progs/data/8queens-data5-bottom-static.meld
	git add progs/data/8queens-data6-bottom-static.meld
	git add progs/data/8queens-data7-bottom-static.meld
	git add progs/data/8queens-data8-bottom-static.meld
	git add progs/data/8queens-data9-bottom-static.meld
	git add progs/data/8queens-data10-bottom-static.meld
	git add progs/data/8queens-data11-bottom-static.meld
	git add progs/data/8queens-data12-bottom-static.meld
	git add progs/data/8queens-data13-bottom-static.meld
	git add progs/data/8queens-data14-bottom-static.meld
	touch progs/8queens-4-bottom-static.meld
	touch progs/8queens-5-bottom-static.meld
	touch progs/8queens-6-bottom-static.meld
	touch progs/8queens-7-bottom-static.meld
	touch progs/8queens-8-bottom-static.meld
	touch progs/8queens-9-bottom-static.meld
	touch progs/8queens-10-bottom-static.meld
	touch progs/8queens-11-bottom-static.meld
	touch progs/8queens-12-bottom-static.meld
	touch progs/8queens-13-bottom-static.meld
	touch progs/8queens-13-bottom.meld
	python ../scripts/generate_8queens.py 9 h t > progs/data/8queens-data9-top.meld
	python ../scripts/generate_8queens.py 10 h t > progs/data/8queens-data10-top.meld
	python ../scripts/generate_8queens.py 11 h t > progs/data/8queens-data11-top.meld
	python ../scripts/generate_8queens.py 12 h t > progs/data/8queens-data12-top.meld
	python ../scripts/generate_8queens.py 13 h t > progs/data/8queens-data13-top.meld
	python ../scripts/generate_8queens.py 14 h t > progs/data/8queens-data14-top.meld
	git add progs/data/8queens-data9-top.meld
	git add progs/data/8queens-data10-top.meld
	git add progs/data/8queens-data11-top.meld
	git add progs/data/8queens-data12-top.meld
	git add progs/data/8queens-data13-top.meld
	git add progs/data/8queens-data14-top.meld
	touch progs/8queens-9-top.meld
	touch progs/8queens-10-top.meld
	touch progs/8queens-11-top.meld
	touch progs/8queens-12-top.meld
	touch progs/8queens-13-top.meld
	python ../scripts/generate_8queens.py 9 h ts > progs/data/8queens-data9-top-static.meld
	python ../scripts/generate_8queens.py 10 h ts > progs/data/8queens-data10-top-static.meld
	python ../scripts/generate_8queens.py 11 h ts > progs/data/8queens-data11-top-static.meld
	python ../scripts/generate_8queens.py 12 h ts > progs/data/8queens-data12-top-static.meld
	python ../scripts/generate_8queens.py 13 h ts > progs/data/8queens-data13-top-static.meld
	python ../scripts/generate_8queens.py 14 h ts > progs/data/8queens-data14-top-static.meld
	git add progs/data/8queens-data9-top-static.meld
	git add progs/data/8queens-data10-top-static.meld
	git add progs/data/8queens-data11-top-static.meld
	git add progs/data/8queens-data12-top-static.meld
	git add progs/data/8queens-data13-top-static.meld
	git add progs/data/8queens-data14-top-static.meld
	touch progs/8queens-9-top-static.meld
	touch progs/8queens-10-top-static.meld
	touch progs/8queens-11-top-static.meld
	touch progs/8queens-12-top-static.meld
	touch progs/8queens-13-top-static.meld

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

state_plots:
	@python ../scripts/plots/plot_state_statistics.py instrumentation_files state_statistics

generate-search-random:
	PYTHONPATH=../scripts python ../scripts/threads/generate_bfs.py 50000 20 100 5 > progs/data/search-random.meld
	touch progs/search-*-random.meld

generate-powergrid:
	PYTHONPATH=../scripts python ../scripts/generate_powergrid.py 2000 50000 1.008 > progs/data/powergrid-50000.meld
	PYTHONPATH=../scripts python ../scripts/generate_powergrid.py 2000 100000 1.004 > progs/data/powergrid-100000.meld

clean:
	rm -f $(RESULTS_FILE)
	rm -rf output
