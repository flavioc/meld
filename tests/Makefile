
VM = ../meld -d -f

all:
	@echo "Type 'make test' to test. Or type 'make compiled' to recompile tests."

test-tl:
	@bash test_all.sh tl

test-td:
	@bash test_all.sh td

test-tx:
	@bash test_all.sh tx

test-serial:
	@bash test_all.sh serial

test-mpidynamic:
	@bash test_all.sh mpidynamic

test-mpistatic:
	@bash test_all.sh mpistatic

test-mpisingle:
	@bash test_all.sh mpisingle

test-single:
	@bash test_all.sh sin

test:
	@bash test_all.sh all

compiled:
	@mkdir -p code
	@compile-directory progs code
	@rm -f code/*.ast code/*.code
