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

- memcached_set: 주어진 key에 value를 저장한다.
- memcached_add: 주어진 key가 존재하지 않을 경우에만 value를 저장한다.
- memcached_replace: 주어진 key가 존재하는 경우에만 value를 저장한다.

Key-Value Item을 저장하는 예제는 아래와 같다.

```c
void arcus_kv_store(memcached_st *memc)
{
  uint32_t flags= 10;
  uint32_t exptime= 600;

  memcached_return_t rc;

  // key에 해당하는 아이템을 저장하며, 존재하지 않는 경우 새로 생성하여 저장한다.
  rc= memcached_set(memc, "item:a_key1", strlen("item:a_key1"), "value", strlen("value"), exptime, flags);
  assert(rc == MEMCACHED_SUCCESS);

  // 존재하지 않는 key를 가지는 Item을 저장한다.
  rc= memcached_add(memc, "item:a_key2", strlen("item:a_key2"), "value", strlen("value"), exptime, flags);
  assert(rc == MEMCACHED_SUCCESS);
  // 이미 존재하는 key에 대하여 add 수행 시 Item이 저장되지 않는다.
  rc= memcached_add(memc, "item:a_key1", strlen("item:a_key1"), "value", strlen("value"), exptime, flags);
  assert(rc == MEMCACHED_NOTSTORED);

  // 이미 존재하는 key를 가지는 Item을 교체한다.
  rc= memcached_replace(memc, "item:a_key1", strlen("item:a_key1"), "replaced_value", strlen("replaced_value"), exptime, flags);
  assert(rc == MEMCACHED_SUCCESS);
  // 존재하지 않는 key에 대하여 replace 수행 시 Item이 교체되지 않는다.
  rc= memcached_replace(memc, "item:a_key2", strlen("item:a_key2"), "replaced_value", strlen("replaced_value"), exptime, flags);
  assert(rc == MEMCACHED_NOTSTORED);
}
```

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

- memcached_prepend: 주어진 key의 value에 새로운 데이터를 prepend한다.
- memcached_append: 주어진 key의 value에 새로운 데이터를 append한다.

Key-value item 저장 연산에서 주요 파라미터는 아래와 같다.
- expiration: key가 현재 시간부터 expire 될 때까지의 시간(초 단위). 시간이 30일을 초과하는 경우 expire 될 unix time을 입력한다.
  - 0: key가 expire 되지 않도록 설정한다.
       하지만 ARCUS cache server의 메모리가 부족한 경우 LRU에 의해 언제든지 삭제될 수 있다.
  - -1: key를 sticky item으로 만든다. Sticky item은 expire 되지 않으며 LRU에 의해 삭제되지도 않는다.
- flags: value와는 별도로 저장할 수 있는 값으로서 Java client 등에서 내부적으로 사용하는 경우가 많으므로 사용하지 않기를 권한다.

저장된 Key-value item에 새로운 데이터를 추가하는 예제는 아래와 같다.

```c
void arcus_kv_attach(memcached_st *memc)
{
  uint32_t flags= 10;
  uint32_t exptime= 600;

  memcached_return_t rc;

  rc= memcached_set(memc, "item:a_key1", strlen("item:a_key1"), "value", strlen("value"), exptime, flags);
  assert(rc == MEMCACHED_SUCCESS);

  // 존재하는 key에 해당하는 Item의 앞 쪽에 데이터를 추가한다.
  rc= memcached_prepend(memc, "item:a_key1", strlen("item:a_key1"), "append_", strlen("append_"), exptime, flags);
  assert(rc == MEMCACHED_SUCCESS);

  // 존재하는 key에 해당하는 Item의 뒤 쪽에 데이터를 추가한다.
  rc= memcached_append(memc, "item:a_key1", strlen("item:a_key1"), "_prepend", strlen("_prepend"), exptime, flags);
  assert(rc == MEMCACHED_SUCCESS);
}
```

## Key-Value Item 조회

Key-value item을 조회하는 API는 두 가지가 있다.

```c
char *
memcached_get(memcached_st *ptr,
              const char *key, size_t key_length,
              size_t *value_length, uint32_t *flags,
              memcached_return_t *error)
```

주어진 key에 대한 value를 조회한다. 반환된 결과는 NULL이 아닌 경우 반드시 free 해주어야 한다.

Key-value item을 조회하는 예제는 아래와 같다.

```c
void arcus_kv_get(memcached_st *memc)
{
  uint32_t flags= 10;
  uint32_t exptime= 600;

  memcached_return_t rc;

  rc= memcached_set(memc, "item:a_key1", strlen("item:a_key1"), "value", strlen("value"), exptime, flags);
  assert(rc == MEMCACHED_SUCCESS);

  char *value;
  size_t value_length;
  bool do_free_value = true;

  // key에 해당하는 Item을 조회한다.
  value = memcached_get(memc, "item:a_key1", strlen("item:a_key1"), &value_length, &flags, &rc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(memcached_get_last_response_code(memc) == MEMCACHED_END);

  if (memcached_success(rc) && value_length == 0 && value == NULL) {
    do_free_value = false;
    value = "";
  }

  if (value != NULL) {
    assert(strcmp(value, "value") == 0);
    if (do_free_value) {
      free(value);
    }
  }
}
```

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

`memcached_mget`은 주어진 key 배열에 대한 value들을 조회하는 요청을 보낸다.
keys 인자는 key pointer array이고, key_length 인자는 key length array이다.

`memcached_fetch`는 mget 요청에 대한 결과를 하나씩 꺼내온다.
error 값이 MEMCACHED_END가 될 때까지 실행하면 되며, 그 이후에 fetch를 수행하면 MEMCACHED_NOTFOUND가 반환된다.

여러 개의 Key-value items를 조회하는 예제는 아래와 같다.

```c
void arcus_kv_mget(memcached_st *memc)
{
  uint32_t flags= 10;
  uint32_t exptime= 600;

  size_t number_of_keys = 50;

  char *setKey[50];
  char setValue[50][32];
  size_t setKey_len[50];
  size_t setValue_len[50];

  memcached_return_t rc;

  for (size_t i=0; i<number_of_keys; i++)
  {
    setKey[i] = (char *)malloc(32);
    setKey_len[i] = snprintf(setKey[i], 32, "item:a_key1%lu", (unsigned long)i);
    setValue_len[i]= snprintf(setValue[i], 32, "value%lu", (unsigned long)i);

    rc= memcached_set(memc, setKey[i], setKey_len[i], setValue[i], setValue_len[i], exptime, flags);
  }

  // 다수 개의 key에 해당하는 Item들을 조회한다.
  rc= memcached_mget(memc, (const char * const *) setKey, setKey_len, number_of_keys);
  assert(MEMCACHED_FAILURE != rc);

  // 조회 요청에 대한 결과를 하나씩 꺼낸다.
  for (size_t i=0; i<number_of_keys; i++) {
    char getKey[32] = {0, };
    char *getValue;
    size_t getKey_len;
    size_t getValue_len;
    bool do_free_value = true;
    flags = 0;

    getValue = memcached_fetch(mc, getKey, &getKey_len, &getValue_len, &flags, &rc);
    if (rc == MEMCACHED_END) {
      break;
    }
    if (memcached_success(rc) && getValue_len == 0 && getValue == NULL) {
      do_free_value = false;
      getValue = "";
    }
    if (getValue == NULL || memcached_failed(rc)) {
      break;
    }

    fprintf(stderr, "[debug] %lu : %s=%s (rc=%s)\n",
            (unsigned long)i, getKey, getValue, memcached_strerror(memc, rc));
    if (do_free_value) {
      free(getValue);
    }
  }
}
```

## Key-Value Item 값의 증감

Key-value item에서 숫자형 value 값에 대해서만 아래 증감 연산을 수행할 수 있다.

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

주어진 key의 value를 offset 만큼 증가/감소 시킨다.
주어진 key가 존재하지 않으면, 오류를 낸다.

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

주어진 key의 value를 offset 만큼 증가/감소 시킨다.
주어진 key가 존재하지 않으면, initial 값으로 저장한다.

Key-value item을 offset 만큼 증감시키는 예제는 아래와 같다.

```c
void arcus_kv_arithmetic(memcached_st *memc)
{
  uint64_t value;
  uint32_t flags= 10;
  uint32_t exptime= 600;
  uint32_t offset= 10;

  memcached_return_t rc;

  rc= memcached_set(memc, "item:a_key1", strlen("item:a_key1"), "10", strlen("10"), exptime, flags);
  assert(rc == MEMCACHED_SUCCESS);

  // 존재하는 key의 숫자형 value에 offset 만큼 증가시킨다.
  rc= memcached_increment(memc, "item:a_key1", strlen("item:a_key1"), offset, &value);
  assert(rc == MEMCACHED_SUCCESS);
  assert(uint64_t(20), value);
  // 존재하지 않는 key에 대해 오류를 내지 않고, initial 값을 저장한다.
  rc= memcached_increment_with_initial(memc, "item:a_key2", strlen("item:a_key2"), offset, 10, flags, exptime, &value);
  assert(rc == MEMCACHED_SUCCESS);
  assert(uint64_t(10), value);

  // 존재하는 key의 숫자형 value에 offset 만큼 감소시킨다.
  rc= memcached_decrement(memc, "item:a_key1", strlen("item:a_key1"), offset, &value);
  assert(rc == MEMCACHED_SUCCESS);
  assert(uint64_t(10), value);
  // 존재하지 않는 key에 대해 오류를 내지 않고, initial 값을 저장한다.
  rc= memcached_increment_with_initial(memc, "item:a_key2", strlen("item:a_key2"), offset, 10, flags, exptime, &value);
  assert(rc == MEMCACHED_SUCCESS);
  assert(uint64_t(10), value);
}
```

## Key-Value Item 삭제

```c
memcached_return_t
memcached_delete(memcached_st *ptr,
                 const char *key, size_t key_length,
                 time_t expiration)
```

주어진 key를 삭제한다.
expiration은 현재 지원하지 않으며, 0을 값으로 주어야 한다.

Key-value item을 삭제하는 예제는 아래와 같다.

```c
void arcus_kv_delete(memcached_st *memc)
{
  uint32_t flags= 10;
  uint32_t exptime= 600;

  memcached_return_t rc;

  rc= memcached_set(memc, "item:a_key1", strlen("item:a_key1"), "10", strlen("10"), exptime, flags);
  assert(rc == MEMCACHED_SUCCESS);

  // key에 해당하는 Item을 삭제한다.
  rc= memcached_delete(memc, "item:a_key1", strlen("item:a_key1"), 0);
  assert(rc == MEMCACHED_SUCCESS);
}
```