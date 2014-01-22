
VM = ../meld -d -f

all: compiled

test-serial: test

test:
	@bash test_all.sh sl

compiled:
	@mkdir -p code
	@meld-compile-directory progs code

clean:
	rm -f code/*.m
