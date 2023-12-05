## arcus-c-client: Arcus C Client

This is the C client library for Arcus memcached.  It is based on
libmemcached 0.53.  Extensive changes are made to support Arcus
collection API and ZooKeeper based clustering.

The library assumes Arcus memcached, and the collection API (list,
set, b+tree) is built in.

Github project page:
https://github.com/naver/arcus

## Build on Linux

Make sure to install auto tools such as autoheader.

    ./config/autorun.sh
    ./configure --prefix=/install/directory
    make
    make install

## ZooKeeper-based clustering

The use of ZooKeeper based clustering is optional.  To enable it, use
`--enable-zk-integration` along with `--with-zookeeper` when running configure.
Make sure to install the ZooKeeper C library from arcus-zookeeper.

Set up a ZooKeeper ensemble and a directory structure for memcached
instances.  For instance, the following shows the configuration for
a single-server ZooKeeper ensemble listening at port 2181.
```
$ cat test-zk.conf
# The number of milliseconds of each tick
tickTime=2000
# The number of ticks that the initial
# synchronization phase can take
initLimit=10
# The number of ticks that can pass between
# sending a request and getting an acknowledgement
syncLimit=5
# the directory where the snapshot is stored.
dataDir=/home1/arcus/zookeeper_data
# the port at which the clients will connect
clientPort=2181
maxClientCnxns=200
```

This script creates ZooKeeper nodes for two memcached instances: one at localhost:11211 and the other at localhost:11212.
```
$ cat setup-test-zk.bash
ZK_CLI="./zookeeper/bin/zkCli.sh"
ZK_ADDR="-server localhost:2181"

$ZK_CLI $ZK_ADDR create /arcus 0
$ZK_CLI $ZK_ADDR create /arcus/cache_list 0
$ZK_CLI $ZK_ADDR create /arcus/cache_list/test 0
$ZK_CLI $ZK_ADDR create /arcus/client_list 0
$ZK_CLI $ZK_ADDR create /arcus/client_list/test 0
$ZK_CLI $ZK_ADDR create /arcus/cache_server_mapping 0
$ZK_CLI $ZK_ADDR create /arcus/cache_server_log 0
$ZK_CLI $ZK_ADDR create /arcus/cache_server_mapping/127.0.0.1:11211 0
$ZK_CLI $ZK_ADDR create /arcus/cache_server_mapping/127.0.0.1:11211/test 0
$ZK_CLI $ZK_ADDR create /arcus/cache_server_mapping/127.0.0.1:11212 0
$ZK_CLI $ZK_ADDR create /arcus/cache_server_mapping/127.0.0.1:11212/test 0
```

To connect to the ZooKeeper ensemble, call one of the Arcus specific
startup functions as follows.
```
arcus/multi_threaded.c:
arcus_pool_connect(pool, "localhost:2181", "test");

arcus/multi_process.c:
arcus_proxy_create(proxy_mc, "localhost:2181", "test");
```

## Quick start helloworld example

The following example code (arcus/sample.c) uses one memcached instance running on the local machine at port 11211.
It simply creates a b+tree key, inserts an element, and then retrieves it from the server.

```
#include <stdlib.h>
#include <stdio.h>

#include "libmemcached/memcached.h"

/* No error checking/handling to minimize clutter. */

int
main(int argc, char *argv[])
{
  memcached_st *mc;
  memcached_coll_create_attrs_st attr;
  const char *key = "this_is_key";
  size_t key_length = strlen(key);
  const uint64_t bkey = 123; /* b+tree element's key */
  const char *value = "helloworld";
  memcached_coll_result_st result;

  /* Create the memcached object */
  if (NULL == (mc = memcached_create(NULL)))
    return -1;

  /* Add the server's address */
  if (MEMCACHED_SUCCESS != memcached_server_add(mc, "127.0.0.1", 11211))
    return -1;

  /* Create a b+tree key and then insert an element in one call. */
  memcached_coll_create_attrs_init(&attr, 20 /* flags */, 100 /* exptime */,
    4000 /* maxcount */);
  if (MEMCACHED_SUCCESS != memcached_bop_insert(mc, key, key_length,
      bkey,
      NULL /* eflag */, 0 /* eflag length */,
      (const char*)value, (size_t)strlen(value)+1 /* include NULL */,
      &attr /* automatically create the b+tree key */))
    return -1;
  printf("Created a b+tree key and inserted an element.\n");

  /* Get the element */
  if (NULL == memcached_coll_result_create(mc, &result))
    return -1;
  if (MEMCACHED_SUCCESS != memcached_bop_get(mc, key, key_length, bkey,
      NULL /* no eflags filters */,
      false /* do not delete the element */,
      false /* do not delete the empty key */,
      &result))
    return -1;
  
  /* Print */
  printf("Retrieved the element. value=%s\n",
    memcached_coll_result_get_value(&result, 0));
  memcached_coll_result_free(&result);
  
  return 0;
}
```

To compile, specify the include and library paths and link against this library.

```
gcc -o sample sample.c -Wall -I/install/directory/include -L/install/directory/lib -lmemcached -lmemcachedutil
```

Then start the memcached instance.
```
$ cd /install/directory/bin
$ ./memcached -p 11211 -E ../lib/default_engine.so -v
Loaded engine: Default engine v0.1
Supplying the following features: LRU, compare and swap

```

Finally run the sample. Make sure to include the path to the C library in LD_LIBRARY_PATH.
```
$ LD_LIBRARY_PATH=/install/directory/lib ./sample
Created a b+tree key and inserted an element.
Retrieved the element. value=helloworld
```

## API Documentation

Please refer to [Arcus C Client User Guide](docs/arcus-c-client-user-guide.md)
for the detailed usage of Arcus C client.

The original libmemcached has man pages (see docs/man).  `make install` copies
these man pages to the target directory.  Arcus APIs do not have man pages.
Instead, doxygen-style comments are added directly into header files.
See the following files.

- libmemcached/arcus.h: ZooKeeper-based clustering
- libmemcached/collection.h: collections API
- libmemcached/collection_result.h: collection results API
- libmemcached/util/pool.h: a few Arcus specific utility functions

## Test cases

libmemcached includes a number of test cases in the tests directory.  Arcus
specific test cases have been added to tests/mem_functions.cc.  To run test
cases, specify the memcached binary and the engine.  Here is an example.
Test cases currently do not work with ZooKeeper-based clustering.  Do not
use --enable-zk-integration when running configure.

    ./configure --prefix=/home1/arcus \
                --with-memcached=/home1/arcus/bin/memcached \
                --with-memcached_engine=/home1/arcus/lib/default_engine.so
    make
    make test
    
    [...]
    PASS: tests/c_sasl_test
    ===================
    All 23 tests passed
    ===================
    Tests completed

If any problem exists in test cases, please refer to [test FAQ](/docs/test_faq.md).

## Issues

If you find a bug, please report it via the GitHub issues page.

https://github.com/naver/arcus-c-client/issues

## Arcus Contributors

In addition to those who had contributed to the original libmemcached, the
following people at NAVER have contributed to arcus-c-client.

Hoonmin Kim (harebox) <hoonmin.kim@navercorp.com>; <harebox@gmail.com>  
YeaSol Kim (ngleader) <sol.k@navercorp.com>; <ngleader@gmail.com>  
HyongYoub Kim <hyongyoub.kim@navercorp.com>  

## License

Licensed under the Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0

## Patents

Arcus has patents on b+tree smget operation.
Refer to PATENTS file in this directory to get the patent information.

Under the Apache License 2.0, a perpetual, worldwide, non-exclusive,
no-charge, royalty-free, irrevocable patent license is granted to any user for any usage.
You can see the specifics on the grant of patent license in LICENSE file in this directory.
