# Map Item

Map item은 하나의 key에 대해 hash 구조 기반으로 mkey & value 쌍을 data 집합으로 가진다.

**제약 조건**
- 저장 가능한 최대 element 개수: 디폴트 4,000개 (attribute 설정으로 최대 50,000개 확장 가능)
- 각 element에서 value 최대 크기 : 16KB
- mkey 최대 길이는 250 바이트 이고, 하나의 map에 중복된 mkey는 허용하지 않는다.

Map item에 대해 수행 가능한 기본 연산들은 아래와 같다.

- [Map Item 생성](06-map-API.md#map-item-생성) (Map Item 삭제는 key-value item 삭제 함수로 수행한다)
- [Map Element 삽입](06-map-API.md#map-element-삽입)
- [Map Element Upsert](06-map-API.md#map-element-upsert)
- [Map Element 변경](06-map-API.md#map-element-변경)
- [Map Element 삭제](06-map-API.md#map-element-삭제)
- [Map Element 조회](06-map-API.md#map-element-조회)

여러 map element들에 대해 한번에 일괄 수행하는 연산은 다음과 같다.

- [Map Element 일괄 삽입](06-map-API.md#map-element-일괄-삽입)


## Map Item 생성

새로운 empty map item을 생성한다.

```c
memcached_return_t
memcached_mop_create(memcached_st *ptr,
                     const char *key, size_t key_length,
                     memcached_coll_create_attrs_st *attributes)
```

- key, key_length: map item의 key
- attributes: map item의 속성 정보 [(링크)](08-attribute-API.md#attribute-생성)

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_CREATED: Map이 생성됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_EXISTS: 동일한 key를 가진 Map이 이미 존재함.

Map item을 생성하는 예시는 아래와 같다.

```c
int arcus_map_item_create(memcached_st *memc)
{
  const char *key= "map:a_key";
  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;
  memcached_return_t rc;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  rc= memcached_mop_create(memc, key, strlen(key), &attributes);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_mop_create: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  assert(memcached_get_last_response_code(memc) == MEMCACHED_CREATED);
  return 0;
}
```

## Map Element 삽입

Map에 하나의 element를 삽입하는 함수이다.

```c
memcached_return_t
memcached_mop_insert(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const char *mkey, size_t mkey_length,
                     const char *value, size_t value_length,
                     memcached_coll_create_attrs_st *attributes)
```

- key, key_length: map item의 key
- mkey, mkey_length: 삽입할 element의 mkey
- value, value_lenth: 삽입할 element의 value
- attributes: Map 없을 시에 attributes에 따라 empty map을 생성 후에 element 삽입한다.

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_STORED: 기존에 존재하던 Map에 element가 삽입됨.
  - MEMCACHED_CREATED_STORED: Map이 새롭게 생성되고 element가 삽입됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: Map이 존재하지 않음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 Map이 아님.
  - MEMCACHED_OVERFLOWED: Overflow 상태임. (overflowaction=error)
  - MEMCACHED_ELEMENT_EXISTS: 동일한 mkey를 가진 element가 이미 존재하고 있음

Map element를 삽입하는 예시는 아래와 같다.

```c
int arcus_map_element_insert(memcached_st *memc)
{
  const char *key= "map:a_key";
  const char *mkey= "mkey";
  const char *value= "value";
  memcached_return_t rc;

  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  rc= memcached_mop_insert(memc, key, strlen(key), mkey, strlen(mkey),
                           value, strlen(value), &attributes);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_mop_insert: %d(%s)",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_STORED ||
         last_response == MEMCACHED_CREATED_STORED);
  return 0;
}
```

## Map Element Upsert

Map에 하나의 element를 upsert한다.
Upsert 연산은 해당 element가 없으면 insert하고, 있으면 update하는 연산이다.

```c
 memcached_return_t
 memcached_mop_upsert(memcached_st *ptr,
                      const char *key, size_t key_length,
                      const char *mkey, size_t mkey_length,
                      const char *value, size_t value_length,
                      memcached_coll_create_attrs_st *attributes)
```

- key, key_length: map item의 key
- mkey, mkey_length: upsert할 element의 mkey
- value, value_lenth: upsert할 element의 value
- attributes: Map 없을 시에 attributes에 따라 empty map을 생성 후에 element 삽입한다.

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_STORED: 기존에 존재하던 Map에 element가 삽입됨.
  - MEMCACHED_CREATED_STORED: Map가 새롭게 생성되고 element가 삽입됨.
  - MEMCACHED_REPLACED: 기존에 존재하던 Map에서 동일한 mkey를 가진 element를 대체함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: Map이 존재하지 않음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 Map이 아님.
  - MEMCACHED_OVERFLOWED: Overflow 상태임. (overflowaction=error)

Map element를 upsert하는 예시는 아래와 같다.

```c
int arcus_map_element_upsert(memcached_st *memc)
{
  const char *key= "map:a_key";
  const char *mkey= "mkey";
  const char *value= "value";
  memcached_return_t rc;

  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  rc= memcached_mop_upsert(memc, key, strlen(key), mkey, strlen(mkey),
                           value, strlen(value), &attributes);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_mop_upsert: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_STORED ||
         last_response == MEMCACHED_CREATED_STORED ||
         last_response == MEMCACHED_REPLACED);
  return 0;
}
```

## Map Element 변경

Map에 하나의 element를 변경하는 함수이다. 주어진 mkey를 가진 element의 value를 변경한다.

```c
memcached_return_t
memcached_mop_update(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const char *mkey, size_t mkey_length,
                     const char *value, size_t value_length)
```

- key, key_length: map item의 key
- mkey, mkey_length: 변경할 element의 mkey
- value, value_lenth: element의 new value

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_UPDATED: 주어진 mkey를 가진 element를 업데이트함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: Map이 존재하지 않음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 Map이 아님.

Map element를 변경하는 예시는 아래와 같다.

```c
int arcus_map_element_update(memcached_st *memc)
{
  const char *key= "map:a_key";
  const char *mkey= "mkey";
  const char *value= "value";
  memcached_return_t rc;

  rc= memcached_mop_update(memc, key, strlen(key), mkey, strlen(mkey), value, strlen(value));
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_mop_update: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_UPDATED);
  return 0;
}
```

## Map Element 삭제

Map element를 삭제하는 함수는 두 가지가 있다.

첫째, map에서 하나의 mkey로 하나의 element만 삭제하는 함수이다.

```c
memcached_return_t
memcached_mop_delete(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const char *mkey, size_t mkey_length,
                     bool drop_if_empty)
```

둘째, map의 element 전체를 삭제하는 함수이다.

```c
memcached_return_t
memcached_mop_delete_all(memcached_st *ptr,
                         const char *key, size_t key_length,
                         bool drop_if_empty)
```
- key, key_length: map item의 key
- mkey, mkey_length: 삭제할 element의 mkey
- drop_if_empty: element 삭제로 empty map이 될 경우, 그 map도 삭제할 것인지를 지정

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_DELETED: 주어진 mkey에 해당하는 element 혹은 전체 element를 삭제함.
  - MEMCACHED_DELETED_DROPPED: 주어진 mkey에 해당하는 element 혹은 전체 element를 삭제하고, empty 상태가 된 Map도 삭제함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 Map이 없음.
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 mkey에 해당하는 element가 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 Map이 아님.

Map element를 삭제하는 예시는 아래와 같다.

```c
int arcus_map_element_delete(memcached_st *memc)
{
  const char *key= "map:a_key";
  const char *mkey= "mkey";
  bool drop_if_empty= false;
  memcached_return_t rc;

  rc= memcached_mop_delete(memc, key, strlen(key), mkey, strlen(mkey), drop_if_empty);
  // rc= memcached_mop_delete_all(memc, key, strlen(key), drop_if_empty);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_mop_delete: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_DELETED ||
         last_response == MEMCACHED_DELETED_DROPPED);
  return 0;
}
```

## Map Element 조회

Map element를 조회하는 함수는 세 가지가 있다.

첫째, 하나의 map mkey로 하나의 element만 조회하는 함수이다.

```c
memcached_return_t
memcached_mop_get(memcached_st *ptr,
                  const char *key, size_t key_length,
                  const char *mkey, size_t mkey_length,
                  bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

둘째, map의 전체 element를 조회하는 함수이다.

```c
memcached_return_t
memcached_mop_get_all(memcached_st *ptr,
                  const char *key, size_t key_length,
                  bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

셋째, mkey list에 해당하는 element들을 조회하는 함수이다.

```c
memcached_return_t
memcached_mop_get_by_list(memcached_st *ptr,
                  const char *key, size_t key_length,
                  const char * const *mkeys, const size_t *mkeys_length,
                  size_t number_of_mkeys, bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

- key, key_length: map item의 key
- mkey, mkey_length: 조회할 mkey
- mkeys, mkeys_length: 조회할 mkey list
- number_of_mkeys: mkey들의 수
- with_delete: 조회와 함께 삭제도 수행할 것인지를 지정
- drop_if_empty: element 삭제로 empty list가 될 경우, 그 list도 삭제할 것인지를 지정

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: Map에서 정상적으로 element를 조회하였음.
  - MEMCACHED_DELETED: Map에서 정상적으로 element를 조회하였으며, 동시에 이들을 삭제하였음.
  - MEMCACHED_DELETED_DROPPED: Map에서 정상적으로 element를 조회하였으며, 동시에 이들을 삭제하였음.

- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 Map이 없음.
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 mkey 또는 mkeys(mkey list)에 해당하는 element가 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 Map이 아님.
  - MEMCACHED_UNREADABLE: 주어진 key에 해당하는 Map이 unreadable 상태임.

조회 결과는 memcached_coll_result_t 구조체에 저장된다.
조회 결과에 접근하기 위한 API는 다음과 같다.

```c
memcached_coll_result_st *
memcached_coll_result_create(const memcached_st *ptr, memcached_coll_result_st *result)
void
memcached_coll_result_free(memcached_coll_result_st *result)

memcached_coll_type_t
memcached_coll_result_get_type(memcached_coll_result_st *result)
size_t
memcached_coll_result_get_count(memcached_coll_result_st *result)
uint32_t
memcached_coll_result_get_flags(memcached_coll_result_st *result)
const char *
memcached_coll_result_get_mkey(memcached_coll_result_st *result, size_t index)
size_t
memcached_coll_result_get_mkey_length(memcached_coll_result_st *result, size_t index)
const char *
memcached_coll_result_get_value(memcached_coll_result_st *result, size_t index)
size_t
memcached_coll_result_get_value_length(memcached_coll_result_st *result, size_t index)
```

- memcached_coll_result_create : 새로운 result 구조체를 생성한다.
  - result 인자가 NULL인 경우, result 구조체를 생성 및 초기화하여 반환한다.
  - result 인자가 NULL이 아닌 경우, 해당 result 구조체를 초기화하여 반환한다.
- memcached_coll_result_free : 동적으로 할당된 result 구조체 및 result 내부 값들의 메모리를 해제(free)한다.
- memcached_coll_result_get_type : 조회한 item의 type을 가져온다.
- memcached_coll_result_get_count : 조회한 item의 count를 가져온다.
- memcached_coll_result_get_flags : 조회한 item의 flags를 가져온다.
- memcached_coll_result_get_value : 조회한 item의 value를 가져온다.
- memcached_coll_result_get_value_length : 조회한 item의 value 길이 정보를 가져온다.

Map element를 조회하는 예제는 아래와 같다.

```c
int arcus_map_element_get(memcached_st *memc)
{
  const char *key= "map:a_key";
  const char *mkey= "mkey";
  // const char * const mkeys[]= { "mkey1", "mkey2", "mkey3" };
  // size_t number_of_mkeys = 3;
  // size_t mkeys_len[3];
  bool with_delete= false;
  bool drop_if_empty= false;
  memcached_coll_result_st result;
  memcached_return_t rc;

  memcached_coll_result_create(memc, &result);

  /* for (size_t i=0; i<number_of_mkeys; i++) {
    mkeys_len[i]= strlen(mkeys[i]);
  } */

  do {
    rc= memcached_mop_get(memc, key, strlen(key), mkey, strlen(mkey),
                          with_delete, drop_if_empty, &result);
    /* rc= memcached_mop_get_all(memc, key, strlen(key),
                                 with_delete, drop_if_empty, &result) */
    /* rc= memcached_mop_get_by_list(memc, key, strlen(key),
                                     mkeys, mkeys_len, number_of_mkeys,
                                     with_delete, drop_if_empty, &result); */
    if (memcached_failed(rc)) {
      fprintf(stderr, "Failed to memcached_mop_get: %d(%s)\n",
              rc, memcached_strerror(memc, rc));
      break;
    }

    memcached_return_t last_response= memcached_get_last_response_code(memc);
    assert(rc == MEMCACHED_SUCCESS);
    assert(last_response == MEMCACHED_END ||
           last_response == MEMCACHED_DELETED ||
           last_response == MEMCACHED_DELETED_DROPPED);

    for (size_t i=0; i<memcached_coll_result_get_count(&result); i++) {
      const char* value= memcached_coll_result_get_value(&result, i);
      fprintf(stdout, "memcached_mop_get: %s : %s => %s\n", key, mkey, value);
    }
  } while(0);

  memcached_coll_result_free(&result);
  return (rc == MEMCACHED_SUCCESS) ? 0 : -1;
}
```

## Map Element 일괄 삽입

Map에 여러 elements를 한번에 삽입하는 함수는 두 가지가 있다.

첫째, 하나의 key가 가리키는 map에 다수의 elements를 삽입하는 함수이다.

```c
memcached_return_t
memcached_mop_piped_insert(memcached_st *ptr,
                           const char *key, const size_t key_length,
                           const size_t number_of_piped_items,
                           const char * const *mkeys, const size_t *mkeys_length,
                           const char * const *values, const size_t *values_length,
                           memcached_coll_create_attrs_st *attributes,
                           memcached_return_t *results,
                           memcached_return_t *piped_rc)
```

- key, key_length: 하나의 key를 지정
- number_of_piped_items: 한번에 삽입할 element 개수
- mkeys, mkeys_length: 다수 element 각각의 mkey와 그 길이
- values, values_length: 다수 element 각각의 value와 그 길이
- attributes: 해당 map이 없을 시에, attributes에 따라 map을 생성 후에 삽입한다.

하나의 key가 가리키는 map에 다수의 elements를 삽입하는 예시는 다음과 같다.

```c
int arcus_map_element_piped_insert(memcached_st *memc)
{
  const char *key= "map:a_key";
  const char * const mkeys[]= { "mkey1", "mkey2", "mkey3" };
  const char * const values[]= { "value1", " value2", "value3" };
  size_t number_of_piped_items= 3;
  size_t mkeys_len[3];
  size_t values_len[3];
  memcached_return_t rc;
  memcached_return_t piped_rc;
  memcached_return_t results[3];

  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  for (size_t i=0; i<number_of_piped_items; i++)
  {
    mkeys_len[i]= strlen(mkeys[i]);
    values_len[i]= strlen(values[i]);
  }

  rc= memcached_mop_piped_insert(memc, key, strlen(key),
                                 number_of_piped_items,
                                 mkeys, mkeys_len, values, values_len,
                                 &attributes, results, &piped_rc);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_mop_piped_insert: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  if (piped_rc == MEMCACHED_ALL_FAILURE) {
    fprintf(stderr, "Failed to memcached_mop_piped_insert: All Failures\n");
    return -1;
  }

  if (piped_rc == MEMCACHED_SOME_SUCCESS) {
    for (size_t i=0; i<number_of_piped_items; i++) {
      if (results[i] != MEMCACHED_STORED &&
          results[i] != MEMCACHED_CREATED_STORED) {
        fprintf(stderr, "Failed to memcached_mop_piped_insert: "
                        "%s : %s => %s %d(%s)\n",
                key, mkeys[i], values[i],results[i],
                memcached_strerror(memc, results[i]));
      }
    }
  }

  assert(rc == MEMCACHED_SUCCESS);
  return 0;
}
```

둘째, 여러 key들이 가리키는 map들에 각각 하나의 element를 삽입하는 함수이다.

```c
memcached_return_t
memcached_mop_piped_insert_bulk(memcached_st *ptr,
                                const char * const *keys,
                                const size_t *key_length,
                                const size_t number_of_keys,
                                const char *mkey, size_t mkey_length,
                                const char *value, size_t value_length,
                                memcached_coll_create_attrs_st *attributes,
                                memcached_return_t *results,
                                memcached_return_t *piped_rc)
```

- keys, key_length: 다수 key들을 지정
- number_of_keys: key들의 수
- mkey, mkey_length: element의 mkey
- value, value_length: element의 value
- attributes: 해당 map이 없을 시에, attributes에 따라 map을 생성 후에 삽입한다.

Map element 일괄 삽입의 결과는 아래의 인자를 통해 받는다.

- results: 일괄 삽입 결과가 주어진 key 또는 (key, element) 순서대로 저장된다.
  - STORED: element만 삽입
  - CREATE_STORED: map item 생성 후에, element 삽입
- piped_rc: 일괄 저장의 전체 결과를 담고 있다
  - MEMCACHED_ALL_SUCCESS: 모든 element가 저장됨.
  - MEMCACHED_SOME_SUCCESS: 일부 element가 저장됨.
  - MEMCACHED_ALL_FAILURE: 전체 element가 저장되지 않음.

여러 key들이 가리키는 map들에 각각 하나의 element를 삽입하는 예시는 아래와 같다.

```c
int arcus_map_element_piped_insert_bulk(memcached_st *memc)
{
  const char * const keys[]= { "map:a_key1", "map:a_key2", "map:a_key3" };
  const char *mkey= "mkey";
  const char *value= "value";
  size_t keys_len[3];
  size_t number_of_keys= 3;
  memcached_return_t rc;
  memcached_return_t piped_rc;
  memcached_return_t results[3];

  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  for (size_t i=0; i<number_of_keys; i++)
  {
    keys_len[i]= strlen(keys[i]);
  }

  rc= memcached_mop_piped_insert_bulk(memc, keys, keys_len,
                                      number_of_keys,
                                      mkey, strlen(mkey),
                                      value, strlen(value),
                                      &attributes, results, &piped_rc);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_mop_piped_insert_bulk: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  if (piped_rc == MEMCACHED_ALL_FAILURE) {
    fprintf(stderr, "Failed to memcached_mop_piped_insert_bulk: All Failures\n");
    return -1;
  }

  if (piped_rc == MEMCACHED_SOME_SUCCESS) {
    for (size_t i=0; i<number_of_keys; i++) {
      if (results[i] != MEMCACHED_STORED &&
          results[i] != MEMCACHED_CREATED_STORED) {
        fprintf(stderr, "Failed to memcached_mop_piped_insert_bulk: "
                        "%s : %s => %s %d(%s)\n",
                keys[i], mkey, value, results[i],
                memcached_strerror(memc, results[i]));
      }
    }
  }

  assert(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_SOME_ERRORS);
  return 0;
}
```
