
VM = ../meld -d -f

all:
	@echo "Type 'make test' to test. Or type 'make compiled' to recompile tests."

test-ts:
	@bash test_all.sh ts

test-serial:
	@bash test_all.sh serial

test-mis:
	@bash test_all.sh mis

test-mix:
	@bash test_all.sh mix

test-mpi:
	@bash test_all.sh mpi

test:
	@bash test_all.sh all

compiled:
	@mkdir -p code
	@compile-directory progs code
	@rm -f code/*.ast code/*.code
