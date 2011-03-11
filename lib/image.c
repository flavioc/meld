
int cols, rows;
meld_float *pixels = NULL;
meld_float *edge_potential = NULL;
meld_float damping = 0.1;
size_t colors = 5;
static meld_float sigma = 2.0;
static meld_float lambda = 10;
static List *initial_edge_list = NULL;

size_t
get_pixel_id(size_t row, size_t col)
{
	return row * cols + col;
}

meld_float*
get_pixel_ptr(size_t row, size_t col)
{
	return pixels + get_pixel_id(row, col);
}

static inline meld_float
get_pixel(size_t row, size_t col)
{
	return *get_pixel_ptr(row, col);
}

static void
read_image_file(const char *filename)
{
	FILE *fp = fopen(filename, "r");

	if(fp == NULL)
		exit(1);

	int a, b;
	fscanf(fp, "%d %d\n", &a, &b);
	rows = a;
	cols = b;

	pixels = (meld_float*)meld_malloc(sizeof(meld_float)*cols*rows);

	int i, j;
	for(i = 0; i < rows; ++i) {
		for(j = 0; j < cols; ++j) {
			fscanf(fp, "%f", get_pixel_ptr(i, j));
		}
	}
	
	printf("Just read %dx%d pixels\n", (int)rows, (int)cols);
}

static meld_float
expectaction(meld_float *val, int size)
{
	meld_float sum = 0.0;
	meld_float s2 = 0.0;

	int i;
	for(i = 0; i < size; ++i) {
		sum += i * exp(val[i]);
		s2 += exp(val[i]);
	}

	return sum / s2;
}

void
write_results(void)
{
	tuple_type type = type_lookup("belief");
	int i, j;
	for(i = 0; i < rows; ++i) {
		for(j = 0; j < cols; ++j) {
			size_t pixel_id = get_pixel_id(i, j);
			Node *node = nodes_get((NodeID)pixel_id);
			tuple_entry *entry = node->tuples[type].head;
			for(; entry != NULL; entry = entry->next) {
				tuple_t tuple = entry->tuple;
				meld_int iter = MELD_INT(GET_TUPLE_FIELD(tuple, 0));

				if(iter == 1) {
					meld_float val1 = MELD_FLOAT(GET_TUPLE_FIELD(tuple, 1));
					meld_float val2 = *(float *)GET_TUPLE_FIELD(tuple, 2);
					meld_float vec[] = {val1, val2};
					meld_float expc = expectaction(vec, 2);
					
					printf(NODE_FORMAT " %f %f %f\n", node->order, expc, val1, val2);

					*get_pixel_ptr(i, j) = expc;
				}
			}
		}
	}
}

void
normalize_vector(meld_float *vec, int size)
{
	meld_float max_value = vec[0];

	int asg;
	for(asg = 0; asg < size; ++asg) {
		if(vec[asg] > max_value)
			max_value = vec[asg];
	}
	meld_float Z = 0.0;
	for(asg = 0; asg < size; ++asg) {
		Z += exp(vec[asg] -= max_value);
	}
	meld_float logZ = log(Z);
	for(asg = 0; asg < size; ++asg) {
		vec[asg] -= logZ;
	}
}

static inline List*
initial_edge_value(void)
{
	meld_float vec[colors];
	size_t i;
	for(i = 0; i < colors; ++i)
		vec[i] = 0.0;

	normalize_vector(vec, colors);

	List* list = list_float_create();
	for(i = 0; i < colors; ++i)
		list_float_push_tail(list, vec[i]);

	return list;
}

void
vector_print(meld_float *vec, int size)
{
	int i;
	for(i = 0; i < size; ++i) {
		printf("%f ", vec[i]);
	}
	printf("\n");
}

static meld_float*
create_edge_potential(void)
{
	meld_float *data = (meld_float*)meld_malloc(sizeof(meld_float)*colors*colors);

	size_t i;
	size_t j;
	for(i = 0; i < colors; ++i) {
		for(j = 0; j < colors; ++j) {
			data[i + j * colors] = - fabs((meld_float)i - (meld_float)j) * lambda;
		}
	}
	
  return data;
}

static inline tuple_t
tuple_build_edge_val(Node *node, Node *neighbor, List *list)
{
  static meld_int zero = 0.0;
  static tuple_type type = -1;
  
  if(type == -1)
    type = type_lookup("edgeVal");
  
  tuple_t tuple = tuple_alloc(type);
  
  SET_TUPLE_FIELD(tuple, 0, &neighbor);
  SET_TUPLE_FIELD(tuple, 1, &zero);
  SET_TUPLE_FIELD(tuple, 2, &list);
  
  List* route = route_build_direct(node, neighbor);
  SET_TUPLE_FIELD(tuple, 3, &route);
  
  return tuple;
}

static inline tuple_t
tuple_build_potential(List *potential)
{
  tuple_t tuple = tuple_alloc(type_lookup("potential"));
  
  SET_TUPLE_FIELD(tuple, 0, &potential);
  
  return tuple;
}

static inline void
add_neighbor(Node *a, Node *b)
{
  node_add_neighbor(a, b);
  node_first_tuples_add(a, tuple_build_edge_val(a, b, initial_edge_list));
}

static void
config_read(const char *filename)
{
	read_image_file(filename);

	meld_float potential[colors];
	meld_float sigmaSq = sigma * sigma;

	size_t i, j;
	for(i = 0; i < (size_t)rows; ++i) {
		for(j = 0; j < (size_t)cols; ++j) {
			NodeID pixel_id = (NodeID)get_pixel_id(i, j);
			meld_float obs = get_pixel(i, j);

			size_t pred;
			for(pred = 0; pred < colors; ++pred)
				potential[pred] = -(obs - pred)*(obs -pred) / (2.0 * sigmaSq);
			normalize_vector(potential, colors);

      Node *node = nodes_create(pixel_id);
      
      List *potential_list = list_float_create();
      for(pred = 0; pred < colors; ++pred)
  			list_float_push_tail(potential_list, potential[pred]);
  		
      node_first_tuples_add(node, tuple_build_potential(potential_list));
		}
	}
	
	edge_potential = create_edge_potential();
  
  initial_edge_list = initial_edge_value();

	/* add neighbors */
	for(i = 0; i < (size_t)rows; ++i) {
		for(j = 0; j < (size_t)rows; ++j) {
			size_t pixel_id = get_pixel_id(i, j);
			Node *this = nodes_get((NodeID)pixel_id);

			if(i-1 < (size_t)rows) {
				size_t neid = get_pixel_id(i-1, j);
				Node *other = nodes_get((NodeID)neid);
				add_neighbor(this, other);
			}
			if(i+1 < (size_t)rows) {
				size_t neid = get_pixel_id(i+1, j);
				Node *other = nodes_get((NodeID)neid);
				add_neighbor(this, other);
			}
			if(j-1 < (size_t)cols) {
				size_t neid = get_pixel_id(i, j-1);
				Node *other = nodes_get((NodeID)neid);
				add_neighbor(this, other);
			}
			if(j+1 < (size_t)cols) {
				size_t neid = get_pixel_id(i, j+1);
				Node *other = nodes_get((NodeID)neid);
				add_neighbor(this, other);
			}
		}
	}
}

void
write_output(void)
{
	FILE *fp = fopen("coiso.pgm", "w");
	fprintf(fp, "P2\n%d %d\n255\n", cols, rows);

	double min = pixels[0];
	double max = pixels[0];
	int i;
	for(i = 0; i < rows * cols; ++i) {
			if(pixels[i] < min)
				min = pixels[i];
			if(pixels[i] > max)
				max = pixels[i];
	}
	printf("min %f max %f\n", min, max);

	int color;
	int j;
	for(i = 0; i < rows; ++i) {
		for(j = 0; j < cols; ++j) {
			if(min != max)
				color = (int)(255.0 * (get_pixel(i, j) - min)/(max - min));
			else
				color = min;
			fprintf(fp, "%d", color);
			if(j != cols - 1)
				fprintf(fp, "\t");
		}
		fprintf(fp, "\n");
	}
}
