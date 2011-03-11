
void printWalkGrid(void)
{
}

static inline tuple_t
tuple_build_source(float capacity)
{
	tuple_t tuple = tuple_alloc(type_lookup("source"));

	SET_TUPLE_FIELD(tuple, 0, &capacity);
	
  return tuple;
}

static inline tuple_t
tuple_build_load(float load)
{
  tuple_t tuple = tuple_alloc(type_lookup("load"));
  
  SET_TUPLE_FIELD(tuple, 0, &load);
  
  return tuple;
}

static inline tuple_t
tuple_build_sink(float capacity)
{
  tuple_t tuple = tuple_alloc(type_lookup("sink"));
  
  SET_TUPLE_FIELD(tuple, 0, &capacity);
  
  return tuple;
}

static inline tuple_t
tuple_build_unplugged(void)
{
  return tuple_alloc(type_lookup("unplugged"));
}

static void
config_read(const char *filename)
{
	FILE *fp = fopen(filename, "r");

	if(fp == NULL)
		err("Can't open %s for reading", filename, 0, 0);

	int n_sources;
	int n_sinks;
	int i, j;
	NodeID id = 0;

	/* read sources and their capacities */
	fscanf(fp, "SOURCES %d\n", &n_sources);
	printf("SOURCES->%d\n", n_sources);
	Node *sources[n_sources];
  float capacity;

	for(i = 0; i < n_sources; ++i) {
		sources[i] = nodes_create(id++);

		fscanf(fp, "%f\n", &capacity);
    node_first_tuples_add(sources[i], tuple_build_source(capacity));
    node_first_tuples_add(sources[i], tuple_build_load(0.0));
		printf("Source capacity: %f\n", capacity);
	}

	/* read sources and their capacities */
	fscanf(fp, "SINKS %d", &n_sinks);
	printf("SINKS->%d\n", n_sinks);

	for(i = 0; i < n_sinks; ++i) {
		Node *sink = nodes_create(id++);

		fscanf(fp, "%f\n", &capacity);
		
    node_first_tuples_add(sink, tuple_build_sink(capacity));
    
		printf("Sink capacity: %f\n", capacity);
		
		node_first_tuples_add(sink, tuple_build_unplugged());
		
		for(j = 0; j < n_sources; ++j)
			node_add_neighbor(sink, sources[j]);
	}

	fclose(fp);
	
	/* print at the end */
	vm_add_finish_function(printWalkGrid);
}
