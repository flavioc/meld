
all:
	@echo "Nothing to do. Type 'make compiled' if you want to compile the benchmarks."

compiled:
	@mkdir -p code
	@compile-directory progs code
	@rm -f code/*.ast code/*.code

clean:
	rm -rf code
