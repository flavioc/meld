
static void
config_read(const char *filename)
{
  (void)filename;
  
  NodeID i;
  for(i = 0; i < (50*20); ++i) {
    nodes_create(i);
  }
}
