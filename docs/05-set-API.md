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

``` c
memcached_return_t
memcached_sop_create(memcached_st *ptr,
                     const char *key, size_t key_length,
                     memcached_coll_create_attrs_st *attributes)
```

- key: set item의 key
- attributes: set item의 속성 정보

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_CREATED: set이 생성됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_EXISTS: 동일한 key를 가진 set이 이미 존재함.


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

```c
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

Set item을 생성하는 예제는 아래와 같다.

``` c
void arcus_set_item_create(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    // 비어 있는 Set을 생성한다.
    rc= memcached_sop_create(memc, "set:an_empty_set", strlen("set:an_empty_set"),
                             &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED == memcached_get_last_response_code(memc));

    // 이미 존재하는 key를 이용하여 Set을 생성하면 오류가 발생한다.
    rc= memcached_sop_create(memc, "set:an_empty_set", strlen("set:an_empty_set"),
                             &attributes);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_EXISTS == memcached_get_last_response_code(memc));
}
```

## Set Element 삽입

Set에 하나의 element를 삽입하는 함수이다.

``` c
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

Set에 하나의 element를 삽입하는 예제는 아래와 같다.

``` c
void arcus_set_element_insert(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    // 입력할 때 Set이 존재하지 않으면 새로 생성한 뒤 입력한다.
    rc= memcached_sop_insert(memc, "set:a_set", strlen("set:a_set"),
                            "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));

    // 이미 존재하는 값을 입력하려 하면 오류가 발생한다.
    rc= memcached_sop_insert(memc, "set:a_set", strlen("set:a_set"),
                             "value", strlen("value"), NULL);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_ELEMENT_EXISTS == memcached_get_last_response_code(memc));

    // Set의 element 개수가 maxcount에 다다를 때까지 입력한다.
    for (uint32_t i=1; i<maxcount; i++)
    {
        char buffer[15];
        size_t buffer_len= snprintf(buffer, 15, "value%d", i);
        rc= memcached_sop_insert(memc, "set:a_set", strlen("set:a_set"),
                                 buffer, buffer_len, &attributes);
        assert(MEMCACHED_SUCCESS == rc);
        assert(MEMCACHED_STORED == memcached_get_last_response_code(memc));
    }

    // Set의 element 개수가 maxcount에 다다른 상태에서 새로운 값을 입력하면 오류가 발생한다.
    rc= memcached_sop_insert(memc, "set:a_set", strlen("set:a_set"),
                             "last_value", strlen("last_value"), &attributes);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_OVERFLOWED == memcached_get_last_response_code(memc));
}
```

## Set Element 삭제

Set에서 주어진 value를 가진 element를 삭제하는 함수이다.

``` c
memcached_return_t
memcached_sop_delete(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const char *value, size_t value_length,
                     bool drop_if_empty)
```

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


Set에서 하나의 element를 삭제하는 예제이다.

``` c
void arcus_set_element_delete(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    // 테스트 데이터를 입력한다.
    rc= memcached_sop_insert(memc, "set:a_set", strlen("set:a_set"),
                             "value1", strlen("value1"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));

    rc= memcached_sop_insert(memc, "set:a_set", strlen("set:a_set"),
                             "value2", strlen("value2"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_STORED == memcached_get_last_response_code(memc));

    // 삭제를 요청한 값이 Set에 존재하지 않으면 오류가 발생한다.
    rc= memcached_sop_delete(memc, "set:a_set", strlen("set:a_set"),
                             "no value", strlen("no value"), true);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_NOTFOUND_ELEMENT == memcached_get_last_response_code(memc));

    // 값 하나를 삭제한다.
    rc= memcached_sop_delete(memc, "set:a_set", strlen("set:a_set"),
                             "value1", strlen("value1"), true);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED == memcached_get_last_response_code(memc));

    // 값 하나를 삭제한다. 삭제 후 empty 상태가 된 Set은 삭제된다.
    rc= memcached_sop_delete(memc, "set:a_set", strlen("set:a_set"),
                             "value2", strlen("value2"), true);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED_DROPPED == memcached_get_last_response_code(memc));
}
```

## Set Element 존재 여부 확인

Set에서 주어진 value를 가진 element의 존재 여부를 확인한다.

``` c
memcached_return_t
memcached_sop_exist(memcached_st *ptr,
                    const char *key, size_t key_length,
                    const char *value, size_t value_length)
```

- value, value_length: 존재 여부를 확인할 element의 value

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_EXIST: 주어진 value를 가지는 element가 존재함.
  - MEMCACHED_NOT_EXIST: 주어진 value를 가지는 element가 존재하지 않음.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 Set이 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 Set이 아님.
  - MEMCACHED_UNREADABLE: 주어진 key에 해당하는 Set이 unreadable 상태임.

Set element의 존재 여부를 확인하는 예제는 아래와 같다.

``` c
void arcus_set_element_exist(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    // 테스트 데이터를 입력한다.
    rc= memcached_sop_insert(memc, "set:a_set", strlen("set:a_set"),
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));

    // 요청한 데이터가 존재하는 않는 경우
    rc= memcached_sop_exist(memc, "set:a_set", strlen("set:a_set"),
                            "no value", strlen("no value"));
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_NOT_EXIST == memcached_get_last_response_code(memc));

    // 요청한 데이터가 존재하는 경우
    rc= memcached_sop_exist(memc, "set:a_set", strlen("set:a_set"),
                            "value", strlen("value"));
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_EXIST == memcached_get_last_response_code(memc));
}
```

## Set Element 조회

Set element를 조회하는 함수이다. 이 함수는 임의의 count 개 element를 조회한다.

``` c
memcached_return_t
memcached_sop_get(memcached_st *ptr,
                  const char *key, size_t key_length,
                  size_t count, bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

- count: 조회할 element 개수
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

Set element를 조회하는 예제는 아래와 같다.

``` c
void arcus_set_element_get(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    for (uint32_t i=0; i<maxcount; i++)
    {
        char buffer[15];
        size_t buffer_len= snprintf(buffer, 15, "value%d", i);
        rc= memcached_sop_insert(memc, "a_set", strlen("a_set"),
                                 buffer, buffer_len, &attributes);
        assert(MEMCACHED_SUCCESS == rc);
    }

    // 조회 범위에 아무런 element가 없는 경우
    rc= memcached_sop_create(memc, "an_empty_set", strlen("an_empty_set"),
                             &attributes);
    assert(MEMCACHED_SUCCESS == rc);

    result = memcached_coll_result_create(memc, NULL);

    rc= memcached_sop_get(memc, "an_empty_set", strlen("an_empty_set"),
                          maxcount, false, false, result);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_NOTFOUND_ELEMENT == memcached_get_last_response_code(memc));

    memcached_coll_result_free(result);

    // 조회와 동시에 조회된 element를 삭제한다. Empty 상태가 된 Set은 삭제된다.
    result = memcached_coll_result_create(memc, NULL);

    rc= memcached_sop_get(memc, "a_set", strlen("a_set"),
                          maxcount, true, true, result);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED_DROPPED == memcached_get_last_response_code(memc));
    assert(maxcount == memcached_coll_result_get_count(result));

    // 조회 결과를 삭제한다.
    memcached_coll_result_free(result);
}
```

## Set Element 일괄 삽입

Set에 여러 element를 한번에 삽입하는 함수는 두 가지가 있다.

첫째, 하나의 key가 가리키는 set에 다수의 element를 삽입하는 함수이다.

``` c
memcached_return_t
memcached_sop_piped_insert(memcached_st *ptr,
                           const char *key, const size_t key_length,
                           const size_t num_of_piped_items,
                           const char * const *values, const size_t *values_length,
                           memcached_coll_create_attrs_st *attributes,
                           memcached_return_t *results,
                           memcached_return_t *piped_rc) 
```

- key, key_length: 하나의 key를 지정
- numr_of_piped_items: 한번에 삽입할 element 개수
- values, values_length: 다수 element 각각의 value와 그 길이
- attributes: 해당 set이 없을 시에, attributes에 따라 set을 생성 후에 삽입한다.

둘째, 여러 key들이 가리키는 set들에 각각 하나의 element를 삽입하는 함수이다. 

``` c
memcached_return_t
memcached_sop_piped_insert_bulk(memcached_st *ptr,
                                const char * const *keys,
                                const size_t *keys_length,
                                const size_t num_of_keys,
                                const char *value, size_t value_length,
                                memcached_coll_create_attrs_st *attributes,
                                memcached_return_t *results,
                                memcached_return_t *piped_rc)
```

- keys, keys_length: 다수 key들을 지정
- numr_of_keys: key들의 수
- values, values_length: 각 set에 삽입할 element의 value와 그 길이
- attributes: 해당 set이 없을 시에, attrbiutes에 따라 set을 생성 후에 삽입한다.

Set element 일괄 삽입의 결과는 아래의 인자를 통해 받는다.

- results: 일괄 삽입 결과가 주어진 key 또는 (key, element) 순서대로 저장된다.
  - STORED: element만 삽입
  - CREATE_STORED: set item 생성 후에, element 삽입
- piped_rc: 일괄 저장의 전체 결과를 담고 있다
  - MEMCACHED_ALL_SUCCESS: 모든 element가 저장됨.
  - MEMCACHED_SOME_SUCCESS: 일부 element가 저장됨.
  - MEMCACHED_ALL_FAILURE: 전체 element가 저장되지 않음.

Set element 일괄 삽입의 예제는 아래와 같다.

``` c
void arcus_set_element_piped_insert(memcached_st *memc)
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
        values[i]= (char *)malloc(sizeof(char) * 15);
        valuelengths[i]= snprintf(values[i], 15, "value%d", i);
    }

    // piped insert를 요청한다.
    rc= memcached_sop_piped_insert(memc, "a_set", strlen("a_set"),
                                   MEMCACHED_COLL_MAX_PIPED_CMD_SIZE,
                                   values, valuelengths, &attributes,
                                   results, &piped_rc);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_ALL_SUCCESS == piped_rc);

    // 각 key에 대한 결과를 확인한다.
    for (size_t i=0; i<maxcount; i++)
    {
        assert(MEMCACHED_STORED == results[i] or
                MEMCACHED_CREATED_STORED == results[i]);
    }

    for (uint32_t i=0; i<maxcount; i++)
        free((void*)values[i]);
    free((void*)values);
}
```

## Set Element 일괄 존재 여부 확인

Set에서 여러 element의 존재 여부를 한번에 확인하는 함수이다.

``` c
memcached_return_t
memcached_sop_piped_exist(memcached_st *ptr,
                          const char *key, size_t key_length,
                          const size_t number_of_piped_items,
                          const char * const *values, const size_t *values_length,
                          memcached_return_t *results,
                          memcached_return_t *piped_rc)
```

- key, key_length: 하나의 key를 지정
- numr_of_piped_items: 한번에 확인할 element 개수
- values, values_length: 각 element의 value와 길이

Set element 일괄 존재 여부 확인의 결과는 아래의 인자를 통해 받는다.

- results: 각 value에 대한 element 존재 여부 결과를 순서대로 저장한다.
  - MEMCACHED_EXIST
  - MEMCACHED_NOT_EXIST
- piped_rc: 일괄 존재 여부 확인 결과를 담고 있다.
  - MEMCACHED_ALL_EXIST: 모든 element가 존재함.
  - MEMCACHED_SOME_EXIST: 일부 element가 존재함.
  - MEMCACHED_ALL_NOT_EXIST: 모든 element가 존재하지 않음.

Set element 일괄 존재 여부 확인의 예는 아래와 같다.

``` c
void arcus_set_element_piped_exist(memcached_st *memc)
{

    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= MEMCACHED_COLL_MAX_PIPED_CMD_SIZE;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;
    char **values = (char **)malloc(sizeof(char *) * MEMCACHED_COLL_MAX_PIPED_CMD_SIZE);
    size_t valuelengths[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE];

    // 비어 있는 Set을 하나 생성한다.
    memcached_sop_create(memc, "set:a_set", strlen("set:a_set"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED == memcached_get_last_response_code(memc));

    // 테스트 데이터를 입력한다.
    for (uint32_t i=0; i<maxcount; i++)
    {
        values[i]= (char *)malloc(sizeof(char) * 15);
        valuelengths[i]= snprintf(values[i], 15, "value%d", i);

        // 일부 데이터를 의도적으로 입력하지 않도록 한다.
        if ((i % 10) == 0)
            continue;

        rc= memcached_sop_insert(memc, "set:a_set", strlen("set:a_set"),
                                 values[i], valuelengths[i], &attributes);
        assert(MEMCACHED_SUCCESS == rc);
        assert(MEMCACHED_STORED == memcached_get_last_response_code(memc));

    }

    // Piped exist 명령을 수행한다.
    memcached_return_t piped_rc; // 전체 데이터 존재 여부
    memcached_return_t responses[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE]; // 각 데이터에 대한 존재 여부

    rc= memcached_sop_piped_exist(memc, "set:a_set", strlen("set:a_set"),
                                  maxcount, values, valuelengths,
                                  responses, &piped_rc);

    // 일부 데이터가 존재한다는 결과가 나왔다.
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_SOME_EXIST == piped_rc);

    // 실제 결과가 맞는지 검증해보자.
    for (uint32_t i=0; i<maxcount; i++)
    {
        if ((i % 10) == 0)
            assert(MEMCACHED_NOT_EXIST == responses[i]);
        else
            assert(MEMCACHED_EXIST == responses[i]);
    }

    for (uint32_t i=0; i<maxcount; i++)
        free(values[i]);
    free(values);
}
```
