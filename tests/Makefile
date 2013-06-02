
VM = ../meld -d -f

all:
	@echo "Type 'make test' to test. Or type 'make compiled' to recompile tests."

test-linear:
	@bash test_linear.sh sl

test-tl:
	@bash test_all.sh tl

test-serial:
	@bash test_all.sh sl

test:
	@bash test_all.sh all

compiled:
	@mkdir -p code
	@compile-directory progs code
	@rm -f code/*.ast code/*.code
