RELEASE = true
INTERFACE = false
JIT = false
EXTRA_ASSERTS = false
# may use 'pool' or 'malloc'
ALLOCATOR = pool
# Use 'true' for dynamic indexing of facts.
INDEXING = false
# Make the virtual machine use the node pointers in predicate arguments
# typed as 'node'. For multithreaded meld this will improve speed since
# there's one less lookup.
USE_ADDRESSES = true
TASK_STEALING = true

# statistics flags.
FACT_STATISTICS = false
LOCK_STATISTICS = false
# add instrumentation functionality
INSTRUMENTATION = false
# gather statistics about the core VM execution
CORE_STATISTICS = false
# activate this to collect statistics on memory use
MEMORY_STATISTICS = false
