# Key-Value Item

Key-value item은 하나의 key에 대해 하나의 value만을 저장하는 item이다.

**제약조건**
- Key의 최대 크기는 4000 character이다.
- Cache item의 최대 크기는 1MB이다.

Key-value item에 대해 수행 가능한 연산들은 아래와 같다.

- [Key-Value Item 저장](03-key-value-API.md#key-value-item-%EC%A0%80%EC%9E%A5)
- [Key-Value Item 조회](03-key-value-API.md#key-value-item-%EC%A1%B0%ED%9A%8C)
- [Key-Value Item 값의 증감](03-key-value-API.md#key-value-item-%EA%B0%92%EC%9D%98-%EC%A6%9D%EA%B0%90)
- [Key-Value Item 삭제](03-key-value-API.md#key-value-item-%EC%82%AD%EC%A0%9C)

## Key-Value Item 저장

key-value item을 저장하는 API로 set, add, replace, prepend/append가 있다.

새로운 아이템을 저장하거나 기존 아이템을 교체하는 API는 다음과 같다.
- memcached_set: 주어진 key에 value를 저장한다.
- memcached_add: 주어진 key가 존재하지 않을 경우에만 value를 저장한다.
- memcached_replace: 주어진 key가 존재하는 경우에만 value를 저장한다.

```c
memcached_return_t
memcached_set(memcached_st *ptr,
              const char *key, size_t key_length,
              const char *value, size_t value_length,
              time_t expiration, uint32_t flags)
memcached_return_t
memcached_add(memcached_st *ptr,
              const char *key, size_t key_length,
              const char *value, size_t value_length,
              time_t expiration, uint32_t flags)
memcached_return_t
memcached_replace(memcached_st *ptr,
              const char *key, size_t key_length,
              const char *value, size_t value_length,
              time_t expiration, uint32_t flags)
```

대표적으로 set을 수행하는 예시는 다음과 같다.

```c
int arcus_kv_store(memcached_st *memc)
{
  const char *key= "item:a_key";
  const char *value= "value";
  uint32_t exptime= 600;
  uint32_t flags= 0;
  memcached_return_t rc;

  rc= memcached_set(memc, key, strlen(key), value, strlen(value), exptime, flags);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_set: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  return 0;
}
```

존재하는 아이템의 value에 데이터를 추가시키는 API는 다음과 같다.
- memcached_prepend: 주어진 key의 value에 새로운 데이터를 prepend한다.
- memcached_append: 주어진 key의 value에 새로운 데이터를 append한다.

```c
memcached_return_t
memcached_prepend(memcached_st *ptr,
                  const char *key, size_t key_length,
                  const char *value, size_t value_length,
                  time_t expiration, uint32_t flags)
memcached_return_t
memcached_append(memcached_st *ptr,
                  const char *key, size_t key_length,
                  const char *value, size_t value_length,
                  time_t expiration, uint32_t flags)
```

대표적으로 prepend를 수행하는 예시는 다음과 같다.

```c
int arcus_kv_attach(memcached_st *memc)
{
  const char *key= "item:a_key";
  const char *value= "value";
  uint32_t exptime= 600;
  uint32_t flags= 0;
  memcached_return_t rc;

  rc= memcached_prepend(memc, key, strlen(key), value, strlen(value), exptime, flags);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_prepend: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  return 0;
}
```

## Key-Value Item 조회

Key-value item을 조회하는 API는 두 가지가 있다.

하나의 키에 대한 값을 조회하는 API는 아래와 같다.
반환된 결과가 NULL이 아닌 경우 반드시 free 해주어야 한다.

```c
char *
memcached_get(memcached_st *ptr,
              const char *key, size_t key_length,
              size_t *value_length, uint32_t *flags,
              memcached_return_t *error)
```

하나의 키에 대한 값을 조회하는 예시는 다음과 같다.

```c
int arcus_kv_get(memcached_st *memc)
{
  const char *key= "item:a_key";
  const char *value;
  size_t value_length;
  uint32_t flags;
  memcached_return_t rc;

  value= memcached_get(memc, key, strlen(key), &value_length, &flags, &rc);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_get: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  if (value != NULL) {
    fprintf(stdout, "memcached_get: %s=%s\n", key, value);
    free((void*)value);
  } else {
    fprintf(stdout, "memcached_get: %s=<empty value>.\n", key);
  }
  return 0;
}
```

여러 키들의 값들을 일괄 조회하는 API는 다음과 같다.
- `memcached_mget`은 주어진 key 배열에 대한 value들을 조회하는 요청을 보낸다.
  - keys 인자는 key pointer array이고, key_length 인자는 key length array이다.
- `memcached_fetch`는 mget 요청에 대한 결과를 하나씩 가져온다.
  - error 값이 MEMCACHED_END가 될 때까지 계속 실행하면 되며, 반환된 결과가 NULL이 아닌 경우 반드시 free 해주어야 한다.

```c
memcached_return_t
memcached_mget(memcached_st *ptr,
               const char * const *keys,  const size_t *key_length,
               const size_t number_of_keys)
char *
memcached_fetch(memcached_st *ptr,
                char *key, size_t *key_length,
                size_t *value_length, uint32_t *flags,
                memcached_return_t *error)
```

여러 키들의 값을 일괄 조회하는 예시는 다음과 같다.

```c
int arcus_kv_mget(memcached_st *memc)
{
  const char * const keys[]= { "item:a_key1", "item:a_key2", "item:a_key3" };
  size_t keys_len[3];
  size_t number_of_keys = 3;
  uint32_t flags;
  memcached_return_t rc;

  for (size_t i=0; i<number_of_keys; i++)
  {
    keys_len[i]= strlen(keys[i]);
  }

  rc= memcached_mget(memc, keys, keys_len, number_of_keys);
  if (rc == MEMCACHED_FAILURE) {
    fprintf(stderr, "Failed to memcached_get: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_SOME_ERRORS);
  while (true)
  {
    char key[MEMCACHED_MAX_KEY];
    size_t key_length;
    size_t value_length;

    char *value= memcached_fetch(memc, key, &key_length, &value_length, &flags, &rc);
    if (memcached_failed(rc)) {
      fprintf(stderr, "Failed to memcached_fetch: %d(%s)\n", rc, memcached_strerror(memc, rc));
      return -1;
    }

    if (rc == MEMCACHED_END) {
      break;
    }

    assert(rc == MEMCACHED_SUCCESS);
    if (value != NULL) {
      fprintf(stdout, "memcached_mget: %s=%s\n", key, value);
      free((void*)value);
    } else {
      fprintf(stdout, "memcached_mget: %s=<empty value>.\n", key);
    }
  }
  return 0;
}
```

## Key-Value Item 값의 CAS

아이템을 조회한 후, 해당 아이템이 변경되지 않았다면 새로운 값을 저장하는 Compare and Set(CAS) API는 다음과 같다
주의 사항으로, CAS 연산을 수행하기 위해서는 MEMCACHED_BEHAVIOR_SUPPORT_CAS 플래그가 설정된 memcached_st 구조체를 사용해야 한다.
- key에 해당하는 아이템의 cas 값을 확인한 후, 일치하면 새로운 값을 저장한다.
- 해당 item이 없거나 cas 값이 일치하지 않으면 오류가 발생한다.

```c
memcached_return_t
memcached_cas(memcached_st *ptr,
              const char *key, size_t key_length,
              const char *value, size_t value_length,
              time_t expiration, uint32_t flags,
              uint64_t cas)
```

Compare and Set 연산을 수행하는 예시는 다음과 같다.

```c
int arcus_kv_compare_and_set(memcached_st *memc)
{
  const char *key= "item:a_key";
  char *old_value= NULL;
  char *new_value= NULL;
  uint64_t cas_value;
  uint32_t exptime= 600;
  uint32_t flags= 0;
  int result= 0;
  memcached_return_t rc;

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, true);

  old_value= memcached_get(memc, key, strlen(key), NULL, NULL, &rc);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_gets: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }
  cas_value= memcached_result_cas(&memc->result);

  // make a new_value based on the old_value
  new_value= make_new_value_from_old_value(old_value);

  rc= memcached_cas(memc, key, strlen(key), new_value, strlen(new_value), exptime, flags, cas_value);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_cas: %d(%s)\n", rc, memcached_strerror(memc, rc));
    result= -1;
  }

  if (old_value) free(old_value);
  if (new_value) free(new_value);

  return result;
}
```

## Key-Value Item 값의 증감

특정 key에 해당하는 item이 가진 숫자형 값을 증감 연산하는 API는 다음과 같다.
- offset 만큼 증가/감소하며, 증감 후의 값을 value 인자로 반환한다.
- 해당 item이 없거나 해당 item이 숫자형 값을 가지지 않는다면 오류를 낸다.

```c
memcached_return_t
memcached_increment(memcached_st *ptr,
                    const char *key, size_t key_length,
                    uint32_t offset, uint64_t *value)
memcached_return_t
memcached_decrement(memcached_st *ptr,
                    const char *key, size_t key_length,
                    uint32_t offset, uint64_t *value)
```

특정 key에 해당하는 item이 있다면 증감 연산을 수행하고,
그 item이 없다면 초기값을 가진 item을 생성하는 API는 다음과 같다.
- 해당 item이 있다면 offset 만큼 증가/감소한다.
- 해당 item이 없다면 initial 값을 가진 item을 생성한다.
- 증감 or 생성 후의 item의 값을 value 인자로 반환한다.

```c
memcached_return_t
memcached_increment_with_initial(memcached_st *ptr,
                                 const char *key, size_t key_length, uint64_t offset,
                                 uint64_t initial, uint32_t flags, time_t expiration,
                                 uint64_t *value)
memcached_return_t
memcached_decrement_with_initial(memcached_st *ptr,
                                 const char *key, size_t key_length, uint64_t offset,
                                 uint64_t initial, uint32_t flags, time_t expiration,
                                 uint64_t *value)
```

특정 item의 숫자형 값을 증가시키는 예시는 다음과 같다.

```c
int arcus_kv_arithmetic(memcached_st *memc)
{
  const char *key= "item:a_key";
  uint64_t value= 0;
  // uint64_t initial = 0;
  uint32_t flags= 10;
  uint32_t exptime= 600;
  uint32_t offset= 10;
  memcached_return_t rc;

  rc= memcached_increment(memc, key, strlen(key), offset, &value);
  // rc= memcached_increment_with_initial(memc, key, strlen(key), offset, initial, flags, exptime, &value);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_increment: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  fprintf(stdout, "incremented value: %llu\n", value);
  return 0;
}
```

## Key-Value Item 삭제

주어진 key에 해당하는 item을 삭제하는 API는 다음과 같다.
expiration은 현재 지원하지 않으며, 0을 값으로 주어야 한다.

```c
memcached_return_t
memcached_delete(memcached_st *ptr,
                 const char *key, size_t key_length,
                 time_t expiration)
```

특정 key를 가진 item을 제거하는 예시는 다음과 같다.

```c
int arcus_kv_delete(memcached_st *memc)
{
  const char *key= "item:a_key";
  memcached_return_t rc;

  rc= memcached_delete(memc, key, strlen(key), 0);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_delete: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  return 0;
}
```
