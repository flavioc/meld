
OS = $(shell uname -s)

INCLUDE_DIRS = -I. -Ilib/ -I/opt/local/include
LIBRARY_DIRS = -L/opt/local/lib

TARGET = meld
PROFILING = #-pg

OPTIMIZATIONS = -O2 -m32
DEBUG = -g
WARNINGS = -Wall -Wextra
TCMALLOC = #-ltcmalloc

CFLAGS = $(PROFILING) $(OPTIMIZATIONS) $(WARNINGS) $(DEBUG) $(INCLUDE_DIRS)
CXXFLAGS = $(CFLAGS)
LDFLAGS = $(PROFILING) $(LIBRARY_DIRS) -lm -lpthread $(TCMALLOC) -m32

OBJS = meld.o utils.o \
			 vm/program.o \
			 vm/predicate.o vm/types.o \
			 vm/instr.o db/node.o \
			 db/tuple.o db/database.o \
			 process/process.o \
			 process/exec.o \
			 vm/state.o \
			 vm/tuple.o vm/exec.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

meld.o: meld.cpp

utils.o: utils.cpp utils.hpp

vm/instr.o: vm/instr.cpp vm/instr.hpp

db/tuple.o: db/tuple.cpp db/tuple.hpp

db/node.o: db/node.cpp db/node.hpp

db/database.o: db/database.cpp db/database.hpp

process/process.o: process/process.cpp process/process.hpp

process/exec.o: process/exec.hpp process/exec.cpp

vm/state.o: vm/state.cpp vm/state.hpp

vm/tuple.o: vm/tuple.cpp vm/tuple.hpp

vm/program.o: vm/program.cpp vm/program.hpp

vm/exec.o: vm/exec.cpp vm/exec.hpp

clean:
	rm -f $(TARGET) *.o vm/*.o db/*.o process/*.o

re: clean all

