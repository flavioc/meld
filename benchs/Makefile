
all:
	@echo "Nothing to do. Type 'make compiled' if you want to compile the benchmarks."

run:
	@bash run.sh

compiled:
	@mkdir -p code
	@meld-compile-directory progs code
	@rm -f code/*.ast code/*.code

clean:
	rm -rf code
