
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
LDFLAGS = $(PROFILING) $(LIBRARY_DIRS) -lm -lpthread $(TCMALLOC) -m32

OBJS = utils.o extern_functions.o vm.o \
			 meld.o list.o barrier.o hash.o set_runtime.o \
			 partition.o thread.o node.o \
			 list_runtime.o core.o \
			 stats.o allocator.o

all: $(TARGET)

$(TARGET): $(OBJS)

list_runtime.o: list_runtime.c list_runtime.h

allocator.o: allocator.c allocator.h

node.o: node.c node.h list.o

vm.o: vm.c core.h defs.h api.h \
		extern_functions.h extern_functions.c \
		vm.h node.h thread.h list.o

meld.o: meld.c lib/image.c lib/pagerank.c lib/walkgrid.c lib/connectivity.c lib/shortest_path.c thread.o

thread.o: thread.c thread.h node.h

partition.o: thread.h partition.h partition.c list.o

list.o: list.c list.h

set_runtime.o: set_runtime.c set_runtime.h

core.o: model.h api.h defs.h core.c core.h

stats.o: stats.c stats.h

extern_functions.o: extern_functions.c extern_functions.h

clean:
	rm -f $(TARGET) *.o

re: clean all

