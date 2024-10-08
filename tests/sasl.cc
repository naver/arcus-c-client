/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Libmemcached library
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

#include <config.h>
#include <libtest/test.hpp>

using namespace libtest;

/*
  Test cases
*/

#define TEST_PORT_BASE MEMCACHED_DEFAULT_PORT +20
#include <libmemcached/memcached.h>

static test_return_t pre_sasl(memcached_st *)
{
  if (LIBMEMCACHED_WITH_SASL_SUPPORT == 0)
  {
    return TEST_SKIPPED;
  }

  return TEST_SUCCESS;
}

/*
 * Test that the sasl authentication works. We cannot use the default
 * pool of servers, because that would require that all servers we want
 * to test supports SASL authentication, and that they use the default
 * creds.
 */
static test_return_t sasl_auth_test(memcached_st *memc)
{
  if (LIBMEMCACHED_WITH_SASL_SUPPORT)
  {
    test_compare(MEMCACHED_SUCCESS, memcached_set(memc, "foo", 3, "bar", 3, (time_t)0, (uint32_t)0));
    test_compare(MEMCACHED_SUCCESS, memcached_delete(memc, "foo", 3, 0));
    test_compare(MEMCACHED_SUCCESS, memcached_destroy_sasl_auth_data(memc));
    test_compare(MEMCACHED_SUCCESS, memcached_destroy_sasl_auth_data(memc));
    test_compare(MEMCACHED_INVALID_ARGUMENTS, memcached_destroy_sasl_auth_data(NULL));
    memcached_quit(memc);

    test_compare(MEMCACHED_AUTH_FAILURE,
                 memcached_set(memc, "foo", 3, "bar", 3, (time_t)0, (uint32_t)0));
    test_compare(MEMCACHED_SUCCESS, memcached_destroy_sasl_auth_data(memc));

    memcached_quit(memc);
    return TEST_SUCCESS;
  }

  return TEST_SKIPPED;
}


test_st sasl_auth_tests[]= {
  {"sasl_auth", true, (test_callback_fn*)sasl_auth_test },
  {0, 0, (test_callback_fn*)0}
};

collection_st collection[] ={
  {"sasl_auth", (test_callback_fn*)pre_sasl, 0, sasl_auth_tests },
#if 0
  {"sasl", (test_callback_fn*)pre_sasl, 0, tests },
#endif
  {0, 0, 0, 0}
};

#include "tests/libmemcached_world.h"

void get_world(Framework *world)
{
  world->collections= collection;

  world->_create= (test_callback_create_fn*)world_create;
  world->_destroy= (test_callback_destroy_fn*)world_destroy;

  world->item._startup= (test_callback_fn*)world_test_startup;
  world->item.set_pre((test_callback_fn*)world_pre_run);
  world->item.set_flush((test_callback_fn*)world_flush);
  world->item.set_post((test_callback_fn*)world_post_run);
  world->_on_error= (test_callback_error_fn*)world_on_error;

  world->collection_startup= (test_callback_fn*)world_container_startup;
  world->collection_shutdown= (test_callback_fn*)world_container_shutdown;

  world->set_runner(&default_libmemcached_runner);

  world->set_sasl("memcached", "memcached");
}
