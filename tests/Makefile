
VM = ../meld -d -f

.PHONY: test clean

all: test-serial test-thread

all: compiled

test-serial:
	@echo "====> Serial test"
	@bash test_all.sh serial

test-thread:
	@echo "===> Thread test"
	@bash test_all.sh thread

code/%.m: FORCE
	@bash test.sh $@ th1

compiled:
	@mkdir -p code
	@meld-compile-directory progs code

clean:
	rm -f code/*.m test
