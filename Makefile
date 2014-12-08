
TARGETS = meld print

include ./conf.mk

OS = $(shell uname -s)

INCLUDE_DIRS = -I $(PWD)
LIBRARY_DIRS =

ARCH = -march=x86-64
FLAGS =
LIBS =
PROFILING =

ifeq ($(RELEASE), true)
	DEBUG = -DNDEBUG
	OPTIMIZATIONS = -O3
else
	DEBUG = -g
	OPTIMIZATIONS = -O0
endif
ifeq ($(FREE_OBJS), true)
	FLAGS += -DFREE_OBJS
endif
ifeq ($(ALLOCATOR), pool)
	FLAGS += -DPOOL_ALLOCATOR
endif
ifeq ($(EXTRA_ASSERTS), true)
	FLAGS += -DTRIE_MATCHING_ASSERT -DMEMORY_ASSERT -DEXTRA_ASSERTS
endif
ifeq ($(INDEXING), true)
	FLAGS += -DDYNAMIC_INDEXING
endif
ifeq ($(USE_ADDRESSES), true)
	FLAGS += -DUSE_REAL_NODES
endif
ifeq ($(TASK_STEALING), true)
	FLAGS += -DTASK_STEALING
endif
ifeq ($(LOCK_STATISTICS), true)
	FLAGS += -DLOCK_STATISTICS
endif
ifeq ($(FACT_STATISTICS), true)
	FLAGS += -DFACT_STATISTICS
endif
ifeq ($(INSTRUMENTATION), true)
	FLAGS += -DINSTRUMENTATION
endif
ifeq ($(CORE_STATISTICS), true)
	FLAGS += -DCORE_STATISTICS
endif
ifeq ($(MEMORY_STATISTICS), true)
	FLAGS += -DMEMORY_STATISTICS
endif
ifeq ($(FACT_BUFFERING), true)
	FLAGS += -DFACT_BUFFERING
endif
ifeq ($(COORDINATION_BUFFERING), true)
	FLAGS += -DCOORDINATION_BUFFERING
endif
ifeq ($(LOCK_ALGORITHM), mutex)
	FLAGS += -DUSE_STD_MUTEX
endif
ifeq ($(LOCK_ALGORITHM), semaphore)
	FLAGS += -DUSE_SEMAPHORE
endif
ifeq ($(LOCK_ALGORITHM), ticket)
	FLAGS += -DUSE_SPINLOCK
endif
ifeq ($(LOCK_ALGORITHM), queued)
	FLAGS += -DUSE_QUEUED_SPINLOCK
endif
ifeq ($(GC_NODES), true)
	FLAGS += -DGC_NODES
endif

WARNINGS = -Wall -Wextra

CFLAGS = -std=c++11 $(ARCH) $(PROFILING) \
			$(OPTIMIZATIONS) $(WARNINGS) $(DEBUG) \
			$(INCLUDE_DIRS) $(FLAGS) #-fno-gcse -fno-crossjumping
LIBRARIES = -lm -lreadline -ldl $(LIBS) -pthread

CXX = g++

GCC_MINOR    := $(shell $(CXX) -v 2>&1 | \
													grep " version " | cut -d' ' -f3  | cut -d'.' -f2)

CLANG = $(shell $(CXX) -v 2>&1 | grep LLVM)
ifneq ($(CLANG), )
	CFLAGS += -Qunused-arguments
endif
ifeq ($(CLANG), )
	LIBRARIES += -lpthread
endif

CXXFLAGS = $(CFLAGS)
LDFLAGS = $(PROFILING) $(LIBRARY_DIRS) $(LIBRARIES)

COMPILE = $(CXX) $(CXXFLAGS) $(OBJS)

SRCS = utils/utils.cpp \
		 	utils/types.cpp \
			utils/fs.cpp \
			utils/mutex.cpp \
			vm/program.cpp \
			vm/predicate.cpp \
			vm/types.cpp \
			vm/instr.cpp \
			vm/state.cpp \
			vm/tuple.cpp \
			vm/full_tuple.cpp \
			vm/exec.cpp \
			vm/external.cpp \
			vm/rule.cpp \
			vm/rule_matcher.cpp \
			db/node.cpp \
			db/agg_configuration.cpp \
			db/tuple_aggregate.cpp \
			db/persistent_store.cpp \
			db/database.cpp \
			db/trie.cpp \
			db/hash_table.cpp \
			mem/thread.cpp \
			mem/center.cpp \
			mem/stat.cpp \
			runtime/objs.cpp \
			thread/ids.cpp \
			thread/threads.cpp \
			thread/coord.cpp \
			compiler/lexer.cpp \
			compiler/parser.cpp \
			external/math.cpp \
			external/lists.cpp \
			external/utils.cpp \
			external/strings.cpp \
			external/others.cpp \
			external/core.cpp \
			external/structs.cpp \
			machine.cpp \
			interface.cpp

ifeq ($(INSTRUMENTATION), true)
	SRCS += stat/stat.cpp \
			  stat/slice_set.cpp
endif
ifeq ($(CORE_STATISTICS), true)
	SRCS += vm/stat.cpp
endif

OBJS = $(patsubst %.cpp,%.o,$(SRCS))

all: $(TARGETS)

.PHONY: clean
clean:
	find . -name '*.o' | xargs rm -f
	rm -f meld print unit_tests/run

-include Makefile.externs
Makefile.externs:	conf.mk
	@echo "Remaking Makefile.externs"
	@/bin/rm -f Makefile.externs
	@for i in $(SRCS); do $(CXX) $(CXXFLAGS) -MM -MT $${i/%.cpp/.o} $$i >> Makefile.externs; done
	@echo "Makefile.externs ready"

meld: $(OBJS) meld.o
	$(COMPILE) meld.o -o meld $(LDFLAGS)

print: $(OBJS) print.o
	$(COMPILE) print.o -o print $(LDFLAGS)

TEST_FILES = external/tests.cpp \
				 db/trie_tests.cpp \
				 compiler/lexer_tests.cpp \
				 compiler/parser_tests.cpp

unit_tests/run: $(OBJS) unit_tests/run.cpp $(TEST_FILES)
	$(COMPILE) unit_tests/run.cpp -o unit_tests/run $(LDFLAGS) -lcppunit

test: unit_tests/run
	@./unit_tests/run

depend:
	makedepend -- $(CXXFLAGS) -- $(shell find . -name '*.cpp')

# DO NOT DELETE

