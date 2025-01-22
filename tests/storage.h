#ifndef __TESTS_STORAGE_H__
#define __TESTS_STORAGE_H__

#ifdef  __cplusplus
extern "C" {
#endif

LIBTEST_LOCAL
test_return_t mset_and_get_test(memcached_st *mc);

LIBTEST_LOCAL
test_return_t madd_and_get_test(memcached_st *mc);

LIBTEST_LOCAL
test_return_t mreplace_and_get_test(memcached_st *mc);

LIBTEST_LOCAL
test_return_t mprepend_and_get_test(memcached_st *mc);

LIBTEST_LOCAL
test_return_t mappend_and_get_test(memcached_st *mc);

LIBTEST_LOCAL
test_return_t mcas_and_get_test(memcached_st *mc);

#ifdef  __cplusplus
}
#endif

#endif /* __TESTS_STORAGE_H__ */
