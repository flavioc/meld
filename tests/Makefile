
VM = ../meld -d -f

.PHONY: test clean

all: compiled

test:
	@bash test_all.sh th 1

code/%.m: FORCE
	@bash test.sh $@ th1

compiled:
	@mkdir -p code
	@meld-compile-directory progs code

clean:
	rm -f code/*.m test
