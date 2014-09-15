RELEASE = true
INTERFACE = false
JIT = false
EXTRA_ASSERTS = false
ALLOCATOR = pool # may use 'pool' or 'malloc'
# Use 'true' for dynamic indexing of facts.
INDEXING = true
# Make the virtual machine use the node pointers in predicate arguments
# typed as 'node'. For multithreaded meld this will improve speed since
# there's one less lookup.
USE_ADDRESSES = true
