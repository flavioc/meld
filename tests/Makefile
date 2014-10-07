
VM = ../meld -d -f

.PHONY: test clean

all: ARGS = 1
all: test-serial test-thread

full: ARGS = 3
full: test-serial test-thread

test-serial:
	@echo "====> Serial test"
	@bash test_all.sh serial $(ARGS)

test-thread:
	@echo "===> Thread test"
	@bash test_all.sh thread $(ARGS)

code/%.m: FORCE
	@bash test.sh $@ th1

compiled:
	@mkdir -p code
	@meld-compile-directory progs code

clean:
	rm -f code/*.m test
