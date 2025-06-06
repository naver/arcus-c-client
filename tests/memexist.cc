/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Test memexist
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


/*
  Test that we are cycling the servers we are creating during testing.
*/

#include <config.h>

#include <libtest/test.hpp>
#include <libmemcached/memcached.h>

using namespace libtest;

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

static std::string executable;

static test_return_t quiet_test(void *)
{
  const char *args[]= { "--quiet", 0 };

  test_true(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

static test_return_t help_test(void *)
{
  const char *args[]= { "--quiet", "--help", 0 };

  test_true(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

static test_return_t exist_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--server=localhost:%d", int(default_port()));
  const char *args[]= { "--quiet", buffer, "foo", 0 };

  memcached_st *memc= memcached(buffer, strlen(buffer));
  test_true(memc);

  test_compare(MEMCACHED_SUCCESS,
               memcached_set(memc, test_literal_param("foo"), 0, 0, 0, 0));

  memcached_return_t rc;
  test_null(memcached_get(memc, test_literal_param("foo"), 0, 0, &rc));
  test_compare(MEMCACHED_SUCCESS, rc);

  test_true(exec_cmdline(executable, args));

  test_null(memcached_get(memc, test_literal_param("foo"), 0, 0, &rc));
  test_compare(MEMCACHED_SUCCESS, rc);

  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t NOT_FOUND_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--server=localhost:%d", int(default_port()));
  const char *args[]= { "--quiet", buffer, "foo", 0 };

  memcached_st *memc= memcached(buffer, strlen(buffer));
  test_true(memc);

  test_compare(MEMCACHED_SUCCESS, memcached_flush(memc, 0));

  memcached_return_t rc;
  test_null(memcached_get(memc, test_literal_param("foo"), 0, 0, &rc));
  test_compare(MEMCACHED_NOTFOUND, rc);

  test_true(exec_cmdline(executable, args));

  test_null(memcached_get(memc, test_literal_param("foo"), 0, 0, &rc));
  test_compare(MEMCACHED_NOTFOUND, rc);

  memcached_free(memc);

  return TEST_SUCCESS;
}

test_st memexist_tests[] ={
  {"--quiet", true, quiet_test },
  {"--help", true, help_test },
  {"exist(FOUND)", true, exist_test },
  {"exist(NOT_FOUND)", true, NOT_FOUND_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"memexist", 0, 0, memexist_tests },
  {0, 0, 0, 0}
};

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (HAVE_MEMCACHED_BINARY == 0)
  {
    error= TEST_FATAL;
    return NULL;
  }

  const char *argv[1]= { "memexist" };
  if (not server_startup(servers, "memcached", MEMCACHED_DEFAULT_PORT +10, 1, argv))
  {
    error= TEST_FAILURE;
  }

  return &servers;
}


void get_world(Framework *world)
{
  executable= "./clients/memexist";
  world->collections= collection;
  world->_create= world_create;
}

