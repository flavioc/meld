
include conf.mk

OS = $(shell uname -s)

INCLUDE_DIRS = -I.
LIBRARY_DIRS =

ARCH = -march=x86-64
FLAGS =
LIBS = 

ifeq ($(RELEASE), true)
	DEBUG =
	OPTIMIZATIONS = -O3 -DNDEBUG
else
	DEBUG = -g
	PROFILING = -pg
	OPTIMIZATIONS = -O0
endif

ifeq ($(JIT), true)
	FLAGS += -DUSE_JIT
	LIBS += -L/usr/local/lib/x86_64 -ljit
endif

WARNINGS = -Wall -Wextra -Werror
C0X = -std=c++0x

ifeq ($(INTERFACE),true)
	LIBS += -lwebsocketpp -ljson_spirit
	FLAGS += -DUSE_UI=1
endif

CFLAGS = $(ARCH) $(PROFILING) $(OPTIMIZATIONS) $(WARNINGS) $(DEBUG) $(INCLUDE_DIRS) $(FLAGS) $(C0X) #-fno-gcse -fno-crossjumping
LIBRARIES = -pthread -lm -lreadline -lboost_thread-mt -lboost_system-mt \
				-lboost_date_time-mt -lboost_regex-mt $(LIBS)

CXX = g++

GCC_MINOR    := $(shell $(CXX) -v 2>&1 | \
													grep " version " | cut -d' ' -f3  | cut -d'.' -f2)

ifeq ($(GCC_MINOR),2)
	CFLAGS += -DTEMPLATE_OPTIMIZERS=1
endif
ifeq ($(GCC_MINOR),4)
	CFLAGS += $(C0X)
endif

CXXFLAGS = $(CFLAGS)
LDFLAGS = $(PROFILING) $(LIBRARY_DIRS) $(LIBRARIES)
COMPILE = $(CXX) $(CXXFLAGS) $(OBJS)

SRCS = utils/utils.cpp \
		 	utils/types.cpp \
			utils/fs.cpp \
			 vm/program.cpp \
			 vm/predicate.cpp \
			 vm/types.cpp \
			 vm/instr.cpp \
			 vm/state.cpp \
			 vm/tuple.cpp \
			 vm/exec.cpp \
			 vm/external.cpp \
			 vm/rule.cpp \
			 vm/rule_matcher.cpp \
			 vm/stat.cpp \
			 db/node.cpp \
			 db/tuple.cpp \
			 db/agg_configuration.cpp \
			 db/tuple_aggregate.cpp \
			 db/database.cpp \
			 db/trie.cpp \
			 process/machine.cpp \
			 process/remote.cpp \
			 process/router.cpp \
			 mem/thread.cpp \
			 mem/center.cpp \
			 mem/stat.cpp \
			 sched/base.cpp \
			 sched/common.cpp \
			 sched/serial.cpp \
			 sched/serial_ui.cpp \
			 thread/threads.cpp \
			 thread/prio.cpp \
			 sched/thread/threaded.cpp \
			 sched/thread/assert.cpp \
			 external/math.cpp \
			 external/lists.cpp \
			 external/utils.cpp \
			 external/strings.cpp \
			 external/others.cpp \
			 external/core.cpp \
			 external/structs.cpp \
			 stat/stat.cpp \
			 stat/slice.cpp \
			 stat/slice_set.cpp \
			 ui/manager.cpp \
			 ui/client.cpp \
			 interface.cpp \
			 sched/sim.cpp \
			 jit/build.cpp

OBJS = $(patsubst %.cpp,%.o,$(SRCS))

all: meld print server simulator

-include Makefile.externs
Makefile.externs:	Makefile
	@echo "Remaking Makefile.externs"
	@/bin/rm -f Makefile.externs
	@for i in $(SRCS); do $(CXX) $(CXXFLAGS) -MM -MT $${i/%.cpp/.o} $$i >> Makefile.externs; done
	@echo "Makefile.externs ready"

meld: $(OBJS) meld.o
	$(COMPILE) meld.o -o meld $(LDFLAGS)

print: $(OBJS) print.o
	$(COMPILE) print.o -o print $(LDFLAGS)

server: $(OBJS) server.o
	$(COMPILE) server.o -o server $(LDFLAGS)

simulator: $(OBJS) simulator.o
	$(COMPILE) simulator.o -o simulator $(LDFLAGS)

depend:
	makedepend -- $(CXXFLAGS) -- $(shell find . -name '*.cpp')

clean:
	find . -name '*.o' | xargs rm -f
	rm -f meld predicates print server Makefile.externs
# DO NOT DELETE

