This is an experimental C client library for Arcus memcached with
replication support.  Replication takes place at the server side using
a master-slave approach.  It is transparent to the client.  There are
no changes to the Arcus client API.

However, the ZooKeeper based clustering mechanism changes somewhat.
It uses a tree structure different from the previous Arcus version to
expose the master servers in the cluster.  The client code needs to
understand this new tree structure and parse information
appropriately.

Below is the original README.md from the master branch.  Everything
still applies.

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

The use of ZooKeeper based clustering is optional.  To enable it, use
--enable-zk-integration along with --with-zookeeper when running configure.
Make sure to use the ZooKeeper library with Arcus modifications.

## Test cases

libmemcached includes a number of test cases in the tests directory.  Arcus
specific test cases have been added to tests/mem_functions.cc.  To run test
cases, specify the memcached binary and the engine.  Here is an example.

    ./configure --prefix=/home1/hyongyoub_kim/openarcus \
                --with-memcached=/home1/openarcus/bin/memcached \
                --with-memcached_engine=/home1/openarcus/lib/default_engine.so
    make
    make test
    
    [...]
    PASS: tests/c_sasl_test
    ===================
    All 23 tests passed
    ===================
    Tests completed

## ZooKeeper-based clustering

Make sure to install the ZooKeeper C library from arcus-zookeeper.  Then,
compile this Arcus C library using --enable-zk-integration and --with-zookeeper.

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
dataDir=/home1/openarcus/zookeeper_data
# the port at which the clients will connect
clientPort=2181
maxClientCnxns=200
```

This script creates ZooKeeper nodes for two memcached instances: one at at
localhost:11211 and the other at localhost:11212.
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

## API Documentation

As is, documentation is minimal.  It definitely needs improvements.

The original libmemcached has man pages (see docs/man).  `make install` copies
these man pages to the target directory.  Arcus APIs do not have man pages.
Instead, doxygen-style comments are added directly into header files.
See the following files.

- libmemcached/arcus.h: ZooKeeper-based clustering
- libmemcached/collection.h: collections API
- libmemcached/util/pool.h: a few Arcus specific utility functions


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
