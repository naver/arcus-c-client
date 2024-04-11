# Map Item

Map item은 하나의 key에 대해 hash 구조 기반으로 mkey & value 쌍을 data 집합으로 가진다.

**제약 조건**
- 저장 가능한 최대 element 개수: 디폴트 4,000개 (attribute 설정으로 최대 50,000개 확장 가능)
- 각 element에서 value 최대 크기 : 16KB
- mkey 최대 길이는 250 바이트 이고, 하나의 map에 중복된 mkey는 허용하지 않는다.

Map item에 대해 수행 가능한 기본 연산들은 아래와 같다.

- [Map Item 생성](06-map-API.md#map-item-생성) (Map Item 삭제는 key-value item 삭제 함수로 수행한다)
- [Map Element 삽입](06-map-API.md#map-element-삽입)
- [Map Element 변경](06-map-API.md#map-element-변경)
- [Map Element 삭제](06-map-API.md#map-element-삭제)
- [Map Element 조회](06-map-API.md#map-element-조회)

여러 map element들에 대해 한번에 일괄 수행하는 연산은 다음과 같다.

- [Map Element 일괄 삽입](06-map-API.md#map-element-일괄-삽입)


## Map Item 생성


새로운 empty map item을 생성한다.

``` c
memcached_return_t
memcached_mop_create(memcached_st *ptr,
                     const char *key, size_t key_length,
                     memcached_coll_create_attrs_st *attributes)
```

- key: map item의 key
- attributes: map item의 속성 정보

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_CREATED: Map이 생성됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_EXISTS: 동일한 key를 가진 Map이 이미 존재함.


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
- memcached_coll_create_attrs_set_overflowaction : Map의 Element 개수가 maxcount를 넘어 섰을 때(overflow)의 동작을 설정한다.
  - OVERFLOWACTION_ERROR: overflow가 발생하면 오류(MEMCACHED_OVERFLOWED)를 반환하도록 한다.
  - OVERFLOWACTION_HEAD_TRIM: Map은 지원하지 않는다.
  - OVERFLOWACTION_TAIL_TRIM: Map은 지원하지 않는다.
- memcached_coll_create_set_unreadable : 생성 시 unreadable 상태로 만들 것인지 설정한다.
  Unreadable 상태로 생성된 Map은 readable 상태가 되기 전까지 조회할 수 없다.
  이렇게 unreadable 상태로 생성된 Map을 readable 상태로 만들기 위해서는 memcached_set_attrs를 사용해야 한다.


Map item을 생성하는 예제는 아래와 같다.

``` c
void arcus_map_item_create(memcached_st *memc)
{
    memcached_return_t rc;

    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    // 비어있는 맵을 생성한다.
    rc= memcached_mop_create(memc, "map:an_empty_map", strlen("map:an_empty_map"),
                             &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED == memcached_get_last_response_code(memc));

    // 맵이 이미 존재한다면 실패한다.
    rc= memcached_mop_create(memc, "map:an_empty_map", strlen("map:an_empty_map"),
                             &attributes);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_EXISTS == memcached_get_last_response_code(memc));
}
```

## Map Element 삽입


Map에 하나의 element를 삽입하는 함수이다.

``` c
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

Map element를 삽입하는 예제는 아래와 같다.

``` c
void arcus_map_element_insert(memcached_st *memc)
{
    memcached_return_t rc;

    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    // 1. CREATED_STORED
    rc= memcached_mop_insert(memc, "a_map", strlen("a_map"), "mkey", strlen("mkey"),
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));

    // 2. STORED
    rc= memcached_mop_insert(memc, "a_map", strlen("a_map"), "mkey1", strlen("mkey1"),
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_STORED == memcached_get_last_response_code(memc));

    // 3. ELEMENT_EXIST
    rc= memcached_mop_insert(memc, "a_map", strlen("a_map"), "mkey1", strlen("mkey1"),
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_ELEMENT_EXISTS == rc);
}
```

## Map Element 변경


Map에 하나의 element를 변경하는 함수이다. 주어진 mkey를 가진 element의 value를 변경한다.

``` c
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

Map element를 변경하는 예제는 아래와 같다.

``` c
void arcus_map_element_update(memcached_st *memc)
{
    memcached_return_t rc;

    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    // 1. CREATED_STORED
    rc= memcached_mop_insert(memc, "a_map", strlen("a_map"), "mkey", strlen("mkey"),
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));

    // 2. UPDATED
    rc= memcached_mop_update(memc, "a_map", strlen("a_map"), "mkey", strlen("mkey"),
                             "new_value", strlen("new_value"));
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_UPDATED == memcached_get_last_response_code(memc));
}
```

## Map Element 삭제

Map element를 삭제하는 함수는 두 가지가 있다.

첫째, map에서 하나의 mkey로 하나의 element만 삭제하는 함수이다.

``` c
memcached_return_t
memcached_mop_delete(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const char *mkey, size_t mkey_length,
                     bool drop_if_empty)
```

둘째, map의 element 전체를 삭제하는 함수이다.

``` c
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


Map element를 삭제하는 예제는 아래와 같다.

``` c
void arcus_map_element_delete(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    // 테스트 데이터를 삽입한다.
    rc= memcached_mop_insert(memc, "map:a_map", strlen("map:a_map"), "mkey0", strlen("mkey0"),
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);

    rc= memcached_mop_insert(memc, "map:a_map", strlen("map:a_map"), "mkey1", strlen("mkey1"),
                             "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);

    // mkey0에 해당하는 element를 삭제한다.
    rc= memcached_mop_delete(memc, "map:a_map", strlen("map:a_map"), "mkey0", strlen("mkey0"), true);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED == memcached_get_last_response_code(memc));

    // 전체 map element를 삭제하고, empty 상태가 된 맵을 삭제한다.
    rc= memcached_mop_delete_all(memc, "map:a_map", strlen("map:a_map"), true);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED_DROPPED == memcached_get_last_response_code(memc));
}
```

## Map Element 조회

Map element를 조회하는 함수는 세 가지가 있다.

첫째, 하나의 map mkey로 하나의 element만 조회하는 함수이다.

``` c
memcached_return_t
memcached_mop_get(memcached_st *ptr,
                  const char *key, size_t key_length,
                  const char *mkey, size_t mkey_length,
                  bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

둘째, map의 전체 element를 조회하는 함수이다.

``` c
memcached_return_t
memcached_mop_get_all(memcached_st *ptr,
                  const char *key, size_t key_length,
                  bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

셋째, mkey list에 해당하는 element들을 조회하는 함수이다.

``` c
memcached_return_t
memcached_mop_get_by_list(memcached_st *ptr,
                  const char *key, size_t key_length,
                  const char * const *mkeys, const size_t *mkeys_length,
                  bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

- key, key_length: map item의 key
- mkey, mkey_length: 조회할 mkey
- mkeys, mkeys_length: 조회할 mkey list
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
memcached_coll_result_get_mkey(memcached_coll_result_st *result, size_t index)
size_t
memcached_coll_result_get_mkey_length(memcached_coll_result_st *result, size_t index)
const char *
memcached_coll_result_get_value(memcached_coll_result_st *result, size_t index)
size_t
memcached_coll_result_get_value_length(memcached_coll_result_st *result, size_t index)
```

Map element를 조회하는 예제는 아래와 같다.

``` c
void arcus_map_element_get(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);
    memcached_return_t rc;
    memcached_coll_result_st *result;

    for (uint32_t i=0; i<maxcount; i++) {
        char mkey[15];
        char buffer[15];
        size_t mkey_len = snprintf(mkey, 15, "mkey%d", i);
        size_t buffer_len= snprintf(buffer, 15, "value%d", i);
        rc= memcached_mop_insert(memc, "a_map", strlen("a_map"), mkey, mkey_len,
                                 buffer, buffer_len, &attributes);
        assert(MEMCACHED_SUCCESS == rc);
    }

    // 조회 범위에 아무런 element가 없는 경우
    const char *not_mkeys[]= { "mkey1001", "mkey1010", "mkey1100" };
    size_t not_mkeys_length[]= { 8, 8, 8 };
    result = memcached_coll_result_create(memc, NULL);

    rc= memcached_mop_get_by_list(memc, "a_map", strlen("a_map"), not_mkeys, not_mkeys_length, 3,
                                  false, false, result);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_NOTFOUND_ELEMENT == memcached_get_last_response_code(memc));

    memcached_coll_result_free(result);

    // 조회와 동시에 조회된 element를 삭제한다.
    const char *mkeys[]= { "mkey0", "mkey1", "mkey2", "mkey3", "mkey4", "mkey5"};
    size_t mkeys_length[]= { 5, 5, 5, 5, 5, 5 };
    result = memcached_coll_result_create(memc, NULL);

    rc= memcached_mop_get_by_list(memc, "a_map", strlen("a_map"), mkeys, mkeys_length, 6,
                                  true, true, result);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED == memcached_get_last_response_code(memc));

    // 조회 결과를 확인한다.
    for (size_t i=0; i<memcached_coll_result_get_count(result); i++)
    {
        char mkey[15];
        char buffer[15];
        snprintf(mkey, 15, "mkey%u", (int)i);
        snprintf(buffer, 15, "value%u", (int)i);
        assert(0 == strcmp(mkey, memcached_coll_result_get_mkey(result, i)));
        assert(0 == strcmp(buffer, memcached_coll_result_get_value(result, i)));
    }

	// 전체 map element를 조회하고, 조회와 동시에 삭제한다. empty 상태가 된 맵을 삭제한다.
    result = memcached_coll_result_create(memc, NULL);

    rc= memcached_mop_get_all(memc, "a_map", strlen("a_map"), true, true, result);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED_DROPPED == memcached_get_last_response_code(memc));

    // 조회 결과를 삭제한다.
    memcached_coll_result_free(result);
}
```

## Map Element 일괄 삽입

Map에 여러 elements를 한번에 삽입하는 함수는 두 가지가 있다.

첫째, 하나의 key가 가리키는 map에 다수의 elements를 삽입하는 함수이다.

``` c
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

둘째, 여러 key들이 가리키는 map들에 각각 하나의 element를 삽입하는 함수이다.

``` c
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

Map element 일괄 삽입의 예제는 아래와 같다.

``` c
void arcus_map_element_piped_insert(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= MEMCACHED_COLL_MAX_PIPED_CMD_SIZE;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;
    memcached_return_t piped_rc; // pipe operation의 전체 성공 여부
    memcached_return_t results[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE]; // 각 key에 대한 응답 코드

    // 테스트 데이터
    char **mkeys = (char **)malloc(sizeof(char *) * MEMCACHED_COLL_MAX_PIPED_CMD_SIZE);
    size_t mkeylengths[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE];
    char **values = (char **)malloc(sizeof(char *) * MEMCACHED_COLL_MAX_PIPED_CMD_SIZE);
    size_t valuelengths[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE];

    for (uint32_t i=0; i<maxcount; i++)
    {
        mkeys[i]= (char *)malloc(sizeof(char) * 15);
        mkeylengths[i]= snprintf(values[i], 15, "mkey%d", i);
        values[i]= (char *)malloc(sizeof(char) * 15);
        valuelengths[i]= snprintf(values[i], 15, "value%d", i);
    }


    // piped insert를 요청한다.
    rc= memcached_mop_piped_insert(memc, "a_map", strlen("a_map"),
            MEMCACHED_COLL_MAX_PIPED_CMD_SIZE,
            mkeys, mkeylengths,
            values, valuelengths,
            &attributes, results, &piped_rc);

    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_ALL_SUCCESS == piped_rc);

    // 각 key에 대한 결과를 확인한다.
    for (size_t i=0; i<maxcount; i++)
    {
        assert(MEMCACHED_STORED == results[i] or MEMCACHED_CREATED_STORED == results[i]);
    }

    for (uint32_t i=0; i<maxcount; i++)
    {
        free((void *)mkeys[i]);
        free((void *)values[i]);
    }
    free((void *)mkeys);
    free((void *)values);
}
```
