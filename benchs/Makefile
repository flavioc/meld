
all:
	@echo "Nothing to do. Type 'make compiled' if you want to compile the benchmarks."

ts:
	@bash benchmark.sh ts 3

tl:
	@bash benchmark.sh tl 3

td:
	@bash benchmark.sh td 3

mpi:
	@bash benchmark.sh mpi 3

mix:
	@bash benchmark.sh mix 3

compiled:
	@mkdir -p code
	@compile-directory progs code
	@rm -f code/*.ast code/*.code

clean:
	rm -rf code
