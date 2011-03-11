static Node *node_dest = NULL;
static NodeID source, dest;

static void
print_final_path(void)
{
  tuple_type path_type = type_lookup("path");
  
  tuple_queue queue = node_dest->tuples[path_type];
  tuple_entry *entry = queue.head;
  
  assert(entry->next == NULL);
  
  tuple_t tuple = entry->tuple;
  meld_int min_distance = MELD_INT(GET_TUPLE_FIELD(tuple, 0));
  
  printf("The minimal distance from " NODE_FORMAT " to " NODE_FORMAT " is %d\n", source, dest, min_distance);
  
  List *path_list = MELD_LIST(GET_TUPLE_FIELD(tuple, 1));
  list_print(path_list);
}

static inline tuple_t
tuple_build_weight(Node *b, int weight)
{
  tuple_t tuple = tuple_alloc(type_lookup("weight"));
  
  SET_TUPLE_FIELD(tuple, 0, &b);
  SET_TUPLE_FIELD(tuple, 1, &weight);
  
  return tuple;
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
    int weight;

		if(fscanf(fp, NODE_FORMAT " " NODE_FORMAT " [%d]\n", &a, &b, &weight) != 3)
			continue;

		Node *node_a = nodes_ensure(a);
    Node *node_b = nodes_ensure(b);
    
		if(!node_has_neighbor(node_a, node_b)) {
      node_add_neighbor(node_a, node_b);
  		node_first_tuples_add(node_a, tuple_build_weight(node_b, weight));
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
  
  vm_add_finish_function(print_final_path);
}
