
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

OBJS = utils.o extern_functions.o vm.o \
			 meld.o list.o barrier.o hash.o set_runtime.o \
			 partition.o thread.o node.o \
			 list_runtime.o core.o \
			 stats.o allocator.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

list_runtime.o: list_runtime.cpp list_runtime.h

allocator.o: allocator.cpp allocator.h

node.o: node.cpp node.h list.o

vm.o: vm.cpp core.h defs.h api.h \
		extern_functions.h extern_functions.cpp \
		vm.h node.h thread.h list.o

meld.o: meld.cpp thread.o

thread.o: thread.cpp thread.h node.h

partition.o: thread.h partition.h partition.cpp list.o

list.o: list.cpp list.h

set_runtime.o: set_runtime.cpp set_runtime.h

core.o: model.h api.h defs.h core.cpp core.h

stats.o: stats.cpp stats.h

extern_functions.o: extern_functions.cpp extern_functions.h

clean:
	rm -f $(TARGET) *.o

re: clean all

