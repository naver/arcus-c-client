#!/usr/bin/env bash
#
# arcus-c-client : Arcus C client
# Copyright 2010-2014 NAVER Corp.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# LibmemcacheD
# Copyright (C) 2011 Data Differential, http://datadifferential.com/
# Copyright (C) 2006-2010 Brian Aker, Monty Taylor, Trond Norbye
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

m4_include([m4/version.m4])
AC_INIT([libmemcached],[VERSION_NUMBER],[http://libmemcached.org/])

AC_CONFIG_AUX_DIR(config)

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE

AC_CANONICAL_HOST
AC_CANONICAL_BUILD

AC_PREREQ([2.61])

AC_CONFIG_MACRO_DIR(m4)

AC_CONFIG_HEADERS([config.h:config.in])dnl Keep filename to 8.3 for MS-DOS.

AC_CONFIG_SRCDIR([libmemcached/memcached.cc])

PANDORA_CANONICAL_TARGET(no-vc-changelog)
AC_CHECK_PROGS([YACC], ['bison'], [:])
AC_CHECK_PROGS([LEX], ['flex'], [:])

#shared library versioning
MEMCACHED_UTIL_LIBRARY_VERSION=2:0:0
MEMCACHED_PROTOCAL_LIBRARY_VERSION=0:0:0
MEMCACHED_LIBRARY_VERSION=8:0:0
#                         | | |
#                  +------+ | +---+
#                  |        |     |
#                 current:revision:age
#                  |        |     |
#                  |        |     +- increment if interfaces have been added
#                  |        |        set to zero if interfaces have been removed or changed
#                  |        +- increment if source code has changed
#                  |           set to zero if current is incremented
#                  +- increment if interfaces have been added, removed or changed
AC_SUBST(MEMCACHED_UTIL_LIBRARY_VERSION)
AC_SUBST(MEMCACHED_PROTOCAL_LIBRARY_VERSION)
AC_SUBST(MEMCACHED_LIBRARY_VERSION)


HASHKIT_LIBRARY_VERSION=1:0:0
AC_SUBST(HASHKIT_LIBRARY_VERSION)

LT_PREREQ([2.2])
LT_INIT()
LT_LANG([C++])
LT_LANG([C])
gl_VISIBILITY

m4_include([m4/memcached.m4])
m4_include([m4/memcached_sasl.m4])
m4_include([m4/memcached_engine.m4])
m4_include([m4/gearmand.m4])

AM_CONDITIONAL(BUILDING_LIBMEMCACHED, true)
AM_CONDITIONAL(HAVE_LIBMEMCACHED, false)
AC_DEFINE([HAVE_LIBMEMCACHED], [1], [Enables libmemcached Support])

AM_CONDITIONAL(BUILDING_GEARMAN, false)
AM_CONDITIONAL(HAVE_LIBGEARMAN, false)
AC_DEFINE([HAVE_LIBGEARMAN], [0], [Enables libgearman Support])

AC_SEARCH_LIBS(getopt_long, gnugetopt)
AC_SEARCH_LIBS(gethostbyname, nsl)

PANDORA_HAVE_LIBEVENT

case "$target_os" in
  *linux*)
       AS_IF([test "x$GCC" = "xyes"],
             [
              LDFLAGS="$LDFLAGS -z relro -z now"
              ])
   ;;
   esac

dnl Specialty checks
AX_PTHREAD
CONFIG_EXTRA
DETECT_BYTEORDER
ENABLE_UTILLIB
SETSOCKOPT_SANITY
ENABLE_HSIEH_HASH
ENABLE_MURMUR_HASH
ENABLE_FNV64_HASH
ENABLE_MEMASLAP
PROTOCOL_BINARY_TEST
ENABLE_DEPRECATED
PANDORA_HAVE_LIBINNODB
PANDORA_HAVE_SASL

AC_CHECK_FUNCS([alarm])
AC_CHECK_FUNCS([dup2])
AC_CHECK_FUNCS([getline])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([memchr])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([setenv])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([sqrt])
AC_CHECK_FUNCS([strcasecmp])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([strtoul])
AC_CHECK_FUNCS([strtoull])

AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sasl/sasl.h])
AC_CXX_HEADER_STDCXX_98

AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

dnl The sasl functions should only be visible if we build with sasl support
AS_IF([test "x$ac_cv_sasl" = "xyes"],
      [
        [ LIBMEMCACHED_WITH_SASL_SUPPORT="#define LIBMEMCACHED_WITH_SASL_SUPPORT 1" ]
      ],
      [
        [ LIBMEMCACHED_WITH_SASL_SUPPORT="#define LIBMEMCACHED_WITH_SASL_SUPPORT 0" ]
       ]
      )
AC_SUBST(LIBMEMCACHED_WITH_SASL_SUPPORT)

dnl ----------------------------------------------------------------------------
dnl Arcus zookeeper integration
AC_ARG_ENABLE(zk-integration,
  [AS_HELP_STRING([--enable-zk-integration],[Enable Arcus cache cluster with zookeeper integration])])

AS_IF([test "x$enable_zk_integration" = "xyes"],
      [LIBMEMCACHED_WITH_ZK_INTEGRATION="#define LIBMEMCACHED_WITH_ZK_INTEGRATION 1"])
AC_SUBST(LIBMEMCACHED_WITH_ZK_INTEGRATION)
AS_IF([test "x$enable_zk_integration" = "xyes"],
      [ARCUS_CLUSTER_API_ENABLED="#define ARCUS_CLUSTER_API_ENABLED 1"])
AC_SUBST(ARCUS_CLUSTER_API_ENABLED)

AC_DEFINE([ARCUS_VERSION_STRING],["VERSION_NUMBER"],[Arcus Version String])

if test "x$enable_zk_integration" = "xyes"; then
  tryzookeeperdir=""
  AC_ARG_WITH(zookeeper,
      [  --with-zookeeper[=DIR]     search for zookeeper in DIR/include and DIR/lib ],
      [
        if test "x$withval" != "xno" ; then
            tryzookeeperdir=$withval
        fi
      ]
  )

  AC_CACHE_CHECK([for zookeeper directory], ac_cv_zookeeper_dir, [
    saved_LIBS="$LIBS"
    saved_LDFLAGS="$LDFLAGS"
    saved_CPPFLAGS="$CPPFLAGS"
    zk_found=no
    for zkdir in $tryzookeeperdir "" $prefix /usr/local ; do
      LDFLAGS="$saved_LDFLAGS"
      LIBS="$saved_LIBS -lzookeeper_mt"

      # Skip the directory if it isn't there.
      if test ! -z "$zookeeperdir" -a ! -d "$zkdir" ; then
         continue;
      fi
      if test ! -z "$zkdir" ; then
        if test -d "$zkdir/lib" ; then
          LDFLAGS="-L$zkdir/lib $LDFLAGS"
        else
          LDFLAGS="-L$zkdir $LDFLAGS"
        fi
        # ZooKeeper 3.3
        if test -d "$zkdir/include/c-client-src" ; then
          CPPFLAGS="-I$zkdir/include/c-client-src $CPPFLAGS"
        else
          # ZooKeeper 3.4 installs headers under include/zookeeper
          if test -d "$zkdir/include/zookeeper" ; then
            CPPFLAGS="-I$zkdir/include/zookeeper $CPPFLAGS"
          else
            CPPFLAGS="-I$zkdir $CPPFLAGS"
          fi
        fi
      fi
      if test ! -z "$zkdir" ; then
        ac_cv_zookeeper_dir=$zkdir
      else
        ac_cv_zookeeper_dir="(system)"
      fi

      AC_TRY_COMPILE([
        #include <zookeeper.h>
      ],[
        zoo_set_debug_level (ZOO_LOG_LEVEL_WARN);
      ],[
        zk_found=yes
      ],[
        zk_found=no
      ])
      if test $zk_found = yes ; then
        break
      fi
    done
    LIBS="$saved_LIBS"
    LDFLAGS="$saved_LDFLAGS"
    CPPFLAGS="$saved_CPPFLAGS"
    if test $zk_found = no ; then
      AC_MSG_ERROR([If it's already installed, specify its path using --with-zookeeper=/dir/])
    fi
  ])
  LIBS="$LIBS -lzookeeper_mt"
  if test $ac_cv_zookeeper_dir != "(system)"; then
    if test -d "$ac_cv_zookeeper_dir/lib" ; then
      LDFLAGS="-L$ac_cv_zookeeper_dir/lib $LDFLAGS"
      le_libdir="$ac_cv_zookeeper_dir/lib"
    else    LDFLAGS="-L$ac_cv_zookeeper_dir $LDFLAGS"
      le_libdir="$ac_cv_zookeeper_dir"
    fi
    # ZooKeeper 3.3
    if test -d "$ac_cv_zookeeper_dir/include/c-client-src" ; then
      CPPFLAGS="-I$ac_cv_zookeeper_dir/include/c-client-src $CPPFLAGS"
    else
      # ZooKeeper 3.4
      if test -d "$ac_cv_zookeeper_dir/include/zookeeper" ; then
        CPPFLAGS="-I$ac_cv_zookeeper_dir/include/zookeeper $CPPFLAGS"
      else
        if test -d "$ac_cv_zookeeper_dir/include" ; then
          CPPFLAGS="-I$ac_cv_zookeeper_dir/include $CPPFLAGS"
        else
          # Default
          CPPFLAGS="-I$ac_cv_zookeeper_dir $CPPFLAGS"
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL([BUILD_ARCUS_ZK_INTEGRATION],[test "$enable_zk_integration" = "yes"])

AC_CHECK_HEADERS([atomic.h])
AS_IF([test "x$ac_cv_header_atomic_h" = "xyes"],[
      AC_CHECK_FUNCS(atomic_add_64)
      AC_CHECK_FUNCS(atomic_add_32)
      AS_IF([test "x$ac_cv_func_atomic_add_64" = "xyes" -a "x$ac_cv_func_atomic_add_32" = "xyes"],[
            AC_DEFINE([USE_ATOMIC_H],
	              [1],
                      [Define to true if you want to use functions from atomic.h])])])

AC_CHECK_HEADERS_ONCE(winsock2.h poll.h sys/wait.h fnmatch.h)
AM_CONDITIONAL(BUILD_POLL, test "x$ac_cv_header_poll_h" = "xno")
AM_CONDITIONAL(BUILD_WIN32_WRAPPERS, test "x$ac_cv_header_winsock2_h" = "xyes")
AS_IF(test "x$ac_cv_header_winsock2_h" = "xyes",
      [AM_LDFLAGS="$AM_LDFLAGS -lws2_32"
       AM_CFLAGS="$AM_CFLAGS $NO_WERROR"
       AM_CXXFLAGS="$AM_CXXFLAGS $NO_WERROR"
      ])
DETECT_EAGAIN
SOCKET_SEND_FLAGS

AC_CONFIG_FILES([
  Makefile
  docs/conf.py
  libhashkit/configure.h
  libmemcached/configure.h
  libmemcached/arcus.h
  support/libmemcached.pc
  support/libmemcached.spec
  support/libmemcached-fc.spec
  libtest/version.h
  ])
AC_OUTPUT

echo "---"
echo "Configuration summary for $PACKAGE_NAME version $VERSION"
echo ""
echo "   * Installation prefix:       $prefix"
echo "   * System type:               $host_vendor-$host_os"
echo "   * Host CPU:                  $host_cpu"
echo "   * C Compiler:                $CC_VERSION"
echo "   * Assertions enabled:        $ac_cv_assert"
echo "   * Debug enabled:             $with_debug"
echo "   * Warnings as failure:       $ac_cv_warnings_as_errors"
echo ""
echo "---"

case "$host_os" in
  *freebsd*)
    echo "*****"
    echo "*"
    echo "*  NOTE: You are on FreeBSD. BSD make will not work."
    echo "*"
    echo "*   use 'gmake' To build libmemcached"
    echo "*"
    echo "*****"
  ;;
esac

