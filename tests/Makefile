
VM = ../meld -d -f

.PHONY: test clean

all: compiled

test:
	@bash test_all.sh sl

code/%.m: FORCE
	@bash test.sh $@ sl

compiled:
	@mkdir -p code
	@meld-compile-directory progs code

clean:
	rm -f code/*.m test
