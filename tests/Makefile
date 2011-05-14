
VM = ../meld -d -f

all:
	@echo "Type 'make tests' to test. Or type 'make compiled' to recompile tests."

tests:
	@for file in code/*.m; do \
		./test.sh $$file || exit 1; \
	done

compiled:
	@mkdir -p code
	@compile-directory progs code
	@rm -f code/*.ast code/*.code
