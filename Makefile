
OS = $(shell uname -s)

INCLUDE_DIRS = -I. -I/opt/local/include
LIBRARY_DIRS = -L/opt/local/lib

PROFILING = #-pg
OPTIMIZATIONS = -O0
DEBUG = -g
WARNINGS = -Wall

CFLAGS = $(PROFILING) $(OPTIMIZATIONS) $(WARNINGS) $(DEBUG) $(INCLUDE_DIRS) -m32
CXXFLAGS = $(CFLAGS) #-std=c++0x
LIBRARIES = -lpthread -lm -lmpi -lmpi_cxx -lboost_thread-mt -lboost_mpi-mt -lboost_serialization-mt
LDFLAGS = $(PROFILING) $(LIBRARY_DIRS) $(LIBRARIES)
CXX = mpic++
COMPILE = $(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS)

OBJS = utils/utils.o \
			 vm/program.o \
			 vm/predicate.o vm/types.o \
			 vm/instr.o db/node.o \
			 db/tuple.o db/database.o \
			 process/process.o \
			 process/machine.o \
			 process/remote.o \
			 process/router.o \
			 vm/state.o \
			 vm/tuple.o vm/exec.o \
			 process/message.o \
			 mem/thread.o

all: meld print

meld: $(OBJS) meld.o
	$(COMPILE) meld.o -o meld

print: $(OBJS) print.o
	$(COMPILE) print.o -o print

meld.o: meld.cpp utils/utils.hpp process/machine.hpp \
				process/router.hpp

print.o: print.cpp vm/program.hpp

utils/utils.o: utils/utils.cpp utils/utils.hpp

vm/instr.o: vm/instr.cpp vm/instr.hpp

db/tuple.o: db/tuple.cpp db/tuple.hpp

db/node.o: db/node.cpp db/node.hpp

db/database.o: db/database.cpp db/database.hpp vm/instr.hpp \
							db/node.hpp

process/process.o: process/process.cpp process/process.hpp vm/instr.hpp

process/machine.o: process/machine.hpp process/machine.cpp \
									vm/state.hpp process/remote.hpp process/process.hpp \
									vm/instr.hpp conf.hpp \
									process/message.hpp

process/remote.o: process/remote.hpp process/remote.cpp	\
									vm/instr.hpp conf.hpp

vm/state.o: vm/state.cpp vm/state.hpp	\
						vm/instr.hpp

vm/tuple.o: vm/tuple.cpp vm/tuple.hpp	\
						vm/instr.hpp runtime/list.hpp

vm/program.o: vm/program.cpp vm/program.hpp vm/instr.hpp

vm/exec.o: vm/exec.cpp vm/exec.hpp process/process.hpp	\
						vm/instr.hpp db/node.hpp

process/router.o: process/router.hpp process/router.cpp \
									process/remote.hpp \
									process/message.hpp

process/message.o: process/message.cpp process/message.hpp \
									db/node.hpp db/tuple.hpp

mem/thread.o: mem/thread.cpp mem/thread.hpp \
							mem/pool.hpp mem/chunkgroup.hpp \
							mem/base.hpp mem/chunk.hpp

clean:
	rm -f $(TARGET) *.o vm/*.o db/*.o process/*.o runtime/*.o utils/*.o mem/*.o

re: clean all

