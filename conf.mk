RELEASE = true
# may use 'pool' or 'malloc'
ALLOCATOR = pool
# spinlock algorithm:
# mutex: use std mutex objects.
# semaphore: use system semaphores.
# ticket: use ticket spinlock.
# queued: use MCS queued spin lock.
LOCK_ALGORITHM = ticket
# Use 'true' for dynamic indexing of facts.
INDEXING = false
# Make the virtual machine use the node pointers in predicate arguments
# typed as 'node'. For multithreaded meld this will improve speed since
# there's one less lookup.
USE_ADDRESSES = true
# allow threads to steal nodes from each other.
TASK_STEALING = true
# enable node collection if the node is no longer referenced anywhere.
GC_NODES = true

# statistics flags.
FACT_STATISTICS = false
LOCK_STATISTICS = false
# add instrumentation functionality
INSTRUMENTATION = false
# gather statistics about the core VM execution
CORE_STATISTICS = false
# activate this to collect statistics on memory use
MEMORY_STATISTICS = false
# activate fact buffering (only send facts after the node has completed running)
FACT_BUFFERING = true


INTERFACE = false
EXTRA_ASSERTS = false
