static NodeID source, dest;
static Node *node_dest;

static void
print_success(void)
{
  tuple_entry *entry = node_dest->tuples[type_lookup("success")].head;
  
  assert(entry->next == NULL);
  
  tuple_t tuple = entry->tuple;
  
  meld_int many_ways = MELD_INT(GET_TUPLE_FIELD(tuple, 0));
  
  printf("There are %d paths from node " NODE_FORMAT " to node " NODE_FORMAT "\n", many_ways, source, dest);
}

static void
config_read(const char *filename)
{
	FILE *fp = fopen(filename, "r");

	if(fp == NULL)
		err("Can't open %s for reading", filename, 0, 0);

	fscanf(fp, "GRAPH : " NODE_FORMAT " -> " NODE_FORMAT "\n", &source, &dest);

	while(!feof(fp)) {
    NodeID a, b;

		if(fscanf(fp, NODE_FORMAT " " NODE_FORMAT "\n", &a, &b) != 2)
			continue;

		Node *node_a = nodes_ensure(a);
    Node *node_b = nodes_ensure(b);
    
		if(!node_has_neighbor(node_a, node_b)) {
      node_add_neighbor(node_a, node_b);
		  printf("EDGE FROM " NODE_FORMAT " to " NODE_FORMAT "\n", a, b);
		}
	}

	Node *node_source = nodes_get(source);
	if(node_source == NULL) {
		fprintf(stderr, "Can't find node " NODE_FORMAT " in the nodes set.\n", source);
		exit(EXIT_FAILURE);
	}
	
  node_first_tuples_add(node_source, tuple_alloc(type_lookup("start")));

	node_dest = nodes_get(dest);
	if(node_dest == NULL) {
		fprintf(stderr, "Can't find node " NODE_FORMAT " in the nodes set.\n", dest);
		exit(EXIT_FAILURE);
	}
	
  node_first_tuples_add(node_dest, tuple_alloc(type_lookup("end")));
  
  vm_add_finish_function(print_success);
}