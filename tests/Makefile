
VM = ../meld -d -f

.PHONY: test clean

ARGS = 1

all: ARGS = 1
all: serial thread

full: ARGS = 3
full: serial thread

test: all

serial:
	@echo "====> Serial test"
	@bash test_all.sh serial $(ARGS)

thread:
	@echo "===> Thread test"
	@bash test_all.sh thread $(ARGS)

code/%.m: FORCE
	@bash test.sh $@ th1

compiled:
	@mkdir -p code
	@meld-compile-directory progs code

clean:
	rm -f code/*.m test
