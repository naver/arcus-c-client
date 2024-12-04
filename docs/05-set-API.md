# Set Item

Set item은 하나의 key에 대해 unique value의 집합을 저장한다. 주로 membership checking에 유용하게 사용할 수 있다.

**제약 조건**
- 저장 가능한 최대 element 개수 : 디폴트 4,000개 (attribute 설정으로 최대 50,000개 확장 가능)
- 각 element에서 value 최대 크기 : 16KB
- Element 값의 중복을 허용하지 않는다.

Set item에 수행 가능한 기본 연산들은 다음과 같다.

- [Set Item 생성](05-set-API.md#set-item-%EC%83%9D%EC%84%B1) (Set item 삭제는 key-value item 삭제 함수로 수행한다)
- [Set Element 삽입](05-set-API.md#set-element-%EC%82%BD%EC%9E%85)
- [Set Element 삭제](05-set-API.md#set-element-%EC%82%AD%EC%A0%9C)
- [Set Element 존재 여부 확인](05-set-API.md#set-element-존재-여부-확인)
- [Set Element 조회](05-set-API.md#set-element-%EC%A1%B0%ED%9A%8C)

여러 set element들에 대해 한번에 일괄 수행하는 연산은 다음과 같다.

- [Set Element 일괄 삽입](05-set-API.md#set-element-%EC%9D%BC%EA%B4%84-%EC%82%BD%EC%9E%85)
- [Set Element 일괄 존재 여부 확인](05-set-API.md#set-element-일괄-존재-여부-확인)


## Set Item 생성

새로운 empty set item을 생성한다.

```c
memcached_return_t
memcached_sop_create(memcached_st *ptr,
                     const char *key, size_t key_length,
                     memcached_coll_create_attrs_st *attributes)
```

- key, key_length: set item의 key
- attributes: set item의 속성 정보 [(링크)](08-attribute-API.md#attribute-생성)

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_CREATED: set이 생성됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_EXISTS: 동일한 key를 가진 set이 이미 존재함.

Set item을 생성하는 예시는 아래와 같다.

```c
int arcus_set_item_create(memcached_st *memc)
{
  const char *key= "set:a_key";
  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;
  memcached_return_t rc;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  rc= memcached_sop_create(memc, key, strlen(key), &attributes);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_sop_create: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  assert(memcached_get_last_response_code(memc) == MEMCACHED_CREATED);
  return 0;
}
```

## Set Element 삽입

Set에 하나의 element를 삽입하는 함수이다.

```c
memcached_return_t
memcached_sop_insert(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const char *value, size_t value_length,
                     memcached_coll_create_attrs_st *attributes)
```

- key, key_length: set item의 key
- value, value_lenth: 삽입할 element의 value
- attributes: Set 없을 시에 attributes에 따라 empty set을 생성 후에 element 삽입한다.

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_STORED: element만 삽입
  - MEMCACHED_CREATED_STORED: Set 생성 후에 element 삽입
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: Set이 존재하지 않음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key의 자료구조가 set이 아님.
  - MEMCACHED_OVERFLOWED: Overflow 상태임. (overflowaction=error, maxcount=count)
  - MEMCACHED_ELEMENT_EXISTS: 동일한 value를 가진 element가 이미 존재하고 있음

Set에 하나의 element를 삽입하는 예시는 아래와 같다.

```c
int arcus_set_element_insert(memcached_st *memc)
{
  const char *key= "set:a_key";
  const char *value= "value";
  memcached_return_t rc;

  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  rc= memcached_sop_insert(memc, key, strlen(key), value, strlen(value), &attributes);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_sop_insert: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_STORED || last_response == MEMCACHED_CREATED_STORED);
  return 0;
}
```

## Set Element 삭제

Set에서 주어진 value를 가진 element를 삭제하는 함수이다.

```c
memcached_return_t
memcached_sop_delete(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const char *value, size_t value_length,
                     bool drop_if_empty)
```

- key, key_length: set item의 key
- value, value_length: 삭제할 element의 value
- drop_if_empty: element 삭제로 empty list가 될 경우, 그 list도 삭제할 것인지를 지정

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_DELETED: 주어진 value를 가지는 element를 삭제함.
  - MEMCACHED_DELETED_DROPPED: 주어진 value를 가지는 element를 삭제하고, empty 상태가 된 Set도 삭제함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 Set이 없음.
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 value를 가지는 element가 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 Set이 아님.

Set에서 하나의 element를 삭제하는 예시는 아래와 같다.

```c
int arcus_set_element_delete(memcached_st *memc)
{
  const char *key= "set:a_key";
  const char *value= "value";
  bool drop_if_empty= false;
  memcached_return_t rc;

  rc= memcached_sop_delete(memc, key, strlen(key), value, strlen(value), drop_if_empty);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_sop_delete: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_DELETED || last_response == MEMCACHED_DELETED_DROPPED);
  return 0;
}
```

## Set Element 존재 여부 확인

Set에서 주어진 value를 가진 element의 존재 여부를 확인한다.

```c
memcached_return_t
memcached_sop_exist(memcached_st *ptr,
                    const char *key, size_t key_length,
                    const char *value, size_t value_length)
```

- key, key_length: set item의 key
- value, value_length: 존재 여부를 확인할 element의 value

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_EXIST: 주어진 value를 가지는 element가 존재함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOT_EXIST: 주어진 value를 가지는 element가 존재하지 않음.
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 Set이 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 Set이 아님.
  - MEMCACHED_UNREADABLE: 주어진 key에 해당하는 Set이 unreadable 상태임.

Set element의 존재 여부를 확인하는 예시는 아래와 같다.

```c
int arcus_set_element_exist(memcached_st *memc)
{
  const char *key= "set:a_key";
  const char *value= "value";
  memcached_return_t rc;

  rc= memcached_sop_exist(memc, key, strlen(key), value, strlen(value));
  if (memcached_failed(rc)) {
    if (rc == MEMCACHED_NOT_EXIST) {
      return 0; /* element doesn't exist */
    }
    fprintf(stderr, "Failed to memcached_sop_exist: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_EXIST);
  return 1; /* element exists */
}
```

## Set Element 조회

Set element를 조회하는 함수이다. 이 함수는 임의의 count 개 elements를 조회한다.

```c
memcached_return_t
memcached_sop_get(memcached_st *ptr,
                  const char *key, size_t key_length,
                  size_t count, bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

- key, key_length: set item의 key
- count: 조회할 element 개수를 지정. 0이면 전체 elements를 의미
- with_delete: 조회와 함께 삭제도 수행할 것인지를 지정
- drop_if_empty: element 삭제로 empty set이 될 경우, 그 set도 삭제할 것인지를 지정

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: Set에서 정상적으로 element를 조회하였음.
  - MEMCACHED_DELETED: Set에서 정상적으로 element를 조회하였으며, 동시에 이들을 삭제하였음.
  - MEMCACHED_DELETED_DROPPED: Set에서 정상적으로 element를 조회하였으며, 동시에 이들을 삭제하였음.
    이 결과 empty 상태가 된 Set을 삭제함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 Set이 없음.
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 value를 가지는 element가 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 Set이 아님.
  - MEMCACHED_UNREADABLE: 주어진 key에 해당하는 Set이 unreadable 상태임.

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

Set element를 조회하는 예시는 아래와 같다.

``` c
int arcus_set_element_get(memcached_st *memc)
{
  const char *key= "set:a_key";
  size_t count= 0;
  bool with_delete = false;
  bool drop_if_empty = false;
  memcached_coll_result_st result;
  memcached_return_t rc;

  memcached_coll_result_create(memc, &result);

  do {
    rc= memcached_sop_get(memc, key, strlen(key), count, with_delete, drop_if_empty, &result);
    if (memcached_failed(rc)) {
      fprintf(stderr, "Failed to memcached_sop_get: %d(%s)\n", rc, memcached_strerror(memc, rc));
      break;
    }

    memcached_return_t last_response= memcached_get_last_response_code(memc);
    assert(rc == MEMCACHED_SUCCESS);
    assert(last_response == MEMCACHED_END ||
           last_response == MEMCACHED_DELETED || last_response == MEMCACHED_DELETED_DROPPED);

    for (size_t i=0; i<memcached_coll_result_get_count(&result); i++) {
      const char* value= memcached_coll_result_get_value(&result, i);
      fprintf(stdout, "memcached_sop_get: %s : %s\n", key, value);
    }
  } while(0);

  memcached_coll_result_free(&result);
  return (rc == MEMCACHED_SUCCESS) ? 0 : -1;
}
```

## Set Element 일괄 삽입

Set에 여러 elements를 한번에 삽입하는 함수는 두 가지가 있다.

첫째, 하나의 key가 가리키는 set에 다수의 elements를 삽입하는 함수이다.

```c
memcached_return_t
memcached_sop_piped_insert(memcached_st *ptr,
                           const char *key, const size_t key_length,
                           const size_t number_of_piped_items,
                           const char * const *values, const size_t *values_length,
                           memcached_coll_create_attrs_st *attributes,
                           memcached_return_t *results,
                           memcached_return_t *piped_rc)
```

- key, key_length: 하나의 key를 지정
- number_of_piped_items: 한번에 삽입할 element 개수
- values, values_length: 다수 element 각각의 value와 그 길이
- attributes: 해당 set이 없을 시에, attributes에 따라 set을 생성 후에 삽입한다.

하나의 key가 가리키는 set에 다수의 elements를 삽입하는 예시는 다음과 같다.

```c
int arcus_set_element_piped_insert(memcached_st *memc)
{
  const char *key= "set:a_key";
  const char * const values[]= { "value1", "value2", "value3" };
  size_t number_of_values = 3;
  size_t values_len[3];
  memcached_return_t rc;
  memcached_return_t piped_rc;
  memcached_return_t results[3];

  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  for (size_t i=0; i<number_of_values; i++)
  {
    values_len[i]= strlen(values[i]);
  }

  rc= memcached_sop_piped_insert(memc, key, strlen(key),
                                 number_of_values,
                                 values, values_len,
                                 &attributes, results, &piped_rc);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_sop_piped_insert: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  if (piped_rc == MEMCACHED_ALL_FAILURE) {
    fprintf(stderr, "Failed to memcached_sop_piped_insert: All Failures\n");
    return -1;
  }

  if (piped_rc == MEMCACHED_SOME_SUCCESS) {
    for (size_t i=0; i<number_of_values; i++) {
      if (results[i] != MEMCACHED_STORED && results[i] != MEMCACHED_CREATED_STORED) {
        fprintf(stderr, "Failed to memcached_sop_piped_insert: %s : %s %d(%s)\n",
                key, values[i], results[i], memcached_strerror(memc, results[i]));
      }
    }
  }

  assert(rc == MEMCACHED_SUCCESS);
  return 0;
}
```

둘째, 여러 key들이 가리키는 set들에 각각 하나의 element를 삽입하는 함수이다.

```c
memcached_return_t
memcached_sop_piped_insert_bulk(memcached_st *ptr,
                                const char * const *keys,
                                const size_t *key_length,
                                const size_t number_of_keys,
                                const char *value, size_t value_length,
                                memcached_coll_create_attrs_st *attributes,
                                memcached_return_t *results,
                                memcached_return_t *piped_rc)
```

- keys, key_length: 다수 key들을 지정
- number_of_keys: key들의 수
- values, values_length: 각 set에 삽입할 element의 value와 그 길이
- attributes: 해당 set이 없을 시에, attributes에 따라 set을 생성 후에 삽입한다.

Set element 일괄 삽입의 결과는 아래의 인자를 통해 받는다.

- results: 일괄 삽입 결과가 주어진 key 또는 (key, element) 순서대로 저장된다.
  - STORED: element만 삽입
  - CREATE_STORED: set item 생성 후에, element 삽입
- piped_rc: 일괄 저장의 전체 결과를 담고 있다
  - MEMCACHED_ALL_SUCCESS: 모든 element가 저장됨.
  - MEMCACHED_SOME_SUCCESS: 일부 element가 저장됨.
  - MEMCACHED_ALL_FAILURE: 전체 element가 저장되지 않음.

여러 key들이 가리키는 set들에 각각 하나의 element를 삽입하는 예시는 아래와 같다.

```c
int arcus_set_element_piped_insert_bulk(memcached_st *memc)
{
  const char * const keys[]= { "set:a_key1", "set:a_key2", "set:a_key3" };
  const char *value= "value";
  size_t keys_len[3];
  size_t number_of_keys = 3;
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

  rc= memcached_sop_piped_insert_bulk(memc, keys, keys_len,
                                      number_of_keys,
                                      value, strlen(value),
                                      &attributes, results, &piped_rc);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_sop_piped_insert: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  if (piped_rc == MEMCACHED_ALL_FAILURE) {
    fprintf(stderr, "Failed to memcached_sop_piped_insert_bulk: All Failures\n");
    return -1;
  }

  if (piped_rc == MEMCACHED_SOME_SUCCESS) {
    for (size_t i=0; i<number_of_keys; i++) {
      if (results[i] != MEMCACHED_STORED && results[i] != MEMCACHED_CREATED_STORED) {
        fprintf(stderr, "Failed to memcached_sop_piped_insert_bulk: %s : %s %d(%s)\n",
                keys[i], value, results[i], memcached_strerror(memc, results[i]));
      }
    }
  }

  assert(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_SOME_ERRORS);
  return 0;
}
```

## Set Element 일괄 존재 여부 확인

Set에서 여러 elements의 존재 여부를 한번에 확인하는 함수이다.

```c
memcached_return_t
memcached_sop_piped_exist(memcached_st *ptr,
                          const char *key, size_t key_length,
                          const size_t number_of_piped_items,
                          const char * const *values, const size_t *values_length,
                          memcached_return_t *results,
                          memcached_return_t *piped_rc)
```

- key, key_length: 하나의 key를 지정
- number_of_piped_items: 한번에 확인할 element 개수
- values, values_length: 각 element의 value와 길이

Set element 일괄 존재 여부 확인의 결과는 아래의 인자를 통해 받는다.

- results: 각 value에 대한 element 존재 여부 결과를 순서대로 저장한다.
  - MEMCACHED_EXIST
  - MEMCACHED_NOT_EXIST
- piped_rc: 일괄 존재 여부 확인 결과를 담고 있다.
  - MEMCACHED_ALL_EXIST: 모든 element가 존재함.
  - MEMCACHED_SOME_EXIST: 일부 element가 존재함.
  - MEMCACHED_ALL_NOT_EXIST: 모든 element가 존재하지 않음.
  - MEMCACHED_SOME_SUCCESS: 일부 element 존재 여부 확인에 실패함.
  - MEMCACHED_ALL_FAILURE: 전체 element 존재 여부 확인에 실패함.

Set elements의 존재 여부를 한번에 확인하는 예시는 다음과 같다.

```c
int arcus_set_element_piped_exist(memcached_st *memc)
{
  const char *key= "set:a_key";
  const char * const values[]= { "value1", "value2", "value3" };
  size_t number_of_values = 3;
  size_t values_len[3];
  memcached_return_t rc;
  memcached_return_t piped_rc;
  memcached_return_t results[3];

  for (size_t i=0; i<number_of_values; i++)
  {
    values_len[i]= strlen(values[i]);
  }

  rc= memcached_sop_piped_exist(memc, key, strlen(key),
                                number_of_values,
                                values, values_len,
                                results, &piped_rc);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_sop_piped_exist: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  if (piped_rc == MEMCACHED_ALL_FAILURE) {
    fprintf(stderr, "Failed to memcached_sop_piped_exist: All Failures\n");
    return -1;
  }

  if (piped_rc == MEMCACHED_SOME_SUCCESS) {
    for (size_t i=0; i<number_of_values; i++) {
      if (results[i] != MEMCACHED_EXIST && results[i] != MEMCACHED_NOT_EXIST) {
        fprintf(stderr, "Failed to memcached_sop_piped_exist: %s : %s %d(%s)\n",
                key, values[i], results[i], memcached_strerror(memc, results[i]));
      }
    }
  }

  assert(rc == MEMCACHED_SUCCESS);
  return 0;
}
```
