
#ifndef UTILS_MACROS_HPP
#define UTILS_MACROS_HPP

#define CONCAT_IMPL(x, y) x ## y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#define DEFINE_PADDING utils::byte MACRO_CONCAT(_pad_, __COUNTER__)[128]
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

#endif