# 처음 사용자용 가이드

이 문서는 ARCUS를 처음 접하는 C 개발자를 위해 작성되었습니다. 자세한 설명보다는 예제를 통해 바로 ARCUS를 사용해볼 수 있는 내용으로 구성되어 있습니다.

## ARCUS

ARCUS는 오픈소스 key-value 캐시 서버인 memcached를 기반으로 부분적으로 fault-tolerant한 메모리 기반의 캐시 클라우드 입니다.

- memcached : 구글, 페이스북 등에서 대규모로 사용하고 있는 메모리 캐시 서버입니다.
- 캐시 : 자주 사용되는 데이터를 비교적 고속의 저장소에 넣어둠으로써, 느린 저장소로의 요청을 줄이고 보다 빠른 응답성을 기대할 수 있게 하는 서비스입니다.
- 메모리 기반 : ARCUS는 데이터를 메모리에만 저장합니다. 따라서 모든 데이터는 휘발성이며 언제든지 삭제될 수 있습니다.
- 클라우드 : 각 서비스는 필요에 따라 전용 캐시 클러스터를 구성할 수 있으며 동적으로 캐시 서버를 추가하거나 삭제할 수 있습니다. (단, 일부 데이터는 유실됩니다)
- fault-tolerant : ARCUS는 일부 또는 전체 캐시 서버의 이상 상태를 감지하여 적절한 조치를 취합니다.

또한 ARCUS는 key-value 형태의 데이터뿐만 아니라 List, Set, Map, B+Tree 등의 자료구조를 저장할 수 있는 기능을 제공합니다.

## 미리 알아두기

- 키(key)
  - ARCUS의 key는 prefix와 subkey로 구성되며, prefix와 subkey는 콜론(:)으로 구분됩니다. (예) users:user_12345
  - ARCUS는 prefix를 기준으로 별도의 통계를 수집합니다. prefix 개수의 제한은 없으나 통계 수집을 하는 경우에는 너무 많지 않는 수준(5~10개)으로 생성하시는 것을 권합니다.
  - 키는 prefix, subkey를 포함하여 4000자를 넘을 수 없습니다. 따라서 응용에서 키 길이를 제한하셔야 합니다.
- 값(value)
  - 하나의 키에 대한 값은 char 배열 형태로 최대 1MB 까지 저장될 수 있습니다.

## Hello, ARCUS!

로컬 환경에서 11211 포트로 통신 가능한 Arcus-memcached가 하나 구성되어 있다고 가정합니다.
B+Tree 키를 생성하고 요소를 삽입한 뒤 조회하는 간단한 예제를 작성합니다.

```c
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

컴파일 시에는 ARCUS 클라이언트가 설치된 경로를 지정합니다.

```sh
gcc -o sample sample.c -Wall -I/install/directory/include -L/install/directory/lib -lmemcached -lmemcachedutil
```

실행 전에는 LD_LIBRARY_PATH에 라이브러리 경로를 포함시키고 프로그램을 실행합니다.

```sh
$ LD_LIBRARY_PATH=/install/directory/lib ./sample
Created a b+tree key and inserted an element.
Retrieved the element. value=helloworld
```
