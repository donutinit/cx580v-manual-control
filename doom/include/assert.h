#ifndef CXD_ASSERT_H
#define CXD_ASSERT_H
void cxd_assert_fail(const char *, const char *, int);
#define assert(x) ((x) ? (void)0 : cxd_assert_fail(#x, __FILE__, __LINE__))
#endif
