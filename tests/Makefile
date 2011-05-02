
VM = ../meld -d -f

all: tests

tests:
	@for file in code/*.m; do \
		./test.sh $$file || exit 1; \
	done

compiled:
	for file in progs/*.meld; do \
		name=`basename $$file .meld`; \
		compile-file $(PWD)/$$file $(PWD)/code/$$name; \
	done
	rm -f code/*.ast code/*.code
