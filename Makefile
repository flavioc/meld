
OS = $(shell uname -s)

INCLUDE_DIRS = -I/usr/include -I.
LIBRARY_DIRS = -L/usr/lib

ifeq (exists, $(shell test -d /opt/local/include && echo exists))
	INCLUDE_DIRS += -I/opt/local/include
endif
ifeq (exists, $(shell test -d /opt/local/lib  && echo exists))
	LIBRARY_DIRS += -L/opt/local/lib
endif
ifeq (exists, $(shell test -d /usr/include/openmpi-x86_64 && echo exists))
	INCLUDE_DIRS += -I/usr/include/openmpi-x86_64/
endif
ifeq (exists, $(shell test -d /usr/lib64/openmpi/lib && echo exists))
	LIBRARY_DIRS += -L/usr/lib64/openmpi/lib
endif

PROFILING = #-pg
OPTIMIZATIONS = -O0
ARCH = -march=x86-64
DEBUG = -g
WARNINGS = -Wall -Wno-sign-compare

CFLAGS = $(ARCH) $(PROFILING) $(OPTIMIZATIONS) $(WARNINGS) $(DEBUG) $(INCLUDE_DIRS)
LIBRARIES = -lpthread -lm -lboost_thread-mt

ifneq ($(COMPILE_MPI),)
	LIBRARIES += -lmpi -lmpi_cxx -lboost_serialization-mt -lboost_mpi-mt
	CFLAGS += -DCOMPILE_MPI=1
endif

CXX = g++
C0X = -std=c++0x

GCC_MINOR    := $(shell $(CXX) -v 2>&1 | \
													grep " version " | cut -d' ' -f3  | cut -d'.' -f2)

ifeq ($(GCC_MINOR),2)
	CFLAGS += -DTEMPLATE_OPTIMIZERS=1
endif
ifeq ($(GCC_MINOR),4)
	CFLAGS += -DIMPLEMENT_MISSING_MPI=1 $(C0X)
endif

CXXFLAGS = $(CFLAGS)
LDFLAGS = $(PROFILING) $(LIBRARY_DIRS) $(LIBRARIES)
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
			 mem/thread.o \
			 db/trie.o \
			 sched/buffer.o \
			 sched/static.o \
			 sched/threads.o \
			 sched/mpi.o \
			 sched/static_local.o \
			 sched/dynamic_local.o

all: meld print

meld: $(OBJS) meld.o
	$(COMPILE) meld.o -o meld

print: $(OBJS) print.o
	$(COMPILE) print.o -o print

meld.o: meld.cpp utils/utils.hpp process/machine.hpp \
				process/router.hpp sched/base.hpp sched/threads.hpp

print.o: print.cpp vm/program.hpp

utils/utils.o: utils/utils.cpp utils/utils.hpp

vm/instr.o: vm/instr.cpp vm/instr.hpp \
						utils/utils.hpp

db/tuple.o: db/tuple.cpp db/tuple.hpp

db/node.o: db/node.cpp db/node.hpp \
					db/tuple.hpp db/trie.hpp

db/database.o: db/database.cpp db/database.hpp vm/instr.hpp \
							db/node.hpp

process/process.o: process/process.cpp process/process.hpp vm/instr.hpp \
									db/node.hpp sched/buffer.hpp

process/machine.o: process/machine.hpp process/machine.cpp \
									vm/state.hpp process/remote.hpp process/process.hpp \
									vm/instr.hpp conf.hpp \
									process/message.hpp \
									sched/static.hpp \
									sched/threads.hpp \
									sched/mpi.hpp \
									sched/static_local.hpp

process/remote.o: process/remote.hpp process/remote.cpp	\
									vm/instr.hpp conf.hpp

vm/state.o: vm/state.cpp vm/state.hpp	\
						vm/instr.hpp

vm/tuple.o: vm/tuple.cpp vm/tuple.hpp	\
						vm/instr.hpp runtime/list.hpp \
						utils/utils.hpp

vm/program.o: vm/program.cpp vm/program.hpp vm/instr.hpp

vm/exec.o: vm/exec.cpp vm/exec.hpp process/process.hpp	\
						vm/instr.hpp db/node.hpp

process/router.o: process/router.hpp process/router.cpp \
									process/remote.hpp \
									process/message.hpp \
									utils/time.hpp \
									process/request.hpp \
									sched/token.hpp

process/message.o: process/message.cpp process/message.hpp \
									db/node.hpp db/tuple.hpp

mem/thread.o: mem/thread.cpp mem/thread.hpp \
							mem/pool.hpp mem/chunkgroup.hpp \
							mem/base.hpp mem/chunk.hpp

db/trie.o: db/trie.cpp db/trie.hpp \
					utils/utils.hpp

vm/types.o: vm/types.hpp vm/types.hpp \
						utils/utils.hpp

sched/buffer.o: sched/buffer.hpp sched/buffer.cpp \
									process/message.hpp process/request.hpp

sched/static.o: sched/static.cpp sched/static.hpp \
								sched/base.hpp

sched/threads.o: sched/threads.cpp sched/threads.hpp \
								sched/base.hpp sched/static.hpp \
								sched/queue.hpp \
								sched/termination_barrier.hpp

sched/mpi.o: sched/mpi.hpp sched/mpi.cpp \
						sched/base.hpp sched/static.hpp \
						sched/token.hpp

sched/static_local.o: sched/base.hpp sched/static_local.hpp \
								sched/static_local.cpp sched/queue.hpp \
								sched/termination_barrier.hpp \
								sched/node.hpp

sched/dynamic_local.o: sched/base.hpp sched/static_local.hpp \
											sched/dynamic_local.hpp sched/dynamic_local.cpp \
											sched/node.hpp sched/termination_barrier.hpp 

clean:
	rm -f meld print *.o vm/*.o \
		db/*.o process/*.o \
		runtime/*.o utils/*.o \
		mem/*.o sched/*.o

re: clean all

