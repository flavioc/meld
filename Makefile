
OS = $(shell uname -s)

INCLUDE_DIRS = -I.
LIBRARY_DIRS =

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
C0X = -std=c++0x

CFLAGS = $(ARCH) $(PROFILING) $(OPTIMIZATIONS) $(WARNINGS) $(DEBUG) $(INCLUDE_DIRS) $(COX)
LIBRARIES = -lpthread -lm -lboost_thread-mt

#ifneq ($(COMPILE_MPI),)
	LIBRARIES += -lmpi -lmpi_cxx -lboost_serialization-mt -lboost_mpi-mt
	CFLAGS += -DCOMPILE_MPI=1
#endif

CXX = g++

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
			 vm/predicate.o \
			 vm/types.o \
			 vm/instr.o \
			 db/node.o \
			 db/tuple.o \
			 db/agg_configuration.o \
			 db/tuple_aggregate.o \
			 db/database.o \
			 process/process.o \
			 process/machine.o \
			 process/remote.o \
			 process/router.o \
			 vm/state.o \
			 vm/tuple.o vm/exec.o \
			 sched/mpi/message.o \
			 mem/thread.o \
			 db/trie.o \
			 sched/mpi/message_buffer.o \
			 sched/global/static.o \
			 sched/global/threads_static.o \
			 sched/global/mpi.o \
			 sched/local/threads_static.o \
			 sched/local/threads_dynamic.o \
			 sched/thread/threaded.o \
			 sched/thread/queue_buffer.o \
			 sched/thread/assert.o \
			 sched/mpi/tokenizer.o \
			 sched/mpi/handler.o \
			 sched/local/mpi_threads_dynamic.o

all: meld print predicates

meld: $(OBJS) meld.o
	$(COMPILE) meld.o -o meld

print: $(OBJS) print.o
	$(COMPILE) print.o -o print

predicates: $(OBJS) predicates.o
	$(COMPILE) predicates.o -o predicates

meld.o: meld.cpp process/machine.hpp \
				process/router.hpp \
				sched/types.hpp

print.o: print.cpp vm/program.hpp

utils/utils.o: utils/utils.cpp utils/utils.hpp

vm/instr.o: vm/instr.cpp vm/instr.hpp \
						utils/utils.hpp

db/tuple.o: db/tuple.cpp db/tuple.hpp

db/agg_configuration.o: db/tuple.hpp db/agg_configuration.cpp \
												db/agg_configuration.hpp

db/tuple_aggregate.o: db/tuple.hpp db/agg_configuration.hpp \
											db/tuple_aggregate.hpp db/tuple_aggregate.cpp

db/node.o: db/node.cpp db/node.hpp \
					db/tuple.hpp db/trie.hpp \
					db/agg_configuration.hpp \
					db/tuple_aggregate.hpp

db/database.o: db/database.cpp db/database.hpp vm/instr.hpp \
							db/node.hpp

process/process.o: process/process.cpp process/process.hpp vm/instr.hpp \
									db/node.hpp sched/mpi/message_buffer.hpp

process/machine.o: process/machine.hpp process/machine.cpp \
									vm/state.hpp process/remote.hpp process/process.hpp \
									vm/instr.hpp conf.hpp \
									sched/mpi/message.hpp \
									sched/global/static.hpp \
									sched/global/threads_static.hpp \
									sched/global/mpi.hpp \
									sched/global/threads_static.hpp \
									sched/local/threads_static.hpp \
									sched/local/threads_dynamic.hpp \
									sched/local/mpi_threads_dynamic.hpp \
									sched/types.hpp \
									db/database.hpp \
									vm/predicate.hpp

process/remote.o: process/remote.hpp process/remote.cpp	\
									vm/instr.hpp conf.hpp

vm/state.o: vm/state.cpp vm/state.hpp	\
						vm/instr.hpp

vm/tuple.o: vm/tuple.cpp vm/tuple.hpp	\
						vm/instr.hpp runtime/list.hpp \
						utils/utils.hpp

vm/program.o: vm/program.cpp vm/program.hpp vm/instr.hpp \
							vm/predicate.hpp

vm/exec.o: vm/exec.cpp vm/exec.hpp process/process.hpp	\
						vm/instr.hpp db/node.hpp

process/router.o: process/router.hpp process/router.cpp \
									process/remote.hpp \
									utils/time.hpp \
									sched/mpi/message.hpp \
									sched/mpi/request.hpp \
									sched/mpi/token.hpp \
									conf.hpp

sched/mpi/message.o: sched/mpi/message.cpp sched/mpi/message.hpp \
									db/node.hpp db/tuple.hpp

mem/thread.o: mem/thread.cpp mem/thread.hpp \
							mem/pool.hpp mem/chunkgroup.hpp \
							mem/base.hpp mem/chunk.hpp

db/trie.o: db/trie.cpp db/trie.hpp \
					utils/utils.hpp

vm/types.o: vm/types.hpp vm/types.hpp \
						utils/utils.hpp

sched/mpi/message_buffer.o: sched/mpi/message_buffer.hpp \
									sched/mpi/message_buffer.cpp \
									sched/mpi/message.hpp sched/mpi/request.hpp

sched/global/static.o: sched/global/static.cpp sched/global/static.hpp \
								sched/base.hpp

sched/global/threads_static.o: sched/global/threads_static.cpp \
								sched/global/threads_static.hpp \
								sched/base.hpp sched/global/static.hpp \
								sched/queue/node.hpp \
								sched/queue/unsafe_queue_count.hpp \
								sched/queue/safe_queue.hpp \
								sched/thread/termination_barrier.hpp \
								utils/atomic.hpp \
								sched/thread/queue_buffer.hpp \
								sched/thread/assert.hpp

sched/global/mpi.o: sched/global/mpi.hpp sched/global/mpi.cpp \
						sched/base.hpp sched/global/static.hpp \
						sched/mpi/token.hpp sched/mpi/tokenizer.hpp \
						conf.hpp sched/mpi/message_buffer.hpp \
						sched/mpi/handler.hpp

sched/local/threads_static.o: sched/base.hpp sched/local/threads_static.hpp \
								sched/local/threads_static.cpp sched/queue/node.hpp \
								sched/thread/termination_barrier.hpp \
								utils/atomic.hpp \
								sched/thread/node.hpp sched/queue/unsafe_queue_count.hpp \
								sched/queue/safe_queue.hpp \
								sched/thread/threaded.hpp \
								sched/queue/bounded_pqueue.hpp \
								sched/thread/assert.hpp

sched/local/threads_dynamic.o: sched/base.hpp sched/local/threads_static.hpp \
											sched/local/threads_dynamic.hpp \
											sched/local/threads_dynamic.cpp \
											sched/thread/node.hpp sched/thread/termination_barrier.hpp \
											sched/queue/node.hpp sched/thread/steal_set.hpp \
											sched/queue/safe_queue.hpp \
											sched/thread/threaded.hpp \
											conf.hpp utils/atomic.hpp \
											sched/queue/bounded_pqueue.hpp \
											sched/thread/assert.hpp

sched/thread/threaded.o: sched/thread/termination_barrier.hpp \
									sched/thread/threaded.hpp sched/thread/threaded.cpp \
									utils/atomic.hpp

sched/thread/queue_buffer.o: sched/thread/queue_buffer.hpp \
														sched/thread/queue_buffer.cpp \
														vm/predicate.hpp

sched/thread/assert.o: sched/thread/assert.hpp \
											sched/thread/assert.cpp

sched/mpi/tokenizer.o: sched/mpi/token.hpp sched/mpi/tokenizer.cpp \
									 sched/mpi/tokenizer.hpp process/remote.hpp

sched/mpi/handler.o: sched/mpi/handler.hpp sched/mpi/handler.cpp

sched/local/mpi_threads_dynamic.o: sched/local/mpi_threads_dynamic.hpp \
										sched/local/mpi_threads_dynamic.cpp \
										sched/mpi/tokenizer.hpp sched/local/threads_dynamic.hpp \
										sched/mpi/token.hpp conf.hpp sched/mpi/message_buffer.hpp \
										sched/queue/bounded_pqueue.hpp sched/mpi/handler.hpp

clean:
	rm -f meld print *.o vm/*.o \
		db/*.o process/*.o \
		runtime/*.o utils/*.o \
		mem/*.o sched/*.o \
		sched/mpi/*.o \
		sched/queue/*.o \
		sched/global/*.o \
		sched/thread/*.o \
		sched/local/*.o

re: clean all
