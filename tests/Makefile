
VM = ../meld -d -f

all:
	@echo "Type 'make test' to test. Or type 'make compiled' to recompile tests."

test-serial:
	@bash test_all.sh serial

test:
	@bash test_all.sh all

compiled:
	@mkdir -p code
	@compile-directory progs code
	@rm -f code/*.ast code/*.code
