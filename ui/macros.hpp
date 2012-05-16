
#ifndef UI_MACROS_HPP
#define UI_MACROS_HPP

#define UI_ADD_FIELD(OBJ, P1, P2) \
	(OBJ).push_back(Pair(P1, P2))
#define UI_ADD_ELEM(OBJ, P) \
	(OBJ).push_back(P)
#define UI_YES Value("yes")
#define UI_NIL Value()
	
#endif
