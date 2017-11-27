## B+Tree Item

B+tree item은 하나의 key에 대해 b+tree 구조 기반으로 b+tree key(bkey)로 정렬된 data의 집합을 가진다.

**제약 조건**
- 저장 가능한 최대 element 개수 : 디폴트 4,000개 (attribute 설정으로 최대 50,000개 확장 가능)
- 각 element에서 value 최대 크기 : 4KB
- 하나의 b+tree 내에서 모든 element는 동일한 bkey 유형을 가져야 한다. 
  즉, 8바이트 unsigned integer bkey 유형과 byte array bkey 유형이 혼재할 수 없다.

B+tree item 구조와 기본 특징은 **[Arcus Server Ascii Protocol 문서의 내용](https://github.com/naver/arcus-memcached/blob/master/doc/arcus-collection-concept.md)**을
먼저 참고하기 바란다.

B+tree item 연산의 설명에 앞서, b+tree 조회 및 변경에 사용하는 자료구조를 설명한다.

- [Bkey(B+Tree Key)와 EFlag(Element Flag)](07-btree-API.md#bkeybtree-key%EC%99%80-eflagelement-flag)
- [Element Flag Filter 구조체](07-btree-API.md#element-flag-filter-%EA%B5%AC%EC%A1%B0%EC%B2%B4)
- [Element Flag Update 구조체](07-btree-API.md#element-flag-update-%EA%B5%AC%EC%A1%B0%EC%B2%B4)
- [B+Tree Query 구조체](07-btree-API.md#btree-query-%EA%B5%AC%EC%A1%B0%EC%B2%B4)

B+tree item에 대해 수행가능한 기본 연산들은 다음과 같다.

- [B+Tree Item 생성](07-btree-API.md#btree-item-%EC%83%9D%EC%84%B1) (B+tree item 삭제는 key-value item 삭제 함수로 수행한다) 
- [B+Tree Element 삽입](07-btree-API.md#btree-element-%EC%82%BD%EC%9E%85)
- [B+Tree Element Upsert](07-btree-API.md#btree-element-upsert)
- [B+Tree Element 변경](07-btree-API.md#btree-element-%EB%B3%80%EA%B2%BD)
- [B+Tree Element 삭제](07-btree-API.md#btree-element-%EC%82%AD%EC%A0%9C)
- [B+Tree Element 값의 증감](07-btree-API.md#btree-element-%EA%B0%92%EC%9D%98-%EC%A6%9D%EA%B0%90)
- [B+Tree Element 개수 확인](07-btree-API.md#btree-element-%EA%B0%9C%EC%88%98-%ED%99%95%EC%9D%B8)
- [B+Tree Element 조회](07-btree-API.md#btree-element-%EC%A1%B0%ED%9A%8C)

여러 b+tree element들에 대해 한번에 일괄 수행하는 연산은 다음과 같다.

- [B+Tree Element 일괄 삽입](07-btree-API.md#btree-element-%EC%9D%BC%EA%B4%84-%EC%82%BD%EC%9E%85)
- [B+Tree Element 일괄 조회](07-btree-API.md#btree-element-%EC%9D%BC%EA%B4%84-%EC%A1%B0%ED%9A%8C)

여러 b+tree element들에 대해 sort-merge 조회하는 연산을 제공한다.

- [B+Tree Element Sort-Merge 조회](07-btree-API.md#btree-element-sort-merge-%EC%A1%B0%ED%9A%8C) 

B+Tree내에서 element 순위(position)와 관련하여 아래 연산들을 제공한다.
- [B+Tree Element 순위 조회](07-btree-API.md#btree-element-순위-조회)
- [B+Tree 순위 기반의 Element 조회](07-btree-API.md#btree-순위-기반의-element-조회)
- [B+Tree 순위와 Element 동시 조회](07-btree-API.md#btree-순위와-element-동시-조회)

### BKey(B+Tree Key)와 EFlag(Element Flag)

B+tree item에서 사용 가능한 bkey 데이터 타입은 아래 두 가지이다.

- 8바이트 unsigned integer
- 최대 31 크기의 byte array 

eflag는 현재 b+tree element에만 존재하는 필드이다.
eflag 데이터 타입은 최대 31 크기의 byte array 타입만 가능하다.


### Element Flag Filter 구조체

B+tree의 element flag에 대한 filtering을 지정하기 위해선, `eflag_filter` 구조체를 사용해야 한다.

먼저, `eflag_filter` 구조체를 생성하는 API는 다음과 같다.
이를 통해, eflag의 전체/부분 값과 특정 값과의 compare 연산을 명시할 수 있다.

``` c
memcached_return_t memcached_coll_eflag_filter_init(memcached_ coll _eflag_filter_st *ptr,
                                                    const size_t fwhere,
                                                    const unsigned char *fvalue, const size_t fvalue_length,
                                                    memcached_coll_comp_t comp_op)
```

- fwhere: eflag에서 비교 연산을 취할 데이터의 시작 offset을 바이트 단위로 지정한다.
- fvalue: 비교 연산을 취할 데이터 값을 지정한다.
- comp_op: 비교 연산을 지정한다.
  - MEMCACHED_COLL_COMP_EQ
  - MEMCACHED_COLL_COMP_NE
  - MEMCACHED_COLL_COMP_LT
  - MEMCACHED_COLL_COMP_LE
  - MEMCACHED_COLL_COMP_GT
  - MEMCACHED_COLL_COMP_GE

eflag의 전체/부분 값에 대해 어떤 operand로 bitwise 연산을 취함으로써 eflag의 특정 bits들만을 골라내어 compare할 수 있다.
이와 같이 `eflag_filter`에 bitwise 연산을 추가할 경우에는 아래의 API를 이용할 수 있다.

``` c
memcached_return_t memcached_coll_eflag_filter_set_bitwise(memcached_coll_eflag_filter_st *ptr,
                                                    memcached_coll_bitwise_t bitwise_op,
                                                    const unsigned char *foperand, const size_t foperand_length)
```

- bitwise_op: bitwise 연산을 지정한다.
  - MEMCACHED_COLL_BITWISE_AND
  - MEMCACHED_COLL_BITWISE_OR
  - MEMCACHED_COLL_BITWISE_XOR
- foperand: eflag에서 bitwise 연산을 취할 operand를 지정한다.


### Element Flag Update 구조체

B+tree의 element flag를 변경하기 위해선 `eflag_update` 구조체를 사용해야 한다.

먼저, `eflag_update` 구조체를 생성하는 API는 다음과 같다.
이를 통해, 새로 변경하고자 하는 new element flag 값을 지정할 수 있다.

``` c
memcached_return_t memcached_coll_eflag_update_init(memcached_coll_eflag_update_st *ptr,
                                        const unsigned char *fvalue, const size_t fvalue_length)
```

만약, eflag의 부분 값만을 변경하고자 한다면, 아래의 함수를 이용할 수 있다.
아래 함수로 부분 값의 시작 위치를 명시하여야 하고, 부분 값과 `eflag_update`의 init 시에 명시한  fvalue에 대해
취할 bitwise 연산을 명시하여야 한다.

``` c
memcached_return_t memcached_coll_eflag_update_set_bitwise(memcached_coll_eflag_update_st *ptr,
                                        const size_t fwhere, memcached_coll_bitwise_t bitwise_op)
```

- fwhere: eflag에서 부분 변경할 부분 데이터의 시작 offset을 바이트 단위로 지정한다.
- bitwise_op: bitwise 연산을 지정한다.
  - MEMCACHED_COLL_BITWISE_AND
  - MEMCACHED_COLL_BITWISE_OR
  - MEMCACHED_COLL_BITWISE_XOR


### B+Tree Query 구조체

memcached_bop_query_st 구조체는 B+tree 조회 조건을 추상화하고 있으며 다양한 API에 사용될 수 있다.

먼저, 아래 함수는 하나의 bkey의 element를 조회하는 query 구조체를 생성한다.

``` c
memcached_return_t memcached_bop_query_init(memcached_bop_query_st *ptr,
                                            const uint64_t bkey,
                                            memcached_bop_eflag_filter_st *eflag_filter)
memcached_return_t memcached_bop_ext_query_init(memcached_bop_query_st *ptr,
                                            const unsigned char *bkey, const size_t bkey_length,
                                            memcached_bop_eflag_filter_st *eflag_filter)
```

아래 함수는 bkery range, element flag filter, offset과 count를 함께 명시하여 query 구조체를 생성한다.

``` c
memcached_return_t memcached_bop_range_query_init(memcached_bop_query_st *ptr,
                                            const uint64_t bkey_from, const uint64_t bkey_to,
                                            memcached_bop_eflag_filter_st *eflag_filter,
                                            const size_t offset, const size_t count)
memcached_return_t memcached_bop_ext_range_query_init (memcached_bop_query_st *ptr,
                                            const unsigned char *bkey_from, const size_t bkey_from_length,
                                            const unsigned char *bkey_to, const size_t bkey_to_length,
                                            memcached_bop_eflag_filter_st *eflag_filter,
                                            const size_t offset, const size_t count)
```


### B+Tree Item 생성

새로운 empty b+tree item을 생성한다.

``` c
memcached_return_t memcached_bop_create(memcached_st *ptr, const char *key, size_t key_length,
                                        memcached_coll_create_attrs_st *attributes)
```

- key: b+tree item의 key
- attributes: b+tree item의 속성 정보

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_CREATED: b+tree item이 생성됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_EXISTS: 동일한 key를 가진 b+tree가 이미 존재함.


Item 생성 시, item 속성 정보를 가지는 attributes 구조체가 필요하다.
Item 속성 정보를 가지는 attributes 구조체는 아래의 함수로 초기화하며,
필수 속성 정보인 flags, exptime, maxcount 만을 설정할 수 있다.


``` c
memcached_return_t memcached_coll_create_attrs_init(memcached_coll_create_attrs_st *attributes,
                                                    uint32_t flags, uint32_t exptime, uint32_t maxcount)
```

그 외에, 선택적 속성들은 attributes 구조체를 초기화한 이후,
아래의 함수를 이용하여 개별적으로 지정할 수 있다.

``` c
memcached_return_t memcached_coll_create_attrs_set_flags(memcached_coll_create_attrs_st *attributes, uint32_t flags)
memcached_return_t memcached_coll_create_attrs_set_expiretime(memcached_coll_create_attrs_st *attributes,
                                                            uint32_t expiretime)
memcached_return_t memcached_coll_create_attrs_set_maxcount(memcached_coll_create_attrs_st *attributes,
                                                            uint32_t maxcount)
memcached_return_t memcached_coll_create_attrs_set_overflowaction(memcached_coll_create_attrs_st *attributes,
                                                            memcached_coll_overflowaction_t overflowaction)
memcached_return_t memcached_coll_create_set_unreadable(memcached_coll_create_attrs_st *attributes)
```

B+tree item을 생성하는 예제는 아래와 같다.

``` c
void arcus_btree_item_create(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc

    // 비어 있는 B+tree를 생성한다.
    rc= memcached_bop_create(memc, "btree:an_empty_btree", strlen("btree:an_empty_btree"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED == memcached_get_last_response_code(memc));

    // 이미 존재하는 key를 갖는 B+tree를 생성하려 하면 오류를 반환한다.
    rc= memcached_bop_create(memc, "btree:an_empty_btree", strlen("btree:an_empty_btree"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_EXISTS == memcached_get_last_response_code(memc));
}
```

### B+Tree Element 삽입

B+Tree에 하나의 element를 삽입한다.
전자는 8바이트 unsigned interger 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

``` c
memcached_return_t memcached_bop_insert(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey, // bkey of 8 bytes unsigned integer type
                                        const unsigned char *eflag, size_t eflag_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attributes)
                     
memcached_return_t memcached_bop_ext_insert(memcached_st *ptr, const char *key, size_t key_length,
                                        const unsigned char *bkey, size_t bkey_length, // bkey of byte array type
                                        const unsigned char *eflag, size_t eflag_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attributes)
```

- key, key_length: b+tree item의 key
- bkey 또는 bkey, bkey_length: element의 bkey(b+tree key)
- eflag, eflag_length: element의 eflag(element flag), that is optional
- value, value_lenth: element의 value
- attributes: B+tree 없을 시에 attributes에 따라 empty b+tree item을 생성 후에 element 삽입한다.

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_STORED: 기존에 존재하던 B+tree에 element가 삽입됨.
  - MEMCACHED_CREATED_STORED: B+tree가 새롭게 생성되고 element가 삽입됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: B+tree가 존재하지 않음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.
  - MEMCACHED_ELEMENT_EXISTS: 동일한 bkey를 가진 element가 이미 존재하고 있음.
  - MEMCACHED_OVERFLOWED: Overflow 상태임. (overflowaction=error, maxcount=count)
  - MEMCACHED_OUT_OF_RANGE: 주어진 bkey가 maxcount 또는 maxbkeyrange를 위배하여 overflowaction에 따라 trim됨.


B+tree element를 삽입하는 예제는 아래와 같다.

``` c
void arcus_btree_element_insert(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    // 입력할 때 B+Tree가 존재하지 않으면 새로 생성한 뒤 입력한다.
    rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"), 1, NULL, 0,
            "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));

    // 이미 존재하는 bkey를 가지는 element는 입력할 수 없다.
    rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"), 1, NULL, 0,
            "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_ELEMENT_EXISTS == memcached_get_last_response_code(memc));

    // B+tree에 설정된 overflow action에 따라 입력 불가능한 bkey가 결정된다.
    // B+tree는 smallest_trim 정책이 기본값으로 설정되어 있으며, B+tree에 포함된 가장 작은 bkey 보다
    // 작은 bkey를 입력하려 하면 OUT_OF_RANGE 오류가 발생한다.
    rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"), 0, NULL, 0,
            "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_OUT_OF_RANGE == memcached_get_last_response_code(memc));
}
```

아커스에서 B+tree는 가질 수 있는 최대 엘리먼트 개수가 제한되어 있다. 이 제한 범위 안에서 사용자가 직접 B+tree 크기를 지정할 수도 있는데(maxcount), 이러한 제약조건 때문에 가득 찬 B+tree에 새로운 엘리먼트를 입력하면 설정에 따라 기존의 엘리먼트가 삭제될 수 있다. 이렇게 암묵적으로 삭제되는 엘리먼트를 입력(insert, upsert)시점에 획득할 수 있는 기능을 제공한다.

하지만, C client에서는 이 기능을 아직 제공하지 않고 있다.


### B+Tree Element Upsert

B+Tree에 하나의 element를 upsert하는 함수들이다.
Upsert 연산은 해당 element가 없으면 insert하고, 있으면 update하는 연산이다.
전자는 8바이트 unsigned interger 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

``` c
memcached_return_t memcached_bop_upsert(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey, // bkey of 8 bytes unsigned integer type
                                        const unsigned char *eflag, size_t eflag_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attributes)
                     
memcached_return_t memcached_bop_ext_upsert(memcached_st *ptr, const char *key, size_t key_length,
                                        const unsigned char *bkey, size_t bkey_length, // bkey of byte array type
                                        const unsigned char *eflag, size_t eflag_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attributes)
```

- key, key_length: b+tree item의 key
- bkey 또는 bkey, bkey_length: element의 bkey
- eflag, eflag_length: element의 eflag, that is optional
- value, value_lenth: element의 value
- attributes: B+tree 없을 시에 attributes에 따라 empty b+tree를 생성 후에 element 삽입한다.

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_STORED: 기존에 존재하던 B+tree에 element가 삽입됨.
  - MEMCACHED_CREATED_STORED: B+tree가 새롭게 생성되고 element가 삽입됨.
  - MEMCACHED_REPLACED: 기존에 존재하던 B+tree에서 동일한 bkey를 가진 element를 대체함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: B+tree가 존재하지 않음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.
  - MEMCACHED_OVERFLOWED: Overflow 상태임. (overflowaction=error, maxcount=count)
  - MEMCACHED_OUT_OF_RANGE: 삽입 위치가 b+tree의 element bkey 범위를 넘어섬.


B+tree element를 upsert하는 예제는 아래와 같다.

``` c
void arcus_btree_element_upsert(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    // 입력할 때 B+Tree가 존재하지 않으면 새로 생성한 뒤 입력한다.
    rc= memcached_bop_upsert(memc, "btree:a_btree", strlen("btree:a_btree"), 1, NULL, 0,
            "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));

    // 이미 존재하는 bkey를 가지는 element를 새로운 element로 대체한다.
    rc= memcached_bop_upsert(memc, "btree:a_btree", strlen("btree:a_btree"), 1, NULL, 0,
            "new value", strlen("new value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_REPLACED == memcached_get_last_response_code(memc));
}
```

### B+Tree Element 변경

B+Tree에서 하나의 element를 변경하는 함수이다. Element의 eflag 그리고/또는 value를 변경한다.
전자는 8바이트 unsigned interger 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

``` c
memcached_return_t memcached_bop_update(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey, // bkey of 8 bytes unsigned integer type
                                        memcached_coll_update_filter_st *update_filter,
                                        const char *value, size_t value_length)

memcached_return_t memcached_bop_ext_update(memcached_st *ptr, const char *key, size_t key_length,
                                        const unsigned char *bkey, size_t bkey_length, // bkey of byte array type
                                        memcached_coll_update_filter_st *update_filter,
                                        const char *value, size_t value_length)
```

- key, key_length: b+tree item의 key
- bkey 또는 bkey, bkey_length: element의 bkey
- update_filter: element의 eflag 변경 요청을 담은 구조체
- value, value_lenth: element의 new value

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_UPDATED: 주어진 bkey를 가진 element를 업데이트함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 B+tree가 없음.
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 bkey에 해당하는 element가 없음.
  - MEMCACHED_NOTHING_TO_UPDATE: 변경사항이 명시되지 않음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.
  - MEMCACHED_EFLAG_MISMATCH: update_filter에 명시된 eflag 데이터가 존재하지 않음.


B+tree element를 변경하는 예제는 아래와 같다.

``` c
void arcus_btree_element_update(memcached_st *memc)
{
    /* TODO : example code */
}
```

### B+Tree Element 삭제

B+tree에서 element를 삭제하는 함수들은 두 유형이 있다.

첫째, b+tree에서 특정 bkey를 가진 element에 대해 eflag filter 조건을 만족하면 삭제하는 함수이다.

``` c
memcached_return_t memcached_bop_delete(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey, // bkey of 8 bytes unsigned integer type
                                        memcached_coll_eflag_filter_st *eflag_filter, bool drop_if_empty)

memcached_return_t memcached_bop_ext_delete(memcached_st *ptr, const char *key, size_t key_length,
                                        const unsigned char *bkey, size_t bkey_length, // bkey of byte array type
                                        memcached_coll_eflag_filter_st *eflag_filter, bool drop_if_empty)
```

둘째, b+tree에서 bkey range에 해당하는 element들을 스캔하면서 
eflag filter 조건을 만족하는 N개의 element를 삭제하는 함수이다.

``` c
memcached_return_t memcached_bop_delete_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t from, const uint64_t to,
                                        memcached_coll_eflag_filter_st *eflag_filter,
                                        size_t count, bool drop_if_empty)

memcached_return_t memcached_bop_ext_delete_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                        const unsigned char *from, size_t from_length,
                                        const unsigned char *to, size_t to_length,
                                        memcached_coll_eflag_filter_st *eflag_filter,
                                        size_t count, bool drop_if_empty)                    
```

- key, key_length: b+tree item의 key
- bkeuy 또는 \<from, to\>:  삭제할 element의 bkey(b+tree key) 또는 bkey range 
- eflag_filter: element의 eflag에 대한 filter 조건
- count: 삭제할 element 개수, 0이면 bkey range의 모든 element가 삭제 대상이 된다.
- drop_if_empty: element 삭제로 empty b+tree가 될 경우, 그 b+tree도 삭제할 것인지를 지정

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_DELETED: 주어진 bkey에 해당하는 element를 삭제함.
  - MEMCACHED_DELETED_DROPPED: 주어진 bkey에 해당하는 element를 삭제하고, empty 상태가 된 B+tree도 삭제함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 B+tree가 없음.
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 bkey에 해당하는 element가 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.

B+tree element를 삭제하는 예제는 아래와 같다.

``` c
void arcus_btree_element_delete(memcached_st *memc)
{

    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);
    memcached_return_t rc;


    // 테스트 데이터를 입력한다.
    rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"), 0, NULL, 0,
                             "value", strlen("value"), &attributes);

    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));


    rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"), 1, NULL, 0,
                             "value", strlen("value"), &attributes);

    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_STORED == memcached_get_last_response_code(memc));


    // 삭제를 요청한 bkey가 B+tree에 존재하지 않으면 오류가 발생한다.
    rc= memcached_bop_delete(memc, "btree:a_btree", strlen("btree:a_btree"), 2, NULL, true);

    assert(MEMCACHED_SUCCESS != rc);
    assert(MEMCACHED_NOTFOUND_ELEMENT == memcached_get_last_response_code(memc));


    // bkey 범위가 0~10 사이인 element를 모두 삭제한다.
    rc= memcached_bop_delete_by_range(memc, "btree:a_btree", strlen("btree:a_btree"), 0, 10, NULL, 0, true);

    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED_DROPPED == memcached_get_last_response_code(memc));
}
```

### B+Tree Element 값의 증감

B+tree element의 값을 증가/감소시키는 함수는 아래와 같다. 
Element의 값은 숫자형 값이어야 한다.

전자는 8바이트 unsigned interger 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

``` c
memcached_return_t memcached_bop_incr(memcached_st *ptr, const char *key, size_t key_length,
                                      const uint64_t bkey, const uint64_t delta, uint64_t *value)
memcached_return_t memcached_bop_decr(memcached_st *ptr, const char *key, size_t key_length,
                                      const uint64_t bkey, const uint64_t delta, uint64_t *value)

memcached_return_t memcached_bop_ext_incr(memcached_st *ptr, const char *key, size_t key_length,
                                      const unsigned char *bkey, size_t bkey_length,
                                      const uint64_t delta, uint64_t *value)                   
memcached_return_t memcached_bop_ext_decr(memcached_st *ptr, const char *key, size_t key_length,
                                      const unsigned char *bkey, size_t bkey_length,
                                      const uint64_t delta, uint64_t *value)                   
```

- key, key_length: b+tree item의 key
- bkey 또는 bkey, bkey_length: element의 bkey
- delta: 증감시킬 값 (감소시킬 값이 element 값보다 크면, 0으로 설정한다.)
- value: 증감후의 element 값을 반환받는 인자

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: B+tree에서 정상적으로 element를 increment/decrement 후 조회하였음.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 B+tree가 없음.
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 bkey 에 해당하는 element가 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.
  - MEMCACHED_UNREADABLE: 주어진 key에 해당하는 B+tree가 unreadable 상태임.
  - MEMCACHED_OUT_OF_RANGE : 주어진 조회 범위에 해당하는 element가 없으나, 조회 범위가 overflow 정책에 의해
                             삭제되는 영역에 걸쳐 있음. 즉, B+tree 크기 제한으로 인해 삭제되어 조회되지 않은
                             element가 어딘가(DB)에 존재할 수도 있음을 뜻함.

B+tree element 값의 증감을 수행하는 예제는 아래와 같다.

``` c
void arcus_btree_element_incr(memcached_st *memc)
{
    uint64_t value; uint32_t flags= 10;
    int32_t exptime= 600;
    uint32_t maxcount= 1000;
    uint64_t value;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);
    memcached_return_t rc;

    memcached_coll_result_st result_obj;
    memcached_coll_result_st *result= memcached_coll_result_create(memc, &result_obj);

    // element 추가
    rc= memcached_bop_insert(memc, "btree:a_btree_incr", 19, 1, NULL, 0, "2", 1, &attributes);
    assert(rc == MEMCACHED_SUCCESS);

    // element increment
    rc= memcached_bop_incr(memc, "btree:a_btree_incr", 19, 1, 1, &value);
    assert(rc == MEMCACHED_SUCCESS);
    assert(uint64_t(3), value);

}

void arcus_btree_element_decr(memcached_st *memc)
{
    uint64_t value; uint32_t flags= 10;
    int32_t exptime= 600;
    uint32_t maxcount= 1000;
    uint64_t value;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);
    memcached_return_t rc;

    memcached_coll_result_st result_obj;
    memcached_coll_result_st *result= memcached_coll_result_create(memc, &result_obj);

    // element 추가
    rc= memcached_bop_insert(memc, "btree:a_btree_incr", 19, 1, NULL, 0, "2", 1, &attributes);
    assert(rc == MEMCACHED_SUCCESS);

    // element decrement 1
    rc= memcached_bop_decr(memc, "btree:a_btree_incr", 19, 1, 1, &value);
    assert(rc == MEMCACHED_SUCCESS);
    assert(uint64_t(1), value);

    // element decrement 2
    rc= memcached_bop_decr(memc, "btree:a_btree_incr", 19, 1, 2, &value);
    assert(rc == MEMCACHED_SUCCESS);
    assert(uint64_t(0), value);

}
```

### B+Tree Element 개수 확인

B+tree element 개수를 확인하는 함수는 두 유형이 있다.

첫째, b+tree에서 특정 bkey를 가진 element에 대해 eflag filter 조건을 만족하는지를 확인하는 함수이다.

``` c
memcached_return_t memcached_bop_count(memcached_st *ptr, const char *key, size_t key_length,
                                       const uint64_t bkey,
                                       memcached_coll_eflag_filter_st *eflag_filter, size_t *count)

memcached_return_t memcached_bop_ext_count(memcached_st *ptr, const char *key, size_t key_length,
                                       const unsigned char *bkey, size_t bkey_length,
                                       memcached_coll_eflag_filter_st *eflag_filter, size_t *count)                    
```

둘째, b+tree에서 bkey range에 해당하는 element들 중 eflag filter 조건을 만족하는 element 개수를 확인하는 함수이다.

``` c
memcached_return_t memcached_bop_count_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                       const uint64_t from, const uint64_t to,
                                       memcached_coll_eflag_filter_st *eflag_filter, size_t *count)

memcached_return_t memcached_bop_ext_count_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                       const unsigned char *from, size_t from_length,
                                       const unsigned char *to, size_t to_length,
                                       memcached_coll_eflag_filter_st *eflag_filter, size_t *count)                    
```

- key, key_length: b+tree item의 key
- bkey 또는 \<from, to\>:  개수 계산할 element의 bkey 또는 bkey range
- eflag_filter: element의 eflag에 대한 filter 조건
- count: element 개수가 반환되는 인자


Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_COUNT: 주어진 key에 해당하는 B+tree의 element 개수를 성공적으로 조회함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 B+tree가 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.

B+tree element 개수를 확인하는 예제는 아래와 같다.

``` c
void arcus_btree_element_count(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    // 테스트 데이터를 입력한다.
    rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"), 0, NULL, 0,
            "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));

    rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"), 1, NULL, 0,
            "value", strlen("value"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_STORED == memcached_get_last_response_code(memc));

    uint32_t bkey_from = 0;
    uint32_t bkey_to = 10000;

    memcached_coll_query_st query;
    memcached_bop_count_range_query_create(memc, &query, bkey_from, bkey_to, NULL);

    // 범위 안의 element 개수를 요청한다.
    size_t count = 0;
    rc= memcached_bop_count(memc, "btree:a_btree", strlen("btree:a_btree"), &query, &count);
    assert(MEMCACHED_SUCCESS == rc);
    assert(2 == count);
}
```

### B+Tree Element 조회

B+tree element를 조회하는 함수는 세 유형이 있다.

첫째, b+tree에서 특정 bkey를 가진 element에 대해 eflag filter 조건을 만족하면 조회하는 함수이다.

``` c
memcached_return_t memcached_bop_get(memcached_st *ptr, const char *key, size_t key_length,
                                     const uint64_t bkey,
                                     memcached_coll_eflag_filter_st *eflag_filter,
                                     bool with_delete, bool drop_if_empty, memcached_coll_result_st *result)

memcached_return_t memcached_bop_ext_get(memcached_st *ptr, const char *key, size_t key_length,
                                     const unsigned char *bkey, size_t bkey_length,
                                     memcached_coll_eflag_filter_st *eflag_filter,
                                     bool with_delete, bool drop_if_empty, memcached_coll_result_st *result)
```

둘째, b+tree에서 bkey range에 해당하는 element들을 스캔하면서 
eflag filter 조건을 만족하는 element들 중 offset 개를 skip한 후 count 개의 element를 조회하는 함수이다.

``` c
memcached_return_t memcached_bop_get_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                     const uint64_t from, const uint64_t to,
                                     memcached_coll_eflag_filter_st *eflag_filter,
                                     const size_t offset, const size_t count,
                                     bool with_delete, bool drop_if_empty, memcached_coll_result_st *result)

memcached_return_t memcached_bop_ext_get_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                     const unsigned char *from, size_t from_length,
                                     const unsigned char *to, size_t to_length,
                                     memcached_coll_eflag_filter_st *eflag_filter,
                                     const size_t offset, const size_t count,
                                     bool with_delete, bool drop_if_empty, memcached_coll_result_st *result)
```

셋째, qeury 구조체를 이용하여 b+tree element를 조회하는 함수이다.

``` c
memcached_return_t memcached_bop_get_by_query(memcached_st *ptr, const char *key, size_t key_length,
                                     memcached_bop_query_st *query
                                     bool with_delete, bool drop_if_empty, memcached_coll_result_st *result)
```

- key, key_length: b+tree item의 key
- bkey 또는 \<from, to\>: 조회할 bkey 또는 bkey range
- query: 조회를 명시한 query 구조체 즉, bkey range, eflag filter, offset, count가 명시된다.
- eflag_filter: element의 eflag에 대한 filter 조건
- with_delete: 조회와 함께 삭제도 수행할 것인지를 지정
- drop_if_empty: element 삭제로 empty b+tree가 될 경우, 그 b+tree도 삭제할 것인지를 지정

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: B+tree에서 정상적으로 element를 조회하였음.
  - MEMCACHED_TRIMMED
    - B+tree에서 정상적으로 element를 조회하였으나, 조회 범위가 overflow 정책에 의해 삭제되는 영역에 걸쳐 있음.
    - 즉, B+tree 크기 제한으로 인해 삭제되어 조회되지 않은 element가 어딘가(DB)에 존재할 수도 있음을 뜻함.
  - MEMCACHED_DELETED: B+tree에서 정상적으로 element를 조회하였으며, 동시에 이들을 삭제하였음.
  - MEMCACHED_DELETED_DROPPED: B+tree에서 정상적으로 element를 조회하였으며, 동시에 이들을 삭제하였음.
                               이 결과 empty 상태가 된 B+tree를 삭제함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 B+tree가 없음.
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 bkey또는 bkey 범위에 해당하는 element가 없음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.
  - MEMCACHED_UNREADABLE: 주어진 key에 해당하는 B+tree가 unreadable 상태임.
  - MEMCACHED_OUT_OF_RANGE
    - 주어진 조회 범위에 해당하는 element가 없으나, 조회 범위가 overflow 정책에 의해 삭제되는 영역에 걸쳐 있음.
    - 즉, B+tree 크기 제한으로 인해 삭제되어 조회되지 않은 element가 어딘가(DB)에 존재할 수도 있음을 뜻함.

조회 결과는 memcached_coll_result_t 구조체에 저장된다.
조회 결과에 접근하기 위한 API는 다음과 같다.


``` c
memcached_coll_result_st *memcached_coll_result_create(const memcached_st *ptr, memcached_coll_result_st *result)
void                      memcached_coll_result_free(memcached_coll_result_st *result)

memcached_coll_type_t     memcached_coll_result_get_type(memcached_coll_result_st *result)
size_t                    memcached_coll_result_get_count(memcached_coll_result_st *result)
uint32_t                  memcached_coll_result_get_flags(memcached_coll_result_st *result)
const char *              memcached_coll_result_get_value(memcached_coll_result_st *result, size_t index)
size_t                    memcached_coll_result_get_value_length(memcached_coll_result_st *result, size_t index)
```

B+tree element를 조회하는 예제는 아래와 같다.

``` c
void arcus_btree_element_get(memcached_st *memc)
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
        rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"), i, NULL, 0,
                                 buffer, buffer_len, &attributes);
        assert(MEMCACHED_SUCCESS == rc);
    }

    // 조회 범위에 아무런 element가 없는 경우
    result = memcached_coll_result_create(memc, NULL);

    rc= memcached_bop_get_by_range(memc, "btree:a_btree", strlen("btree:a_btree"), maxcount, maxcount+maxcount,
                                   NULL, 0, maxcount, false, false, result);
    assert(MEMCACHED_NOTFOUND_ELEMENT == rc);

    memcached_coll_result_free(result);

    // 조회와 동시에 조회된 element를 삭제한다. Empty 상태가 된 B+tree는 삭제된다.
    result = memcached_coll_result_create(memc, NULL);

    rc= memcached_bop_get_by_range(memc, "btree:a_btree", strlen("btree:a_btree"), 0, maxcount,
                                   NULL, 0, maxcount, true, false, result);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_DELETED_DROPPED == memcached_get_last_response_code(memc));

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

### B+Tree Element 일괄 삽입

B+tree에 여러 element를 한번에 삽입하는 함수는 두 유형이 있다.

첫째, 하나의 key가 가리키는 b+tree에 다수의 element를 삽입하는 함수이다.
전자는 8바이트 unsigned interger 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

``` c
memcached_return_t memcached_bop_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                       const size_t number_of_piped_items,
                                       const uint64_t *bkeys,
                                       const unsigned char * const *eflags, const size_t *eflags_length,
                                       const char * const *values, const size_t *values_length,
                                       memcached_coll_create_attrs_st *attributes,
                                       memcached_return_t *results, memcached_return_t *piped_rc)

memcached_return_t memcached_bop_ext_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                       const size_t number_of_piped_items,
                                       const unsigned char * const *bkeys, const size_t *bkeys_length,
                                       const unsigned char * const *eflags, const size_t *eflags_length,
                                       const char * const *values, const size_t *values_length,
                                       memcached_coll_create_attrs_st *attributes,
                                       memcached_return_t *results, memcached_return_t *piped_rc)     
```

- key, key_length: b+tree item의 key
- number_of_piped_items: 한번에 삽입할 element 개수
- bkeys 또는 bkeys, bkeys_length: element 개수만큼의 bkey array (필수)
- eflags, eflags_length: element 개수만큼의 eflag array (옵션)
- values, values_length: element 개수만큼의 value array (필수)
- attributes: B+tree 없을 시에 attributes에 따라 empty b+tree를 생성 후에 element 삽입한다.

둘째, 여러 key들이 가리키는 b+tree들에 각각 하나의 element를 삽입하는 함수이다. 
전자는 8바이트 unsigned interger 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

``` c
memcached_return_t memcached_bop_piped_insert_bulk(memcached_st *ptr,
                                       const char * const *keys, const size_t *keylengths,
                                       size_t number_of_keys,
                                       const uint64_t bkey,
                                       const unsigned char *eflag, size_t eflag_length,
                                       const char *value, size_t value_length,
                                       memcached_coll_create_attrs_st *attributes,
                                       memcached_return_t *results, memcached_return_t *piped_rc)

memcached_return_t memcached_bop_ext_piped_insert_bulk(memcached_st *ptr,
                                       const char * const *keys, const size_t *keylengths,
                                       size_t number_of_keys,
                                       const unsigned char *bkey, size_t bkey_length,
                                       const unsigned char *eflag, size_t eflag_length,
                                       const char *value, size_t value_length,
                                       memcached_coll_create_attrs_st *attributes,
                                       memcached_return_t *results, memcached_return_t *piped_rc)
```

- keys, keys_length: 다수 b+tree items의 key array
- number_of_keys: key들의 개수
- bkey, bkey_length: element의 bkey(b+tree key)
- eflag, eflag_length: element의 eflag(element flag) (옵션)
- value, value_length: element의 value
- attributes: B+tree 없을 시에 attributes에 따라 empty b+tree를 생성 후에 element 삽입한다.

B+tree element 일괄 삽입의 결과는 아래의 인자를 통해 받는다.

- results: 일괄 삽입 결과가 주어진 key 또는 (key, element) 순서대로 저장된다.
  - MEMCACHED_STORED: element만 삽입
  - MEMCACHED_CRATED_STORED: b+tree item 생성 후에, element 삽입
- piped_rc: 일괄 저장의 전체 결과를 담고 있다
  - MEMCACHED_ALL_SUCCESS: 모든 element가 저장됨.
  - MEMCACHED_SOME_SUCCESS: 일부 element가 저장됨.
  - MEMCACHED_ALL_FAILURE: 전체 element가 저장되지 않음.

B+tree element 일괄 삽입의 예제는 아래와 같다.

``` c
void arcus_btree_element_piped_insert(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 500;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;
    memcached_return_t piped_rc;
    memcached_return_t results[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE];

    uint64_t bkeys[100];
    unsigned char **eflags = (unsigned char **)malloc(sizeof(unsigned char *) * 100);
    size_t elfaglengths[100];
    char **values = (char **)malloc(sizeof(char *) * 100);

    uint32_t elfag = 0;
    int i;

    // pipe 연산에 필요한 argument를 생성한다.
    for (i=0; i<100; i++)
    {
        bkeys[i] = i;
        eflags[i] = (unsigned char *)&eflag;
        eflaglengts[i] = sizeof(eflag);
        values[i] = (char *)malloc(sizeof(char)*15);
        valuelengths[i] = snprintf(values[i], 15, "value%llu", (unsigned long long)i);
    }

    // 입력할 때 B+Tree가 존재하지 않으면 새로 생성한 뒤 입력한다.
    rc= memcached_bop_piped_insert(memc, "btree:a_btree", strlen("btree:a_btree"),
                                   100, bkeys, eflags, eflaglengths, values, valuelengths,
                                   &attributes, results, &piped_rc);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_ALL_SUCCESS == piped_rc);
    for (i=0; i<100; i++)
    {
        assert(MEMCACHED_STORED == results[i] || MEMCACHED_CREATED_STORED == results[i]);
    }
    for (i=0; i<100; i++)
    {
        free((void*)values[i]);
    }
    free((void*)eflags);
    free((void*)values);
}
```

### B+tree Element 일괄 조회

서로 다른 key로 분산되어 있는 b+tree들의 element들을 한 번의 요청으로 조회할 수 있는 기능이다.
이 기능은 비동기(asynchronous) 방식으로 수행하며,
(1) 다수 b+tree들의 element 조회 요청을 보내는 단계와 
(2) 조회 결과를 받아내는 단계로 구분된다.

첫째 단계로, 다수 b+tree들에 대한 element 조회 요청을 보내는 함수는 아래와 같다.
B+tree element 조회 조건는 query 구조체를 이용하여 명시한다.

``` c
memcached_return_t memcached_bop_mget(memcached_st *ptr, const char * const *keys, const size_t *keys_length,
                                      size_t number_of_keys, memcached_coll_query_st *query)
```

- keys, keys_length: 다수 bt+tree item들의 key array
- number_of_keys: key 개수
- qeury: 조회 조건을 가진 query 구조체

Response code는 아래와 같다.

- MEMCACHED_SUCCESS : 각 key를 담당하는 모든 캐시 서버에 성공적으로 조회 요청을 보냄.
- MEMCACHED_SOME_SUCCESS : 각 key를 담당하는 캐시 서버로의 요청이 일부 실패함.
- MEMCACHED_FAILURE : 각 key를 담당하는 모든 캐시 서버로의 요청이 모두 실패함.

둘째 단계로, element 조회 결과를 iteration 방식으로 하나씩 가져오기 위한 함수는 아래와 같다.

``` c
memcached_coll_result_st *memcached_coll_fetch_result(memcached_st *ptr, memcached_coll_result_st *result,
                                                      memcached_return_t *error)
```

조회 결과는 다음과 같다.

- result != null
  - MEMCACHED_SUCCESS: 정상적으로 element를 조회함.
  - MEMCACHED_TRIMMED: 정상적으로 element를 조회하였으나, 조회 범위가 특정 B+tree의 overflow 정책에 의해
                       삭제되는 영역에 걸쳐 있음. 즉, 해당 B+tree 크기 제한으로 인해 삭제되어
                       조회되지 않은 element가 존재할 수 있음.
- result == null
  - MEMCACHED_NOT_FOUND: 주어진 key를 찾을 수 없음.
  - MEMCACHED_NOT_FOUND_ELEMENT: 조회 조건에 해당하는 element를 찾을 수 없음.
  - MEMCACHED_OUT_OF_RANGE: 주어진 조회 조건에 해당하는 element가 없으나,
                            조회 범위가 overflow 정책에 의해 삭제되는 영역에 걸쳐 있음.
                            즉, 해당 B+tree 크기 제한으로 인해 삭제되어 조회되지 않은 element가 존재할 수 있음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.


B+tree element 일괄 조회하는 예제는 아래와 같다.

``` c
static void arcus_btree_element_mget(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 50;

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    memcached_return_t rc;

    // test data
    const char *keys[]= { "btree:a_btree1", "btree:a_btree2", "btree:a_btree3", "btree:a_btree4", "btree:a_btree5" };
    size_t key_length[] = { 14, 14, 14, 14, 14 };

    for (size_t i=0; i<3; i++)
    {
        for (size_t j=0; j<maxcount; j++)
        {
            char buffer[32];
            size_t buffer_len= snprintf(buffer, 32, "value%lu", (unsigned long)j);
            rc= memcached_bop_insert(memc, keys[i], key_length[i], j, NULL, 1, buffer, buffer_len, &attributes);
        }
    }

    // query
    memcached_bop_query_st query_obj;
    memcached_bop_range_query_init(&query_obj, 0, 10000, NULL, 0, maxcount);

    rc= memcached_bop_mget(memc, keys, key_length, 5, &query_obj);
    // result
    memcached_coll_result_st result_obj;
    memcached_coll_result_st *result= memcached_coll_result_create(memc, &result_obj);

    while ((result= memcached_coll_fetch_result(memc, &result_obj, &rc)))
    {
        if (rc == MEMCACHED_SUCCESS or
                rc == MEMCACHED_TRIMMED )
        {
            for (size_t i=0; i<memcached_coll_result_get_count(result); i++)
            {
                fprintf(stderr, "[debug] %04lu : %s[%llu]=%s (rc=%s)\n",
                        (unsigned long)i, memcached_coll_result_get_key(result),
                        memcached_coll_result_get_bkey(result, i),
                        memcached_coll_result_get_value(result, i),
                        memcached_strerror(NULL, rc));
            }
        }
        else
        {
        }
        memcached_coll_result_free(result);
    }
}
```

### B+tree Element Sort-Merge 조회

서로 다른 key로 분산되어 있는 b+Tree들의 element를 sort-merge 방식으로 조회하는 기능이다.
이는 서로 다른 b+tree들이지만, 논리적으로 하나로 합쳐진 거대한 b+tree에 대해 element 조회 연산하는 것과
동일한 효과를 낸다.

smget 동작은 조회 범위와 어떤 b+tree의 trim 영역과 겹침에 대한 처리로,
아래 두 가지 동작 모드가 있다.

1) 기존 Sort-Merge 조회 (1.8.X 이하 버전에서 동작하던 방식)
   - smget 조회 조건을 만족하는 첫 번째 element가 trim된 b+tree가 하나라도 존재하면 OUT_OF_RANGE 응답을 보낸다.
     이 경우, 응용은 모든 key에 대해 백엔드 저장소인 DB에서 elements 조회한 후에
     응용에서 sort-merge 작업을 수행하여야 한다.
   - OUT_OF_RANGE가 없는 상황에서 smget을 수행하면서
     조회 조건을 만족하는 두 번째 이후의 element가 trim된 b+tree를 만나게 되면,
     그 지점까지 조회한 elements를 최종 elements 결과로 하고
     smget 수행 상태는 TRIMMED로 하여 응답을 보낸다.
     이 경우, 응용은 모든 key에 대해 백엔드 저장소인 DB에서 trim 영역의 elements를 조회하여
     smget 결과에 반영하여야 한다.

2) 신규 Sort-Merge 조회 (1.9.0 이후 버전에서 추가된 방식)
   - 기존의 OUT_OF_RANGE에 해당하는 b+tree를 missed keys로 분류하고
     나머지 b+tree들에 대해 smget을 계속 수행한다.
     따라서, 응용에서는 missed keys에 한해서만
     백엔드 저장소인 DB에서 elements를 조회하여 최종 smget 결과에 반영할 수 있다.
   - smget 조회 조건을 만족하는 두 번째 이후의 element가 trim된 b+tree가 존재하더라도,
     그 지점에서 smget을 중지하는 것이 아니라, 그러한 b+tree를 trimmed keys로 분류하고
     원하는 개수의 elements를 찾을 때까지 smget을 계속 진행한다.
     따라서, 응용에서는 trimmed keys에 한하여
     백엔드 저장소인 DB에서 trim된 elements를 조회하여 최종 smget 결과에 반영할 수 있다.
   - bkey에 대한 unique 조회 기능을 지원한다.
     중복 bkey를 허용하여 조회하는 duplicate 조회 외에
     중복 bkey를 제거하고 unique bkey만을 조회하는 unique 조회를 지원한다.
   - 조회 조건에 offset 기능을 제거한다.

기존 smget 연산을 사용하더라도, offset 값은 항상 0으로 사용하길 권고한다.
양수의 offset을 사용하는 smget에서 missed keys가 존재하고
missed keys에 대한 DB 조회가 offset으로 skip된 element를 가지는 경우,
응용에서 정확한 offset 처리가 불가능해지기 때문이다.
이전의 조회 결과에 이어서 추가로 조회하고자 하는 경우,
이전에 조회된 bkey 값을 바탕으로 bkey range를 재조정하여 사용할 수 있다.

Sort-Merge 조회를 수행하는 함수는 아래와 같다.

``` c
memcached_return_t memcached_bop_smget(memcached_st *ptr, const char * const *keys, const size_t *keys_length,
                                       size_t num_of_keys, memcached_bop_query_st *query,
                                       memcached_bop_smget_result_st *result)
```
- keys, keys_length: 다수 bt+tree item들의 key array
- number_of_keys: key 개수
- query: 조회 조건을 가진 query 구조체
- result: sort-merge 조회 결과는 담는 구조체

Sort-Merge 조회 질의를 표현하는 memcached_bop_query_st 구조체 생성 방법은
기존 sort-merge 조회와 신규 sort-merge 조회에 따라 다르다.
기존 sort-merge 조회에서 memcached_bop_query_st 구조체 생성 방법은
앞서 설명한 [B+Tree Query 구조체](07-btree-API.md#btree-query-구조체) 참고하기 바란다.
신규 sort-merge 조회에서는 아래의 sort-merge 질의 생성하는 전용 API를 사용해
bkey range, element flag, count 그리고 unique를 명시하여 query 구조체를 생성한다.
마지막 인자인 unique가 false이면 중복 bkey를 허용하여 조회하며,
true이면 중복 bkey를 제거하여 unique bkey만을 조회한다. 

``` c
memcached_return_t memcached_bop_smget_query_init(memcached_bop_query_st *ptr,
                                                  const uint64_t bkey_from, const uint64_t bkey_to,
                                                  memcached_coll_eflag_filter_st *eflag_filter,
                                                  size_t count, bool unique);
memcached_return_t memcached_bop_ext_smget_query_init(memcached_bop_query_st *ptr,
                                                  const unsigned char *bkey_from, size_t bkey_from_length,
                                                  const unsigned char *bkey_to, size_t bkey_to_length,
                                                  memcached_coll_eflag_filter_st *eflag_filter,
                                                  size_t count, bool unique);
```


Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: 여러 B+tree에서 정상적으로 element를 조회하였음.
  - MEMCACHED_DUPLICATED: 여러 B+tree에서 정상적으로 element를 조회하였으나 중복된 bkey가 존재함.
  - MEMCACHED_TRIMMED (기존 sort-merge 조회에 한정)
    - 정상적으로 element를 조회하였으나, 조회 범위가 특정 B+tree의 overflow 정책에 의해 삭제되는 영역에 걸쳐 있음.
    - 즉, 해당 B+tree 크기 제한으로 인해 삭제되어 조회되지 않은 element가 어딘가(DB)에 존재할 수도 있음을 뜻함.
  - MEMCACHED_DUPLICATED_TRIMMED (기존 sort-merge 조회에 한정)
    - MEMCACHED_DUPLICATED 상태와MEMCACHED_TRIMMED 상태가 모두 존재.
- not MEMCACHED_SUCCESS
  - MEMCACHED_OUT_OF_RANGE (기존 sort-merge 조회에 한정)
    - 주어진 조회 범위에 해당하는 element가 없으나, 조회 범위가 overflow 정책에 의해 삭제되는 영역에 걸쳐 있음.
    - 즉, B+tree 크기 제한으로 인해 삭제되어 조회되지 않은 element가 어딘가(DB)에 존재할 수도 있음을 뜻함.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.
  - MEMCACHED_ATTR_MISMATCH: smget에 참여하는 B+tree들의 attribute가 서로 다름.
    - 참고로 smget에 참여하는 B+tree들은 maxcount, maxbkeyrange, overflowaction이 모두 동일해야 함.

Sort-merge 조회 결과는 memcached_bop_smget_result_t 구조체로 받아온다.
Sort-merge 조회하기 전에 memcached_bop_smget_result_t 구조체를 초기화하여 사용해야 하고,
memcached_bop_smget_result_t 구조체에서 조회 결과를 모두 얻은 후에는 다시 반환하여야 한다.
이를 수행하는 두 함수는 아래와 같다.

``` c
memcached_bop_smget_result_st *memcached_bop_smget_result_create(const memcached_st *ptr, memcached_bop_smget_result_st *result)
void                           memcached_bop_smget_result_free(memcached_bop_smget_result_st *result)
```

함수                              | 기능
--------------------------------- | ---------------------------------------------------------------
memcached_bop_smget_result_create | result 구조체를 초기화한다. result에 NULL을 넣으면 새로 allocate 하여 반환한다.
memcached_bop_smget_result_free   | result 구조체를 초기화하고 allocate 된 경우 free 한다.


memcached_bop_smget_result_t 구조체에서 조회 결과를 얻기 위한 함수들은 아래와 같다.

``` c
size_t                    memcached_bop_smget_result_get_count(memcached_bop_smget_result_st *result)
const char *              memcached_bop_smget_result_get_key(memcached_bop_smget_result_st *result, size_t index)
uint64_t                  memcached_bop_smget_result_get_bkey(memcached_bop_smget_result_st *result, size_t index)
memcached_hexadecimal_st *memcached_bop_smget_result_get_bkey_ext(memcached_bop_smget_result_st *result, size_t index)
memcached_hexadecimal_st *memcached_bop_smget_result_get_eflag(memcached_bop_smget_result_st *result, size_t index)
const char *              memcached_bop_smget_result_get_value(memcached_bop_smget_result_st *result, size_t index)
size_t                    memcached_bop_smget_result_get_value_length(memcached_bop_smget_result_st *result, size_t index)
size_t                    memcached_bop_smget_result_get_missed_key_count(memcached_bop_smget_result_st *result)
const char *              memcached_bop_smget_result_get_missed_key(memcached_bop_smget_result_st *result, size_t index)
size_t                    memcached_bop_smget_result_get_missed_key_length(memcached_bop_smget_result_st *result, size_t index)
memcached_return_t        memcached_coll_smget_result_get_missed_cause(memcached_coll_smget_result_st *result, size_t index)
size_t                    memcached_coll_smget_result_get_trimmed_key_count(memcached_coll_smget_result_st *result)
const char *              memcached_coll_smget_result_get_trimmed_key(memcached_coll_smget_result_st *result, size_t index)
size_t                    memcached_coll_smget_result_get_trimmed_key_length(memcached_coll_smget_result_st *result, size_t index)
uint64_t                  memcached_coll_smget_result_get_trimmed_bkey(memcached_coll_smget_result_st *result, size_t index)
memcached_hexadecimal_st *memcached_coll_smget_result_get_trimmed_bkey_ext(memcached_coll_smget_result_st *result, size_t index)
```

함수                                              | 기능
------------------------------------------------- | ---------------------------------------------------------------
memcached_bop_smget_result_get_count              | 조회 결과의 element 개수
memcached_bop_smget_result_get_key                | Element array에서 index 위치에 있는 element의 key
memcached_bop_smget_result_get_bkey               | Element array에서 index 위치에 있는 element의 unsigned int bkey
memcached_bop_smget_result_get_bkey_ext           | Element array에서 index 위치에 있는 element의 byte array bkey
memcached_bop_smget_result_get_eflag              | Element array에서 index 위치에 있는 element의 eflag
memcached_bop_smget_result_get_value              | Element array에서 index 위치에 있는 element의 value
memcached_bop_smget_result_get_value_length       | Element array에서 index 위치에 있는 element의 value 길이
memcached_bop_smget_result_get_missed_key_count   | 조회 결과의 missed key 개수
memcached_bop_smget_result_get_missed_key         | Missed key array에서 index 위치에 있는 key
memcached_bop_smget_result_get_missed_key_length  | Missed key array에서 index 위치에 있는 key 길이
memcached_bop_smget_result_get_missed_cause       | 해당 missed key가 miss된 원인 (신규 sort-merge 조회 한정)
memcached_bop_smget_result_get_trimmed_key_count  | 조회 결과의 trimmed key 개수 (신규 sort-merge 조회 한정)
memcached_bop_smget_result_get_trimmed_key_key    | Trimmed key array에서 index 위치에 있는 key (신규 sort-merge 조회 한정)
memcached_bop_smget_result_get_trimmed_key_length | Trimmed key array에서 index 위치에 있는 key 길이 (신규 sort-merge 조회 한정)
memcached_bop_smget_result_get_trimmed_bkey       | 해당 trimmed key의 마지막 unsigned int bkey (신규 sort-merge 조회 한정)
memcached_bop_smget_result_get_trimmed_bkey_ext   | 해당 trimmed key의 마지막 byte array bkey (신규 sort-merge 조회 한정)

B+tree element sort-merge 조회하는 예제는 아래와 같다.

``` c
void arcus_btree_element_smget(memcached_st *memc)
{
    memcached_return_t rc;
    const char *keys[100];
    size_t key_length[100];
    uint32_t bkeys[100];

    srand(time(NULL));

    // 100개의 key와 무작위로 생성한 bkey를 준비한다.
    for (int i=0; i<100; i++) {
        keys[i] = (char *)malloc(255);
        key_length[i] = (size_t) snprintf((char *)keys[i], 255, "test:ext_ascending_order_id_%d", i);

        // Integer -> Hexadecimal 변환을 Big-endian에 맞추기로 하자.
        bkeys[i] = htonl(rand());
    }

    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, 0, 600, 1000);

    // 테스트 데이터를 입력한다.
    for (int i=0; i<100; i++) {
        uint32_t bkey = bkeys[i];
        uint32_t eflag = 0;
        char value[64];
        size_t value_length = snprintf(value, 64, "value_id%d_bkey%u", i, bkey);

        // 여기서는 bkey와 eflag를 uint32_t 형태의 정수(big-endian)를 캐스팅하여 넘겨 주고 있음을 참고하자.
        rc = memcached_bop_ext_insert(memc, keys[i], key_length[i], (unsigned char *)&bkey, sizeof(uint32_t),
                                      (unsigned char *)&eflag, sizeof(eflag), value, value_length, &attributes);
    }

    memcached_bop_smget_result_st smget_result_object;
    memcached_bop_smget_result_st *smget_result = memcached_bop_smget_result_create(memc, &smget_result_object);

    uint32_t bkey_from = 0;
    uint32_t bkey_to = htonl(UINT32_MAX);

    // byte array bkey에 대해 중복 bkey 허용한 범위 검색 쿼리를 생성한다.
    memcached_bop_query_st query;
    memcached_bop_ext_smget_query_init(memc, &query, (unsigned char *)&bkey_from, sizeof(uint32_t),
                                       (unsigned char *)&bkey_to, sizeof(uint32_t), NULL, 100, false);

    // smget을 수행한다.
    rc = memcached_bop_smget(memc, keys, key_length, 100, &query, smget_result);
    assert(MEMCACHED_END == memcached_get_last_response_code(memc));
    aseert(100 == memcached_bop_smget_result_get_count(smget_result));
    aseert(0 == memcached_bop_smget_result_get_missed_key_count(smget_result));
    aseert(0 == memcached_bop_smget_result_get_trimmed_key_count(smget_result));

    if (rc == MEMCACHED_SUCCESS) {
        uint32_t last_bkey = bkey_from;
        for (uint32_t i=0; i<memcached_bop_smget_result_get_count(smget_result); i++) {
            memcached_hexadecimal_st bkey = smget_result->sub_keys[i].bkey_ext;
            char bkey_buf[64];
            memcached_hexadecimal_to_str(&bkey, bkey_buf, 64);
            char eflag_buf[64];
            memcached_hexadecimal_to_str(&smget_result->eflags[i], eflag_buf, 64);
            char *rvalue = smget_result->values[i].string;
            fprintf(stderr, "key[%s], bkey[%s], eflag[%s] = %s\n",
                             smget_result->keys[i].string, bkey_buf, eflag_buf, rvalue);
        }
    } else {
        fprintf(stderr, "memcached_bop_smget() failed, reason=%s\n", memcached_strerror(NULL, rc));
        return;
    }

    memcached_bop_smget_result_free(smget_result);

    for (int i=0; i<100; i++) {
        free((void*)keys[i]);
    }
}
```

### B+Tree Element 순위 조회

B+Tree element 순위를 조회하는 함수는 아래와 같다.
전자는 8바이트 unsigned integer 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

```c
memcached_return_t memcached_bop_find_position(memcached_st *ptr, const char *key, size_t key_length,
                                               const uint64_t bkey,
                                               memcached_coll_order_t order,
                                               size_t *position)
                                     
memcached_return_t memcached_bop_ext_find_position(memcached_st *ptr, const char *key, size_t key_length,
                                                   const unsigned char *bkey, size_t bkey_length,
                                                   memcached_coll_order_t order,
                                                   size_t *position)
```
- key, key_length: B+Tree item의 key
- bkey, bkey_length: 순위를 조회할 element의 bkey
- order : 순위 기준
  - MEMCACHED_COLL_ORDER_ASC: bkey 값의 오름차순
  - MEMCACHED_COLL_ORDER_DESC: bkey 값의 내림차순
- position: element 순위가 반환되는 인자

Response code는 아래와 같다.
- MEMCACHED_SUCCESS
  - MEMCACHED_SUCCESS: 주어진 key에서 bkey에 해당하는 B+Tree element 순위를 성공적으로 조회함
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 B+Tree item이 없음
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 bkey에 해당하는 B+Tree element가 없음
  - MEMCACHED_TYPE_MISMATCH: 해당 item이 B+Tree가 아님
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형이 기존 bkey 유형과 다름
  - MEMCACHED_UNREADABLE: 해당 key item이 unreadable 상태임
  - MEMCACHED_NOT_SUPPORTED: 현재 순위 조회 연산이 지원되지 않음.

B+Tree element 순위를 조회하는 예제는 아래와 같다.

```c
void arcus_btree_find_position(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;

    memcached_return_t rc;
    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    // 테스트 데이터를 입력한다.
    rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"), 0, NULL, 0,
            "value0", strlen("value0"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_CREATED_STORED == memcached_get_last_response_code(memc));

    rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"), 1, NULL, 0,
            "value1", strlen("value1"), &attributes);
    assert(MEMCACHED_SUCCESS == rc);
    assert(MEMCACHED_STORED == memcached_get_last_response_code(memc));

    // 순위를 조회한다.
    int position = -1;
    rc = memcached_bop_find_position(memc, "btree:a_btree", strlen("btree:a_btree"), 1,
                                     MEMCACHED_COLL_ORDER_ASC, &position);
    assert(1 == position);
    rc = memcached_bop_find_position(memc, "btree:a_btree", strlen("btree:a_btree"), 1,
                                     MEMCACHED_COLL_ORDER_DESC, &position);
    assert(0 == position);
}
```

### B+Tree 순위 기반의 Element 조회

B+Tree에서 순위 범위로 element를 조회하는 함수는 아래와 같다.

```c
memcached_return_t memcached_bop_get_by_position(memcached_st *ptr,
                                                 const char *key, size_t key_length,
                                                 memcached_coll_order_t order,
                                                 size_t from_position, size_t to_position,
                                                 memcached_coll_result_st *result);
```
- key, key_length: B+Tree item의 key
- order : 순위 기준
  - MEMCACHED_COLL_ORDER_ASC: bkey 값의 오름차순
  - MEMCACHED_COLL_ORDER_DESC: bkey 값의 내림차순
- from_position, to_position: 순위의 범위로 시작 순위와 끝 순위
- result: 조회 결과를 담는 result 구조체

Response code는 아래와 같다.
- MEMCACHED_SUCCESS
  - MEMCACHED_SUCCESS: 주어진 순위 기준과 순위 범위에 해당하는 element들을 성공적으로 조회함
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 B+Tree item이 없음
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 순위 범위에 해당하는 B+Tree element가 없음
  - MEMCACHED_TYPE_MISMATCH: 해당 item이 B+Tree가 아님
  - MEMCACHED_UNREADABLE: 해당 key item이 unreadable 상태임
  - MEMCACHED_NOT_SUPPORTED: 현재 순위 조회 연산이 지원되지 않음.

B+Tree에서 순위 범위로 element를 조회하는 예제는 아래와 같다.

```c
void arcus_btree_get_by_position(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;
    char buffer[16];
    size_t buffer_len;

    memcached_return_t rc;
    memcached_coll_result_st *result;
    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    // bkey 범위가 0 ~ 999인 1000개 element를 가진 b+tree 생성.
    for (uint32_t bkey=0; bkey<maxcount; bkey++)
    {
      buffer_len= snprintf(buffer, 15, "value%u", bkey);
      rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"),
                               bkey, NULL, 0, buffer, buffer_len, &attributes);
      assert(MEMCACHED_SUCCESS == rc);
    }

    // 오름차순으로 100번째 element를 조회한다.
    result = memcached_coll_result_create(memc, NULL);
    rc= memcached_bop_get_by_position(memc, "btree:a_btree", strlen("btree:a_btree"),
                                      MEMCACHED_COLL_ORDER_ASC, 99, 99, result);
    assert(MEMCACHED_SUCCESS == rc);
    assert(1 == memcached_coll_result_get_count(result));
    for (uint32_t i= 0; i < memcached_coll_result_get_count(result); i++) {
      snprintf(buffer, 15, "value%u", (99+i));
      assert(0 == strcmp(buffer, memcached_coll_result_get_value(result, i)));
    }
    memcached_coll_result_free(result);

    // 내림차순으로 100번째 element를 조회한다.
    result = memcached_coll_result_create(memc, NULL);
    rc= memcached_bop_get_by_position(memc, "btree:a_btree", strlen("btree:a_btree"),
                                      MEMCACHED_COLL_ORDER_DESC, 99, 99, result);
    assert(MEMCACHED_SUCCESS == rc);
    assert(1 == memcached_coll_result_get_count(result));
    for (uint32_t i= 0; i < memcached_coll_result_get_count(result); i++) {
      snprintf(buffer, 15, "value%u", ((maxcount-1)-99-i));
      assert(0 == strcmp(buffer, memcached_coll_result_get_value(result, i)));
    }
    memcached_coll_result_free(result);

    // 오름차순으로 101번째부터 200번째까지의 element들을 조회한다.
    result = memcached_coll_result_create(memc, NULL);
    rc= memcached_bop_get_by_position(memc, "btree:a_btree", strlen("btree:a_btree"),
                                      MEMCACHED_COLL_ORDER_ASC, 100, 199, result);
    assert(MEMCACHED_SUCCESS == rc);
    assert(100 == memcached_coll_result_get_count(result));
    for (uint32_t i= 0; i < memcached_coll_result_get_count(result); i++) {
      snprintf(buffer, 15, "value%u", (100+i));
      assert(0 == strcmp(buffer, memcached_coll_result_get_value(result, i)));
    }
    memcached_coll_result_free(result);

    // 내림차순으로 101번째부터 200번째까지의 element들을 조회한다.
    result = memcached_coll_result_create(memc, NULL);
    rc= memcached_bop_get_by_position(memc, "btree:a_btree", strlen("btree:a_btree"),
                                      MEMCACHED_COLL_ORDER_DESC, 100, 199, result);
    assert(MEMCACHED_SUCCESS == rc);
    assert(100 == memcached_coll_result_get_count(result));
    for (uint32_t i= 0; i < memcached_coll_result_get_count(result); i++) {
      snprintf(buffer, 15, "value%u", ((maxcount-1)-100-i));
      assert(0 == strcmp(buffer, memcached_coll_result_get_value(result, i)));
    }
    memcached_coll_result_free(result);
}
```

### B+Tree 순위와 Element 동시 조회

B+Tree에서 주어진 bkey에 대한 순위를 조회하면서 그 bkey의 element를 포함하여
앞뒤 양방향으로 각 N개의 elements를 함께 조회하는 함수는 아래와 같다.
전자는 8바이트 unsigned integer 타입의 bkey를 사용하는 함수이고,
후자는 최대 31 크기의 byte array 타입의 bkey를 사용하는 함수이다.

```c
memcached_return_t memcached_bop_find_position_with_get(memcached_st *ptr,
                                                 const char *key, size_t key_length,
                                                 const uint64_t bkey,
                                                 memcached_coll_order_t order, size_t count,
                                                 memcached_coll_result_st *result);

memcached_return_t memcached_bop_ext_find_position_with_get(memcached_st *ptr,
                                                 const char *key, size_t key_length,
                                                 const unsigned char *bkey, size_t bkey_length,
                                                 memcached_coll_order_t order, size_t count,
                                                 memcached_coll_result_st *result);
```
- key, key_length: B+Tree item의 key
- bkey, bkey_length: 순위를 조회할 element의 bkey
- order : 순위 기준
  - MEMCACHED_COLL_ORDER_ASC: bkey 값의 오름차순
  - MEMCACHED_COLL_ORDER_DESC: bkey 값의 내림차순
- count : 찾은 element의 앞뒤 양방향으로 각각 조회할 element 개수
- result: 조회한 순위와 element를 담는 result 구조체

Response code는 아래와 같다.
- MEMCACHED_SUCCESS
  - MEMCACHED_SUCCESS: 순위 조회와 element 조회를 성공적으로 수행함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 B+Tree item이 없음
  - MEMCACHED_NOTFOUND_ELEMENT: 주어진 bkey에 해당하는 B+Tree element가 없음
  - MEMCACHED_TYPE_MISMATCH: 해당 item이 B+Tree가 아님
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형이 기존 bkey 유형과 다름
  - MEMCACHED_UNREADABLE: 해당 key item이 unreadable 상태임
  - MEMCACHED_NOT_SUPPORTED: 현재 순위 조회 연산이 지원되지 않음.

정상 수행되었을 경우, result 구조체는 아래의 결과를 가진다.
- 조회된 element 결과
  - bkey 값의 order 기준에 따라 정렬된 순서대로 element들이 보관된다.
- 각 element의 bkey 순위
  - memcached_coll_result_get_position(result, index) API를 통해 조회한다.
- btree 내에서 주어진 bkey의 순위
  - memcached_coll_result_get_btree_position(result) API를 통해 조회한다.
- result에 보관된 element들에서 주어진 bkey의 element 위치
  - memcached_coll_result_get_result_position(result) API를 통해 조회한다.


B+Tree에서 특정 bkey에 대한 순위 및 element 조회와 함께
앞뒤 양방향으로 N개 element를 조회하는 예제는 아래와 같다.

```c
void arcus_btree_find_position_with_get(memcached_st *memc)
{
    uint32_t flags= 10;
    uint32_t exptime= 600;
    uint32_t maxcount= 1000;
    char buffer[16];
    size_t buffer_len;

    memcached_return_t rc;
    memcached_coll_result_st *result;
    memcached_coll_create_attrs_st attributes;
    memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

    // bkey 범위가 0 ~ 999인 1000개 element를 가진 b+tree 생성.
    for (uint32_t bkey=0; bkey<maxcount; bkey++)
    {
      buffer_len= snprintf(buffer, 15, "value%u", bkey);
      rc= memcached_bop_insert(memc, "btree:a_btree", strlen("btree:a_btree"),
                               bkey, NULL, 0, buffer, buffer_len, &attributes);
      assert(MEMCACHED_SUCCESS == rc);
    }

    // 오름차순으로 100번째 bkey의 순위 및 element와 함께 앞뒤 양방향으로 각 10개 element를 조회한다.
    result = memcached_coll_result_create(memc, NULL);
    bkdata_len= snprintf(bkdata, 15, "%04u", 99);
    rc= memcached_bop_ext_find_position_with_get(memc, test_literal_param("btree:a_btree"),
                                                 (unsigned char *)bkdata, bkdata_len,
                                                 MEMCACHED_COLL_ORDER_ASC, 10, result);
    assert(rc == MEMCACHED_SUCCESS);
    assert(21 == memcached_coll_result_get_count(result));
    assert(99 == memcached_coll_result_get_btree_position(result));
    assert(10 == memcached_coll_result_get_result_position(result));
    if (rc == MEMCACHED_SUCCESS) {
      size_t position = (99-10);
      for (uint32_t i= 0; i < memcached_coll_result_get_count(result); i++) {
        assert(position == memcached_coll_result_get_position(result, i));
        snprintf(buffer, 15, "value%u", ((99-10)+i));
        assert(0 == strcmp(buffer, memcached_coll_result_get_value(result, i)));
        position += 1;
      }
    }
    memcached_coll_result_free(result);

    // 내림차순으로 100번째 bkey의 순위 및 element와 함께 앞뒤 양방향으로 각 10개 element를 조회한다.
    result = memcached_coll_result_create(memc, NULL);
    bkdata_len= snprintf(bkdata, 15, "%04u", ((maxcount-1)-99));
    rc= memcached_bop_ext_find_position_with_get(memc, test_literal_param("btree:a_btree"),
                                                 (unsigned char *)bkdata, bkdata_len,
                                                  MEMCACHED_COLL_ORDER_DESC, 10, result);
    assert(rc == MEMCACHED_SUCCESS);
    assert(21 == memcached_coll_result_get_count(result));
    assert(99 == memcached_coll_result_get_btree_position(result));
    assert(10 == memcached_coll_result_get_result_position(result));
    if (rc == MEMCACHED_SUCCESS) {
      size_t position = (99-10);
      for (uint32_t i= 0; i < memcached_coll_result_get_count(result); i++) {
        assert(position == memcached_coll_result_get_position(result, i));
        snprintf(buffer, 15, "value%u", ((maxcount-1)-(99-10)-i));
        assert(0 == strcmp(buffer, memcached_coll_result_get_value(result, i)));
        position += 1;
      }
    }
    memcached_coll_result_free(result);
}
```
