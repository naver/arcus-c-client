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

``` c
memcached_return_t
memcached_lop_create(memcached_st *ptr,
                     const char *key, size_t key_length,
                     memcached_coll_create_attrs_st *attributes)
```

- key: list item의 key
- attributes: list item의 속성 정보

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_CREATED: List가 생성됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_EXISTS: 동일한 key를 가진 List가 이미 존재함.


Item 생성 시, item 속성 정보를 가지는 attributes 구조체가 필요하다.
Item 속성 정보를 가지는 attributes 구조체는 아래와 같이 초기화한다.
Attributes 구조체의 초기화 시에는 필수 속성 정보인 flags, exptime, maxcount 만을 설정할 수 있다.


``` c
memcached_return_t
memcached_coll_create_attrs_init(memcached_coll_create_attrs_st *attributes,
                                 uint32_t flags, uint32_t exptime, uint32_t maxcount)
```

그 외에, 선택적 속성들은 attributes 구조체를 초기화한 이후,
아래의 함수를 이용하여 개별적으로 지정할 수 있다.

``` c
memcached_return_t
memcached_coll_create_attrs_set_flags(memcached_coll_create_attrs_st *attributes,
                                      uint32_t flags)
memcached_return_t
memcached_coll_create_attrs_set_expiretime(memcached_coll_create_attrs_st *attributes,
                                           uint32_t expiretime)
memcached_return_t
memcached_coll_create_attrs_set_maxcount(memcached_coll_create_attrs_st *attributes,
                                         uint32_t maxcount)
memcached_return_t
memcached_coll_create_attrs_set_overflowaction(memcached_coll_create_attrs_st *attributes,
                                               memcached_coll_overflowaction_t overflowaction)
memcached_return_t
memcached_coll_create_set_unreadable(memcached_coll_create_attrs_st *attributes,
                                     bool is_unreadable)
```

- memcached_coll_create_attrs_set_flags : attributes에 flags 값을 설정한다.
- memcached_coll_create_attrs_set_expiretime : attributes에 expire time을 설정한다.
- memcached_coll_create_attrs_set_maxcount : attributes에 maxcount 값을 설정한다.
- memcached_coll_create_attrs_set_overflowaction : List의 Element 개수가 maxcount를 넘어 섰을 때(overflow)의 동작을 설정한다.
  - OVERFLOWACTION_ERROR: overflow가 발생하면 오류(MEMCACHED_OVERFLOWED)를 반환하도록 한다.
  - OVERFLOWACTION_HEAD_TRIM: overflow가 발생하면 가장 작은 index를 갖는 element를 삭제한다.
  - OVERFLOWACTION_TAIL_TRIM: overflow가 발생하면 가장 큰 index를 갖는 element를 삭제한다.
- memcached_coll_create_set_unreadable : 생성 시 unreadable 상태로 만들 것인지 설정한다.
  Unreadable 상태로 생성된 List는 readable 상태가 되기 전까지 조회할 수 없다.
  이렇게 unreadable 상태로 생성된 B+tree를 readable 상태로 만들기 위해서는 memcached_set_attrs를 사용해야 한다.


List item을 생성하는 예제는 아래와 같다.

``` c
void arcus_list_item_create(memcached_st *memc)
{
    memcached_return_t rc;

    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    // 비어있는 리스트를 생성한다.
    rc= memcached_lop_create(memc, "list:an_empty_list", strlen("list:an_empty_list"),
                             &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED == memcached_get_last_response_code(memc));

    // 리스트가 이미 존재한다면 실패한다.
    rc= memcached_lop_create(memc, "list:an_empty_list", strlen("list:an_empty_list"),
                             &attributes);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_EXISTS == memcached_get_last_response_code(memc));
}
```

## List Element 삽입


List에 하나의 element를 삽입하는 함수이다.

``` c
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
- value, value_lenth: 삽입할 element의 value
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

List element를 삽입하는 예제는 아래와 같다.

``` c
void arcus_list_element_insert(memcached_st *memc)
{
    memcached_return_t rc;

    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    int index= 0;

    // 1. CREATED_STORED
    index= 0;
    rc= memcached_lop_insert(memc, "a_list", strlen("a_list"), index,
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));

    // 2. STORED
    index= 1;
    rc= memcached_lop_insert(memc, "a_list", strlen("a_list"), index,
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_STORED == memcached_get_last_response_code(memc));

    // 3. OUT_OF_RANGE
    index= 10;
    rc= memcached_lop_insert(memc, "a_list", strlen("a_list"), index,
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_OUT_OF_RANGE == rc);
}
```

## List Element 삭제

List element를 삭제하는 함수는 두 가지가 있다.

첫째, 하나의 list index로 하나의 element만 삭제하는 함수이다.

``` c
memcached_return_t
memcached_lop_delete(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const int32_t index, bool drop_if_empty)
```

- index: single list index (0-based index)
  - 0, 1, 2, ... : list의 앞에서 시작하여 각 element 위치를 나타냄
  - -1, -2, -3, ... : list의 뒤에서 시작하여 각 element 위치를 나타냄
- drop_if_empty: element 삭제로 empty list가 될 경우, 그 list도 삭제할 것인지를 지정

둘째, list index range로 다수의 element를 삭제하는 함수이다.

``` c
memcached_return_t
memcached_lop_delete_by_range(memcached_st *ptr,
                              const char *key, size_t key_length,
                              const int32_t from, const int32_t to,
                              bool drop_if_empty)
```

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


List element를 삭제하는 예제는 아래와 같다.

``` c
void arcus_list_element_delete(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    // 테스트 데이터를 삽입한다.
    rc= memcached_lop_insert(memc, "list:a_list", strlen("list:a_list"), -1,
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);

    rc= memcached_lop_insert(memc, "list:a_list", strlen("list:a_list"), -1,
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);

    // 1번 인덱스의 element를 삭제한다.
    rc= memcached_lop_delete(memc, "list:a_list", strlen("list:a_list"), 1, true);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED == memcached_get_last_response_code(memc));

    // 0번 인덱스의 element를 삭제하고, empty 상태가 된 리스트를 삭제한다.
    rc= memcached_lop_delete(memc, "list:a_list", strlen("list:a_list"), 0, true);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED_DROPPED == memcached_get_last_response_code(memc));
}
```

## List Element 조회

List element를 조회하는 함수는 두 가지가 있다.

첫째, 하나의 list index로 하나의 element만 조회하는 함수이다.

``` c
memcached_return_t
memcached_lop_get(memcached_st *ptr,
                  const char *key, size_t key_length,
                  const int32_t index,
                  bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

- index: single list index (0-based index)
  - 0, 1, 2, ... : list의 앞에서 시작하여 각 element 위치를 나타냄
  - -1, -2, -3, ... : list의 뒤에서 시작하여 각 element 위치를 나타냄
- with_delete: 조회와 함께 삭제도 수행할 것인지를 지정
- drop_if_empty: element 삭제로 empty list가 될 경우, 그 list도 삭제할 것인지를 지정


둘째, list index range로 다수의 element를 조회하는 함수이다.

``` c
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


``` c
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

List element를 조회하는 예제는 아래와 같다.

``` c
void arcus_list_element_get(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);
    memcached_return_t rc;
    memcached_coll_result_st *result;

    for (uint32_t i=0; i<maxcount; i++)    {
        char buffer[15];
        size_t buffer_len= snprintf(buffer, 15, "value%d", i);
        rc= memcached_lop_insert(memc, "a_list", strlen("a_list"), i,
                                 buffer, buffer_len, &attributes);
        assert(MEMCACHED_SUCCESS == rc);
    }

    // 조회 범위에 아무런 element가 없는 경우
    result = memcached_coll_result_create(memc, NULL);

    rc= memcached_lop_get_by_range(memc, "a_list", strlen("a_list"),
                                   maxcount, maxcount+maxcount, false, false, result);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_NOTFOUND_ELEMENT == memcached_get_last_response_code(memc));

    memcached_coll_result_free(result);

    // 조회와 동시에 조회된 element를 삭제한다. Empty 상태가 된 리스트는 삭제된다.
    result = memcached_coll_result_create(memc, NULL);

    rc= memcached_lop_get_by_range(memc, "a_list", strlen("a_list"),
                                   0, maxcount, true, true, result);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED_DROPPED == memcached_get_last_response_code(memc));

    // 조회 결과를 확인한다.
    for (size_t i=0; i<memcached_coll_result_get_count(result); i++)
    {
        char buffer[15];
        snprintf(buffer, 15, "value%u", (int)i);
        assert(0 == strcmp(buffer, memcached_coll_result_get_value(result, i)));
    }

    // 조회 결과를 삭제한다.
    memcached_coll_result_free(result);
}
```

## List Element 일괄 삽입

List에 여러 element를 한번에 삽입하는 함수는 두 가지가 있다.

첫째, 하나의 key가 가리키는 list에 다수의 element를 삽입하는 함수이다.

``` c
memcached_return_t
memcached_lop_piped_insert(memcached_st *ptr,
                           const char *key, const size_t key_length,
                           const size_t numr_of_piped_items,
                           const int32_t *indexes,
                           const char * const *values, const size_t *values_length,
                           memcached_coll_create_attrs_st *attributes,
                           memcached_return_t *results,
                           memcached_return_t *piped_rc)
```

- key, key_length: 하나의 key를 지정
- numr_of_piped_items: 한번에 삽입할 element 개수
- indexes: list index array (0-based index)
- values, values_length: 다수 element 각각의 value와 그 길이
- attributes: 해당 list가 없을 시에, attrbiutes에 따라 list를 생성 후에 삽입한다.

둘째, 여러 key들이 가리키는 list들에 각각 하나의 element를 삽입하는 함수이다.

``` c
memcached_return_t
memcached_lop_piped_insert_bulk(memcached_st *ptr,
                                const char * const *keys, const size_t *key_length,
                                size_t number_of_keys,
                                const int32_t index,
                                const char *value, size_t value_length,
                                memcached_coll_create_attrs_st *attributes,
                                memcached_return_t *results,
                                memcached_return_t *piped_rc)
```

- keys, keys_length: 다수 key들을 지정
- number_of_keys: key들의 수
- index: list index (0-based index)
- value, value_length: 각 list에 삽입할 element의 value와 그 길이
- attributes: 해당 list가 없을 시에, attrbiutes에 따라 list를 생성 후에 삽입한다.

List element 일괄 삽입의 결과는 아래의 인자를 통해 받는다.

- results: 일괄 삽입 결과가 주어진 key 또는 (key, element) 순서대로 저장된다.
  - STORED: element만 삽입
  - CREATE_STORED: list item 생성 후에, element 삽입
- piped_rc: 일괄 저장의 전체 결과를 담고 있다
  - MEMCACHED_ALL_SUCCESS: 모든 element가 저장됨.
  - MEMCACHED_SOME_SUCCESS: 일부 element가 저장됨.
  - MEMCACHED_ALL_FAILURE: 전체 element가 저장되지 않음.

List element 일괄 삽입의 예제는 아래와 같다.

``` c
void arcus_list_element_piped_insert(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= MEMCACHED_COLL_MAX_PIPED_CMD_SIZE;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;
    memcached_return_t piped_rc; // pipe operation의 전체 성공 여부
    memcached_return_t results[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE]; // 각 key에 대한 응답코드

    // 테스트 데이터
    int32_t indexes[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE];
    char **values = (char **)malloc(sizeof(char *) * MEMCACHED_COLL_MAX_PIPED_CMD_SIZE);
    size_t valuelengths[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE];

    for (uint32_t i=0; i<maxcount; i++)
    {
        indexes[i] = i;
        values[i]= (char *)malloc(sizeof(char) * 15);
        valuelengths[i]= snprintf(values[i], 15, "value%d", i);
    }


    // piped insert를 요청한다.
    rc= memcached_lop_piped_insert(memc, "a_list", strlen("a_list"),
            MEMCACHED_COLL_MAX_PIPED_CMD_SIZE,
            indexes, values, valuelengths,
            &attributes, results, &piped_rc);

    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_ALL_SUCCESS == piped_rc);

    // 각 key에 대한 결과를 확인한다.
    for (size_t i=0; i<maxcount; i++)
    {
        assert(MEMCACHED_STORED == results[i] or MEMCACHED_CREATED_STORED == results[i]);
    }

    for (uint32_t i=0; i<maxcount; i++)
        free((void *)values[i]);
    free((void *)values);
}
```
