AX_WITH_PROG(MEMCACHED_ENGINE,memcached_engine)
ac_cv_path_MEMCACHED_ENGINE=$MEMCACHED_ENGINE
AS_IF([test -f "$ac_cv_path_MEMCACHED_ENGINE"],
      [
        AC_DEFINE([HAVE_MEMCACHED_ENGINE], [1], [If Memcached engine object is available])
        AC_DEFINE_UNQUOTED([MEMCACHED_ENGINE], "$ac_cv_path_MEMCACHED_ENGINE", [Name of the memcached engine object used in make test])
       ],
       [
        AC_DEFINE([HAVE_MEMCACHED_ENGINE], [0], [If Memcached engine object is available])
        AC_DEFINE([MEMCACHED_ENGINE], [0], [Name of the memcached engine object used in make test])
      ])
