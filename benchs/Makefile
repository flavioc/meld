
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

HT = new-heat-transfer-120 new-heat-transfer-120-coord0 new-heat-transfer-120-coord

BP = belief-propagation-400
SPLASH_BP = splash-bp-400

HT = new-heat-transfer-80 new-heat-transfer-120
HT_COORD = $(addsuffix -coord,$(HT)) $(addsuffix -coord0,$(HT))

SHORTEST = shortest-livejournal \
			  shortest-usairports500 \
			  shortest-twitter \
			  shortest-email \
			  shortest-oclinks \
			  shortest-uspowergrid \
			  shortest-orkut
SHORTEST_COORD = $(addsuffix -coord,$(SHORTEST))

MINMAX = min-max-tictactoe min-max-tictactoe-big
MINMAX_COORD = $(addsuffix -coord,$(MINMAX))

BP = belief-propagation-300 belief-propagation-400
BP_COORD = $(addsuffix -coord,$(BP))

SCRIPT = run_series.sh th $(TYPE) 1 $(NPROCS) $(RUNS)
SERIAL = run_serial.sh $(RUNS)

SHORTEST_INDEXING_PROGS = shortest-twitter shortest-usairports500 shortest-oclinks shortest-email

INDEXING_PROGS = belief-propagation-50 belief-propagation-200 \
					  belief-propagation-300 belief-propagation-400 \
					  $(HT) \
					  $(MINMAX) 8queens-11 8queens-12 8queens-13 8queens-14
SERIAL_PROGS = $(INDEXING_PROGS)

SCALE_PROGS = belief-propagation-300 belief-propagation-400 \
				  $(SHORTEST) \
				  greedy-graph-coloring-gplus \
				  greedy-graph-coloring-twitter \
				  8queens-13 8queens-14 \
				  $(MINMAX) min-max-tictactoe min-max-tictactoe-big \
				  pagerank-pokec pagerank-gplus $(HT)
COORD_PROGS = $(SHORTEST_COORD)

serial:
	@bash $(SERIAL) $(SERIAL_PROGS)
serial-mem:
	@bash run_mem.sh $(SERIAL_PROGS)
no-indexing-serial:
	@bash $(SERIAL) $(INDEXING_PROGS)
sssp-no-indexing-mem:
	@bash run_mem.sh $(SHORTEST_INDEXING_PROGS)
no-indexing-serial-mem:
	@bash run_mem.sh $(INDEXING_PROGS)
serial-recompile:
	touch $(addprefix progs/,$(addsuffix .meld,$(SERIAL_PROGS)))

scale:
	@bash $(SCRIPT) $(SCALE_PROGS)
scale-recompile:
	touch $(addprefix progs/,$(addsuffix .meld,$(SCALE_PROGS)))

sssp-stat:
	@bash run_stats.sh 1 $(SHORTEST)
sssp-recompile:
	touch $(addprefix progs/,$(addsuffix .meld,$(SHORTEST)))
sssp:
	@bash $(SCRIPT) $(SHORTEST)
sssp-coord:
	@bash $(SCRIPT) $(SHORTEST_COORD)
sssp-coord-recompile:
	touch $(addprefix progs/,$(addsuffix .meld,$(SHORTEST_COORD)))
sssp-ppopp:
	@bash $(SCRIPT) shortest-uspowergrid shortest-orkut shortest-livejournal
sssp-ppopp-coord:
	@bash $(SCRIPT) shortest-uspowergrid-coord shortest-orkut-coord shortest-livejournal-coord
sssp-ppopp-compile: progs/data/weighted-livejournal.txt progs/data/orkut.txt
	./compile.sh progs/shortest-uspowergrid.meld
	./compile.sh progs/shortest-uspowergrid-coord.meld
	./compile.sh progs/shortest-orkut.meld
	./compile.sh progs/shortest-orkut-coord.meld
	./compile.sh progs/shortest-livejournal.meld
	./compile.sh progs/shortest-livejournal-coord.meld

ht-ppopp-compile:
	./compile.sh progs/new-heat-transfer-120.meld
	./compile.sh progs/new-heat-transfer-120-coord.meld
	./compile.sh progs/new-heat-transfer-120-coord0.meld
ht-ppopp:
	@bash $(SCRIPT) new-heat-transfer-120
ht-ppopp-coord:
	@bash $(SCRIPT) new-heat-transfer-120-coord0 \
		new-heat-transfer-120-coord

queens-ppopp-compile:
	./compile.sh progs/8queens-14.meld
	./compile.sh progs/8queens-14-bottom-static.meld
	./compile.sh progs/8queens-13.meld
	./compile.sh progs/8queens-13-bottom-static.meld
queens-ppopp:
	@bash $(SCRIPT) 8queens-14 8queens-14-bottom-static \
		8queens-13 8queens-13-bottom-static

bp-ppopp-compile:
	./compile.sh progs/belief-propagation-400.meld
	./compile.sh progs/splash-bp-400.meld

bp-ppopp:
	@bash $(SCRIPT) belief-propagation-400 splash-bp-400

minmax-coord:
	@bash $(SCRIPT) $(MINMAX_COORD)
minmax-mem:
	@bash run_mems.sh $(MINMAX)
minmax-coord-mem:
	@bash run_mems.sh $(MINMAX_COORD)
minmax-ppopp-compile:
	./compile.sh progs/min-max-tictactoe-big.meld
	./compile.sh progs/min-max-tictactoe-big-coord.meld
minmax-ppopp:
	@bash $(SCRIPT) min-max-tictactoe-big
minmax-ppopp-coord:
	@bash $(SCRIPT) min-max-tictactoe-big-coord

ht-coord:
	@bash $(SCRIPT) $(HT_COORD)

bp-coord:
	@bash $(SCRIPT) $(BP_COORD)

splash:
	@bash $(SCRIPT) $(SPLASH_BP)

QUEENS_COORD = 8queens-13-top 8queens-13-top-static 8queens-13-bottom 8queens-13-bottom-static 8queens-14-top 8queens-14-top-static 8queens-14-bottom 8queens-14-bottom-static

queens:
	@bash $(SCRIPT) 8queens-13 8queens-14
queens-coord:
	@bash $(SCRIPT) $(QUEENS_COORD)

SEARCH = search-pokec search-pokec-threads \
			search-facebook search-facebook-threads \
			search-twitter search-twitter-threads \
			search-random search-random-threads

search-scale:
	@bash $(SCRIPT) $(SEARCH)
search-stat:
	@bash run_stats.sh 1 $(SEARCH)

POWERGRID = powergrid-500000-64 powergrid-500000-64-coord \
powergrid-1000000 powergrid-1000000-coord \
				powergrid-1000000-2 powergrid-1000000-2-coord \
				powergrid-50000 powergrid-50000-coord \
				powergrid-500000 powergrid-500000-coord

powergrid-recompile:
	touch $(addprefix progs/,$(addsuffix .meld, $(POWERGRID)))
powergrid:
	@bash $(SCRIPT) $(POWERGRID)
powergrid-stats:
	@bash run_stats.sh 32 $(POWERGRID)

KEYVALUE = key-value-all-19-ten key-value-19-ten \
			  key-value-all-21-ten key-value-21-ten

keyvalue:
	@bash $(SCRIPT) $(KEYVALUE)
keyvalue-stats:
	@bash run_stats.sh 32 $(KEYVALUE)
	
ht:
	@bash $(SCRIPT) $(HT)
simple:
	@bash $(SCRIPT) $(SIMPLE)
bp:
	@bash $(SCRIPT) $(BP)
splash-bp:
	@bash $(SCRIPT) $(SPLASH_BP)
minmax:
	@bash $(SCRIPT) $(MINMAX)

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
	python ../scripts/generate_8queens.py 15 h > progs/data/8queens-data15.meld
	git add progs/data/8queens-data9.meld
	git add progs/data/8queens-data10.meld
	git add progs/data/8queens-data11.meld
	git add progs/data/8queens-data12.meld
	git add progs/data/8queens-data13.meld
	git add progs/data/8queens-data14.meld
	git add progs/data/8queens-data15.meld
	touch progs/8queens-9.meld
	touch progs/8queens-10.meld
	touch progs/8queens-11.meld
	touch progs/8queens-12.meld
	touch progs/8queens-13.meld
	touch progs/8queens-14.meld
	touch progs/8queens-15.meld
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
	PYTHONPATH=../scripts python ../scripts/generate_powergrid.py 64 500000 1.002 20000 1.0 > progs/data/powergrid-500000-64.meld
	PYTHONPATH=../scripts python ../scripts/generate_powergrid.py 2000 50000 1.02 500 1.0 > progs/data/powergrid-50000.meld
	PYTHONPATH=../scripts python ../scripts/generate_powergrid.py 2000 500000 1.002 500 1.0 > progs/data/powergrid-500000.meld
	PYTHONPATH=../scripts python ../scripts/generate_powergrid.py 4000 1000000 1.002 500 0.5 > progs/data/powergrid-1000000.meld
	PYTHONPATH=../scripts python ../scripts/generate_powergrid.py 4000 1000000 1.002 500 2.0 > progs/data/powergrid-1000000-2.meld

generate-key-value:
	PYTHONPATH=../scripts python ../scripts/threads/generate_tree.py 19 2000000 0.1 > progs/data/key-value-19-ten.meld
	PYTHONPATH=../scripts python ../scripts/threads/generate_tree.py 19 2000000 0.2 > progs/data/key-value-19-twenty.meld
	PYTHONPATH=../scripts python ../scripts/threads/generate_tree.py 21 5000000 0.1 > progs/data/key-value-21-ten.meld
	PYTHONPATH=../scripts python ../scripts/threads/generate_tree.py 21 5000000 0.2 > progs/data/key-value-21-twenty.meld

progs/data/gplus.txt:
	wget -c http://snap.stanford.edu/data/gplus_combined.txt.gz
	gzip -d gplus_combined.txt.gz
	python ../scripts/uniform_snap.py gplus_combined.txt > progs/data/gplus.txt
progs/data/weighted-livejournal.txt:
	curl -o soc-LiveJournal1.txt.gz http://snap.stanford.edu/data/soc-LiveJournal1.txt.gz
	gzip -d soc-LiveJournal1.txt.gz
	python ../scripts/uniform_snap.py soc-LiveJournal1.txt > progs/data/shortest-livejournal1.txt
	rm -f soc-LiveJournal1.txt
	python ../scripts/weighted_snap.py progs/data/shortest-livejournal1.txt > progs/data/weighted-livejournal.txt
	rm -f shortest-livejournal1.txt

progs/data/pokec.txt:
	wget -c http://snap.stanford.edu/data/soc-pokec-relationships.txt.gz
	gzip -d soc-pokec-relationships.txt.gz
	python ../scripts/uniform_snap.py soc-pokec-relationships.txt > progs/data/pokec.txt
progs/data/orkut.txt:
	curl -o orkut.txt.gz http://snap.stanford.edu/data/bigdata/communities/com-orkut.ungraph.txt.gz
	gzip -d orkut.txt.gz
	python ../scripts/uniform_snap.py orkut.txt > uniform-orkut.txt
	rm -f orkut.txt
	python ../scripts/weighted_snap.py uniform-orkut.txt > progs/data/orkut.txt

clean:
	rm -f $(RESULTS_FILE)
	rm -rf output
