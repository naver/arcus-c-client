# List Item

List item은 하나의 key에 대해 여러 value들을 double linked list 구조로 유지한다.

**제약 조건**
- 저장 가능한 최대 element 개수: 디폴트 4,000개 (attribute 설정으로 최대 50,000개 확장 가능)
- 각 element에서 value 최대 크기 : 16KB
- List의 앞, 뒤에서 element를 삽입/삭제하기를 권한다. 임의의 index 위치에서 element 삽입/삭제가 가능하지만,
  임의의 index 위치를 신속히 찾아가기 위한 자료구조가 현재 없는 상태라서 비용이 많이 든다.

List item에 대해 수행 가능한 기본 연산들은 아래와 같다.

- [List Item 생성](04-list-API.md#list-item-%EC%83%9D%EC%84%B1) (List Item 삭제는 key-value item 삭제 함수로 수행한다)
- [List Element 삽입](04-list-API.md#list-element-%EC%82%BD%EC%9E%85)
- [List Element 삭제](04-list-API.md#list-element-%EC%82%AD%EC%A0%9C)
- [List Element 조회](04-list-API.md#list-element-%EC%A1%B0%ED%9A%8C)

여러 list element들에 대해 한번에 일괄 수행하는 연산은 다음과 같다.

- [List Element 일괄 삽입](04-list-API.md#list-element-%EC%9D%BC%EA%B4%84-%EC%82%BD%EC%9E%85)


## List Item 생성

새로운 empty list item을 생성한다.

```c
memcached_return_t
memcached_lop_create(memcached_st *ptr,
                     const char *key, size_t key_length,
                     memcached_coll_create_attrs_st *attributes)
```

- key, key_length: list item의 key
- attributes: list item의 속성 정보 [(링크)](08-attribute-API.md#attribute-생성)

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_CREATED: List가 생성됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_EXISTS: 동일한 key를 가진 List가 이미 존재함.

List item을 생성하는 예시는 아래와 같다.

```c
int arcus_list_item_create(memcached_st *memc)
{
  const char *key= "list:a_key";
  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;
  memcached_return_t rc;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  rc= memcached_lop_create(memc, key, strlen(key), &attributes);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_lop_create: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  assert(memcached_get_last_response_code(memc) == MEMCACHED_CREATED);
  return 0;
}
```

## List Element 삽입


List에 하나의 element를 삽입하는 함수이다.

```c
memcached_return_t
memcached_lop_insert(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const int32_t index,
                     const char *value, size_t value_length,
                     memcached_coll_create_attrs_st *attributes)
```

- key, key_length: list item의 key
- index: list index (0-based index)
  - 0, 1, 2, ... : list의 앞에서 시작하여 각 element 위치를 나타냄
  - -1, -2, -3, ... : list의 뒤에서 시작하여 각 element 위치를 나타냄
- value, value_length: 삽입할 element의 value
- attributes: List 없을 시에 attributes에 따라 empty list를 생성 후에 element 삽입한다.

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_STORED: 기존에 존재하던 List에 element가 삽입됨.
  - MEMCACHED_CREATED_STORED: List가 새롭게 생성되고 element가 삽입됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: List가 존재하지 않음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 List가 아님.
  - MEMCACHED_OVERFLOWED: Overflow 상태임. (overflowaction=error, maxcount=count)
  - MEMCACHED_OUT_OF_RANGE: 삽입 위치가 List의 element index 범위를 넘어섬.

List element를 삽입하는 예시는 아래와 같다.

```c
int arcus_list_element_insert(memcached_st *memc)
{
  const char *key= "list:a_key";
  const char *value= "value";
  const int32_t index= -1;
  memcached_return_t rc;

  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  rc= memcached_lop_insert(memc, key, strlen(key), index, value, strlen(value), &attributes);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_lop_insert: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_STORED || last_response == MEMCACHED_CREATED_STORED);
  return 0;
}
```

## List Element 삭제

List element를 삭제하는 함수는 두 가지가 있다.

첫째, 하나의 list index로 하나의 element만 삭제하는 함수이다.

```c
memcached_return_t
memcached_lop_delete(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const int32_t index, bool drop_if_empty)
```

- index: single list index (0-based index)
  - 0, 1, 2, ... : list의 앞에서 시작하여 각 element 위치를 나타냄
  - -1, -2, -3, ... : list의 뒤에서 시작하여 각 element 위치를 나타냄
- drop_if_empty: element 삭제로 empty list가 될 경우, 그 list도 삭제할 것인지를 지정

둘째, list index range로 다수의 elements를 삭제하는 함수이다.

```c
memcached_return_t
memcached_lop_delete_by_range(memcached_st *ptr,
                              const char *key, size_t key_length,
                              const int32_t from, const int32_t to,
                              bool drop_if_empty)
```

- key, key_length: list item의 key
- from, to: list index range (0-based index)
  - 0, 1, 2, ... : list의 앞에서 시작하여 각 element 위치를 나타냄
  - -1, -2, -3, ... : list의 뒤에서 시작하여 각 element 위치를 나타냄
- drop_if_empty: element 삭제로 empty list가 될 경우, 그 list도 삭제할 것인지를 지정

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_DELETED: 주어진 index에 해당하는 element를 삭제함.
  - MEMCACHED_DELETED_DROPPED: 주어진 index에 해당하는 element를 삭제하고, empty 상태가 된 List도 삭제함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 List가 없음.
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 index나 index 범위에 해당하는 element가 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 List가 아님.


List element를 삭제하는 예시는 아래와 같다.

```c
int arcus_list_element_delete(memcached_st *memc)
{
  const char *key="list:a_key";
  const int32_t index= 0;
  // const int32_t from= 0, to= -1;
  bool drop_if_empty = false;
  memcached_return_t rc;

  rc= memcached_lop_delete(memc, key, strlen(key), index, drop_if_empty);
  // rc= memcached_lop_delete_by_range(memc, key, strlen(key), from, to, drop_if_empty);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_lop_delete: %d(%s)\n", rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_DELETED || last_response == MEMCACHED_DELETED_DROPPED);
  return 0;
}
```

## List Element 조회

List element를 조회하는 함수는 두 가지가 있다.

첫째, 하나의 list index로 하나의 element만 조회하는 함수이다.

```c
memcached_return_t
memcached_lop_get(memcached_st *ptr,
                  const char *key, size_t key_length,
                  const int32_t index,
                  bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

- key, key_length: list item의 key
- index: single list index (0-based index)
  - 0, 1, 2, ... : list의 앞에서 시작하여 각 element 위치를 나타냄
  - -1, -2, -3, ... : list의 뒤에서 시작하여 각 element 위치를 나타냄
- with_delete: 조회와 함께 삭제도 수행할 것인지를 지정
- drop_if_empty: element 삭제로 empty list가 될 경우, 그 list도 삭제할 것인지를 지정


둘째, list index range로 다수의 elements를 조회하는 함수이다.

```c
memcached_return_t
memcached_lop_get_by_range(memcached_st *ptr,
                           const char *key, size_t key_length,
                           const int32_t from, const int32_t to,
                           bool with_delete, bool drop_if_empty,
                           memcached_coll_result_st *result)
```

- from, to: list index range (0-based index)
  - 0, 1, 2, ... : list의 앞에서 시작하여 각 element 위치를 나타냄
  - -1, -2, -3, ... : list의 뒤에서 시작하여 각 element 위치를 나타냄
- with_delete: 조회와 함께 삭제도 수행할 것인지를 지정
- drop_if_empty: element 삭제로 empty list가 될 경우, 그 list도 삭제할 것인지를 지정

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: List에서 정상적으로 element를 조회하였음.
  - MEMCACHED_DELETED: List에서 정상적으로 element를 조회하였으며, 동시에 이들을 삭제하였음.
  - MEMCACHED_DELETED_DROPPED: List에서 정상적으로 element를 조회하였으며, 동시에 이들을 삭제하였음.
    이 결과 empty 상태가 된 List를 삭제함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 List가 없음.
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 index 또는 index 범위에 해당하는 element가 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 List가 아님.
  - MEMCACHED_UNREADABLE: 주어진 key에 해당하는 List가 unreadable 상태임.

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

List element를 조회하는 예시는 아래와 같다.

```c
int arcus_list_element_get(memcached_st *memc)
{
  const char *key= "list:a_key";
  const int32_t index= 0;
  // const int32_t from= 0, to= -1;
  bool with_delete = false;
  bool drop_if_empty = false;
  memcached_coll_result_st result;
  memcached_return_t rc;

  memcached_coll_result_create(memc, &result);

  do {
    rc= memcached_lop_get(memc, key, strlen(key), index, with_delete, drop_if_empty, &result);
    // rc= memcached_lop_get_by_range(memc, key, strlen(key), from, to, with_delete, drop_if_empty, &result);
    if (memcached_failed(rc)) {
      fprintf(stderr, "Failed to memcached_lop_get: %d(%s)\n", rc, memcached_strerror(memc, rc));
      break;
    }

    memcached_return_t last_response= memcached_get_last_response_code(memc);
    assert(rc == MEMCACHED_SUCCESS);
    assert(last_response == MEMCACHED_END ||
           last_response == MEMCACHED_DELETED || last_response == MEMCACHED_DELETED_DROPPED);

    const char* value= memcached_coll_result_get_value(&result, 0);
    fprintf(stdout, "memcached_lop_get: %s[%d] : %s\n", key, index, value);
    /* for (size_t i=0; i<memcached_coll_result_get_count(&result); i++) {
      const char* value= memcached_coll_result_get_value(&result, i);
      fprintf(stdout, "memcached_lop_get: %s[%d] : %s\n", key, i, value);
    } */
  } while(0);

  memcached_coll_result_free(&result);
  return (rc == MEMCACHED_SUCCESS) ? 0 : -1;
}
```

## List Element 일괄 삽입

List에 여러 elements를 한번에 삽입하는 함수는 두 가지가 있다.

첫째, 하나의 key가 가리키는 list에 다수의 elements를 삽입하는 함수이다.

```c
memcached_return_t
memcached_lop_piped_insert(memcached_st *ptr,
                           const char *key, const size_t key_length,
                           const size_t number_of_piped_items,
                           const int32_t *indexes,
                           const char * const *values, const size_t *values_length,
                           memcached_coll_create_attrs_st *attributes,
                           memcached_return_t *results,
                           memcached_return_t *piped_rc)
```

- key, key_length: 하나의 key를 지정
- number_of_piped_items: 한번에 삽입할 element 개수
- indexes: list index array (0-based index)
- values, values_length: 다수 element 각각의 value와 그 길이
- attributes: 해당 list가 없을 시에, attributes에 따라 list를 생성 후에 삽입한다.

하나의 key가 가리키는 list에 다수의 elements를 삽입하는 예시는 다음과 같다.

```c
int arcus_list_element_piped_insert(memcached_st *memc)
{
  const char *key= "list:a_key";
  const char * const values[]= { "value1", "value2", "value3" };
  size_t number_of_values = 3;
  size_t values_len[3];
  int32_t indexes[3];
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
    indexes[i]= i;
    values_len[i]= strlen(values[i]);
  }

  rc= memcached_lop_piped_insert(memc, key, strlen(key),
                                 number_of_values, indexes,
                                 values, values_len,
                                 &attributes, results, &piped_rc);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_lop_piped_insert: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  if (piped_rc == MEMCACHED_ALL_FAILURE) {
    fprintf(stderr, "Failed to memcached_lop_piped_insert: All Failures\n");
    return -1;
  }

  if (piped_rc == MEMCACHED_SOME_SUCCESS) {
    for (size_t i=0; i<number_of_values; i++) {
      if (results[i] != MEMCACHED_STORED && results[i] != MEMCACHED_CREATED_STORED) {
        fprintf(stderr, "Failed to memcached_lop_piped_insert: %s[%d] %d(%s)\n",
                key, indexes[i], results[i], memcached_strerror(memc, results[i]));
      }
    }
  }

  assert(rc == MEMCACHED_SUCCESS);
  return 0;
}
```

둘째, 여러 key들이 가리키는 list들에 각각 하나의 element를 삽입하는 함수이다.

```c
memcached_return_t
memcached_lop_piped_insert_bulk(memcached_st *ptr,
                                const char * const *keys, const size_t *key_length,
                                const size_t number_of_keys,
                                const int32_t index,
                                const char *value, size_t value_length,
                                memcached_coll_create_attrs_st *attributes,
                                memcached_return_t *results,
                                memcached_return_t *piped_rc)
```

- keys, key_length: 다수 key들을 지정
- number_of_keys: key들의 수
- index: list index (0-based index)
- value, value_length: 각 list에 삽입할 element의 value와 그 길이
- attributes: 해당 list가 없을 시에, attributes에 따라 list를 생성 후에 삽입한다.

List element 일괄 삽입의 결과는 아래의 인자를 통해 받는다.

- results: 일괄 삽입 결과가 주어진 key 또는 (key, element) 순서대로 저장된다.
  - STORED: element만 삽입
  - CREATE_STORED: list item 생성 후에, element 삽입
- piped_rc: 일괄 저장의 전체 결과를 담고 있다
  - MEMCACHED_ALL_SUCCESS: 모든 element가 저장됨.
  - MEMCACHED_SOME_SUCCESS: 일부 element가 저장됨.
  - MEMCACHED_ALL_FAILURE: 전체 element가 저장되지 않음.

여러 key들이 가리키는 list들에 각각 하나의 element를 삽입하는 예시는 아래와 같다.

```c
int arcus_list_element_piped_insert_bulk(memcached_st *memc)
{
  const char * const keys[]= { "list:a_key1", "list:a_key2", "list:a_key3" };
  const char *value= "value";
  size_t keys_len[3];
  size_t number_of_keys = 3;
  int32_t index= -1;
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

  rc= memcached_lop_piped_insert_bulk(memc, keys, keys_len,
                                      number_of_keys, index,
                                      value, strlen(value),
                                      &attributes, results, &piped_rc);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_lop_piped_insert: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  if (piped_rc == MEMCACHED_ALL_FAILURE) {
    fprintf(stderr, "Failed to memcached_lop_piped_insert_bulk: All Failures\n");
    return -1;
  }

  if (piped_rc == MEMCACHED_SOME_SUCCESS) {
    for (size_t i=0; i<number_of_keys; i++) {
      if (results[i] != MEMCACHED_STORED && results[i] != MEMCACHED_CREATED_STORED) {
        fprintf(stderr, "Failed to memcached_lop_piped_insert_bulk: %s[%d] %d(%s)\n",
                keys[i], index, results[i], memcached_strerror(memc, results[i]));
      }
    }
  }

  assert(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_SOME_ERRORS);
  return 0;
}
```
