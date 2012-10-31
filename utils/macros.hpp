
#ifndef UTILS_MACROS_HPP
#define UTILS_MACROS_HPP

#define CONCAT_IMPL(x, y) x ## y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

#endif
