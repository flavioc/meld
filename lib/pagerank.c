
static bool
print_rank_node(hash_table_element el, void* data)
{
  (void)data;
  
	Node *node = (Node *)el;

	tuple_type RANK_TYPE = type_lookup("rank");
	
	printf(NODE_FORMAT ":\n", node->order);

	tuple_entry *entry;
	for(entry = node->tuples[RANK_TYPE].head;
	    entry != NULL;
	    entry = entry->next)
	{
		tuple_t tuple = entry->tuple;
		meld_int index = MELD_INT(GET_TUPLE_FIELD(tuple, 0));
		meld_float data = MELD_FLOAT(GET_TUPLE_FIELD(tuple, 1));
			
		printf("\t%d: %f\n", index, data);
	}

	return true;
}

static void
print_ranks(void)
{
	nodes_iterate(print_rank_node, NULL);
}

#if 0
static bool
print_links_node(hash_table_element el, void* data)
{
  (void)data;
  
	Node *node = (Node *)el;
	tuple_type NUM_LINKS = type_lookup("numLinks");
	printf(NODE_FORMAT ": ", node->order);

	tuple_entry *entry;
	
	for(entry = node->tuples[NUM_LINKS].head;
	    entry != NULL;
	    entry = entry->next)
	{
		tuple_t tuple = entry->tuple;
		meld_int total = MELD_INT(GET_TUPLE_FIELD(tuple, 1));
		printf("%d\n", total);
	}
	
	return true;
}

static void
print_num_links(void)
{
	nodes_iterate(print_links_node, NULL);
}
#endif

static tuple_t total_pages = NULL;
static tuple_t initial_rank = NULL;

static inline tuple_t
tuple_build_num_links(Node *node)
{
  tuple_t num_links = tuple_alloc(type_lookup("numLinks"));
  int links = node_total_neighbors(node);
  
  SET_TUPLE_FIELD(num_links, 1, &links);
  
  return num_links;
}

static bool
node_add_tuples(hash_table_element el, void* data)
{
  (void)data;
  
  Node *node = (Node *)el;
  
  node_first_tuples_add(node, tuple_build_num_links(node));
  node_first_tuples_add(node, total_pages);
  //node_first_tuples_add(node, initial_rank);
  
  return true;
}

static inline tuple_t
tuple_build_num_pages(void)
{
  tuple_t total_pages = tuple_alloc(type_lookup("numPages"));
  const int total = nodes_total();
  
  SET_TUPLE_FIELD(total_pages, 0, &total);
	
  return total_pages;
}

static inline tuple_t
tuple_build_initial_rank(void)
{
  tuple_t rank = tuple_alloc(type_lookup("rank"));
  meld_float value = 1.0 / (meld_float)nodes_total();
  meld_int zero = 0.0;
  
  SET_TUPLE_FIELD(rank, 0, &zero);
  SET_TUPLE_FIELD(rank, 1, &value);
  
  return rank;
}

static void
config_read(const char *filename)
{
	FILE* f = fopen(filename, "r");

	if (f == NULL)
		err("Can't open %s for reading", filename, 0, 0);

	NodeID src, dst;

	while (!feof(f)) {
		if (fscanf(f, NODE_FORMAT " " NODE_FORMAT "\n", &src, &dst) != 2)
      continue;

    Node* pagesrc = nodes_ensure(src);
		Node* pagedst = nodes_ensure(dst);

    if(!node_has_neighbor(pagesrc, pagedst)) {
		  node_add_neighbor(pagesrc, pagedst);
	  }
	}

	fclose(f);
	
	/* create common tuples */
  total_pages = tuple_build_num_pages();
  initial_rank = tuple_build_initial_rank();
	
	/* add tuples for each node */
  nodes_iterate(node_add_tuples, NULL);
	
	/* print ranks at the end */
	vm_add_finish_function(print_ranks);
}
