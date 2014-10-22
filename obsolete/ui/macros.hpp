
#ifndef UI_MACROS_HPP
#define UI_MACROS_HPP

#define UI_ADD_FIELD(OBJ, P1, P2) \
	(OBJ).push_back(Pair(P1, P2))
#define UI_ADD_ELEM(OBJ, P) \
	(OBJ).push_back(P)
#define UI_ADD_NODE_FIELD(OBJ, NODE)	\
	UI_ADD_FIELD(OBJ, "node", (int)((NODE)->get_id()))
#define UI_ADD_TUPLE_FIELD(OBJ, TPL)	\
	UI_ADD_FIELD(OBJ, "tuple", (TPL)->dump_json())
#define UI_YES Value("yes")
#define UI_NIL Value()
	
#endif
