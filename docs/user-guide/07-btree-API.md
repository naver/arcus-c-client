# B+Tree Item

B+tree item은 하나의 key에 대해 b+tree 구조 기반으로 b+tree key(bkey)로 정렬된 data의 집합을 가진다.

**제약 조건**

- 저장 가능한 최대 element 개수 : 디폴트 4,000개 (attribute 설정으로 최대 50,000개 확장 가능)
- 각 element에서 value 최대 크기 : 16KB
- 하나의 b+tree 내에서 모든 element는 동일한 bkey 유형을 가져야 한다.
  즉, 8바이트 unsigned integer bkey 유형과 byte array bkey 유형이 혼재할 수 없다.

B+tree item 구조와 기본 특징은 [ARCUS Server Ascii Protocol 문서의 내용](https://github.com/naver/arcus-memcached/blob/master/doc/ch02-collection-items.md)을
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

## BKey(B+Tree Key)와 EFlag(Element Flag)

B+tree item에서 사용 가능한 bkey 데이터 타입은 아래 두 가지이다.

- 8바이트 unsigned integer
- 최대 31 크기의 byte array

eflag는 현재 b+tree element에만 존재하는 필드이다.
eflag 데이터 타입은 최대 31 크기의 byte array 타입만 가능하다.

## Element Flag Filter 구조체

B+tree의 element flag에 대한 filtering을 지정하기 위해선, `eflag_filter` 구조체를 사용해야 한다.

먼저, `eflag_filter` 구조체를 생성하는 API는 다음과 같다.
이를 통해, eflag의 전체/부분 값과 특정 값과의 compare 연산을 명시할 수 있다.

```c
memcached_return_t
memcached_coll_eflag_filter_init(memcached_coll_eflag_filter_st *ptr,
                                 const size_t offset,
                                 const unsigned char *value,
                                 const size_t value_length,
                                 memcached_coll_comp_t comp_op)
```

- offset: eflag에서 비교 연산을 취할 데이터의 시작 offset을 바이트 단위로 지정한다.
- value, value_length: 비교 연산을 취할 데이터 값을 지정한다.
- comp_op: 비교 연산을 지정한다.
  - MEMCACHED_COLL_COMP_EQ
  - MEMCACHED_COLL_COMP_NE
  - MEMCACHED_COLL_COMP_LT
  - MEMCACHED_COLL_COMP_LE
  - MEMCACHED_COLL_COMP_GT
  - MEMCACHED_COLL_COMP_GE

비교 연산을 취할 데이터 값인 value는 여러 값을 지정할 수도 있으며, 다음과 같은 API를 사용한다.
비교 데이터 값은 최대 100개 values를 지정할 수 있으며, MEMCACHED_COLL_COMP_EQ 와 MEMCACHED_COLL_COMP_NE 연산자 만을 지원한다.

```c
memcached_return_t
memcached_coll_eflags_filter_init(memcached_coll_eflag_filter_st *ptr,
                                  const size_t offset,
                                  const unsigned char *values,
                                  const size_t value_length,
                                  const size_t value_count,
                                  memcached_coll_comp_t comp_op)
```

- offset: eflag에서 비교 연산을 취할 데이터의 시작 offset을 바이트 단위로 지정한다.
- values, value_length: 비교 연산을 취할 데이터 값을 array 형태로 지정한다.
  - 모든 값의 길이는 동일하게 value_length이어야 한다.
- value_count: 비교 연산을 취할 데이터 값들의 개수를 지정한다.
- comp_op: 비교 연산을 지정한다.
  - MEMCACHED_COLL_COMP_EQ
  - MEMCACHED_COLL_COMP_NE

eflag의 전체/부분 값에 대해 bitwise 연산을 취함으로써 eflag의 특정 bit들만을 골라내어 compare할 수 있다.
이와 같이 `eflag_filter`에 bitwise 연산을 추가할 경우에는 아래의 API를 이용할 수 있다.

```c
memcached_return_t
memcached_coll_eflag_filter_set_bitwise(memcached_coll_eflag_filter_st *ptr,
                                        const unsigned char *value,
                                        const size_t value_length,
                                        memcached_coll_bitwise_t bitwise_op)
```

- value, value_length: eflag에서 bitwise 연산을 취할 값을 지정한다.
- bitwise_op: bitwise 연산을 지정한다.
  - MEMCACHED_COLL_BITWISE_AND
  - MEMCACHED_COLL_BITWISE_OR
  - MEMCACHED_COLL_BITWISE_XOR

## Element Flag Update 구조체

B+tree의 element flag를 변경하기 위해선 `eflag_update` 구조체를 사용해야 한다.

먼저, `eflag_update` 구조체를 생성하는 API는 다음과 같다.
이를 통해, 새로 변경하고자 하는 new element flag 값을 지정할 수 있다.

```c
memcached_return_t
memcached_coll_eflag_update_init(memcached_coll_eflag_update_st *ptr,
                                 const unsigned char *value,
                                 const size_t value_length)
```

만약, eflag의 부분 값만을 변경하고자 한다면, 아래의 함수를 이용할 수 있다.
아래 함수로 부분 값의 시작 위치를 명시하여야 하고, 부분 값과 `eflag_update`의 init 시에 명시한 value에 대해
취할 bitwise 연산을 명시하여야 한다.

```c
memcached_return_t
memcached_coll_eflag_update_set_bitwise(memcached_coll_eflag_update_st *ptr,
                                        const size_t offset,
                                        memcached_coll_bitwise_t bitwise_op)
```

- offset: eflag에서 부분 변경할 부분 데이터의 시작 offset을 바이트 단위로 지정한다.
- bitwise_op: bitwise 연산을 지정한다.
  - MEMCACHED_COLL_BITWISE_AND
  - MEMCACHED_COLL_BITWISE_OR
  - MEMCACHED_COLL_BITWISE_XOR

## B+Tree Query 구조체

memcached_bop_query_st 구조체는 B+tree 조회 조건을 추상화하고 있으며 다양한 API에 사용될 수 있다.

먼저, 아래 함수는 하나의 bkey의 element를 조회하는 query 구조체를 생성한다.

```c
memcached_return_t
memcached_bop_query_init(memcached_bop_query_st *ptr,
                         const uint64_t bkey,
                         memcached_bop_eflag_filter_st *eflag_filter)

memcached_return_t
memcached_bop_ext_query_init(memcached_bop_query_st *ptr,
                             const unsigned char *bkey, const size_t bkey_length,
                             memcached_bop_eflag_filter_st *eflag_filter)
```

아래 함수는 bkey range, element flag filter, offset과 count를 함께 명시하여 query 구조체를 생성한다.

```c
memcached_return_t
memcached_bop_range_query_init(memcached_bop_query_st *ptr,
                               const uint64_t bkey_from, const uint64_t bkey_to,
                               memcached_bop_eflag_filter_st *eflag_filter,
                               size_t offset, size_t count)

memcached_return_t
memcached_bop_ext_range_query_init (memcached_bop_query_st *ptr,
                                    const unsigned char *bkey_from,
                                    const size_t bkey_from_length,
                                    const unsigned char *bkey_to,
                                    const size_t bkey_to_length,
                                    memcached_bop_eflag_filter_st *eflag_filter,
                                    size_t offset, size_t count)
```

## B+Tree Item 생성

새로운 empty b+tree item을 생성한다.

```c
memcached_return_t
memcached_bop_create(memcached_st *ptr,
                     const char *key, size_t key_length,
                     memcached_coll_create_attrs_st *attributes)
```

- key, key_length: b+tree item의 key
- attributes: b+tree item의 속성 정보 [(링크)](08-attribute-API.md#attribute-생성)

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_CREATED: b+tree item이 생성됨.
- not MEMCACHED_SUCCESS
  - MEMCACHED_EXISTS: 동일한 key를 가진 b+tree가 이미 존재함.

B+tree item을 생성하는 예시는 아래와 같다.

```c
int arcus_btree_item_create(memcached_st *memc)
{
  const char *key= "btree:a_key";
  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;
  memcached_return_t rc;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  rc= memcached_bop_create(memc, key, strlen(key), &attributes);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_create: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  assert(memcached_get_last_response_code(memc) == MEMCACHED_CREATED);
  return 0;
}
```

## B+Tree Element 삽입

B+Tree에 하나의 element를 삽입한다.
전자는 8바이트 unsigned integer 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

```c
memcached_return_t
memcached_bop_insert(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const uint64_t bkey,
                     const unsigned char *eflag, size_t eflag_length,
                     const char *value, size_t value_length,
                     memcached_coll_create_attrs_st *attributes)

memcached_return_t
memcached_bop_ext_insert(memcached_st *ptr,
                         const char *key, size_t key_length,
                         const unsigned char *bkey, size_t bkey_length,
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

B+tree element를 삽입하는 예시는 아래와 같다.

```c
int arcus_btree_element_insert(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const char *value= "value";
  const uint64_t bkey= 0;
  // const unsigned char bkey[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  const unsigned char eflag[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  memcached_return_t rc;

  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  rc= memcached_bop_insert(memc, key, strlen(key), bkey, eflag, sizeof(eflag),
                           value, strlen(value), &attributes);
  /* rc= memcached_bop_ext_insert(memc, key, strlen(key),
                                  bkey, sizeof(bkey), eflag, sizeof(eflag),
                                  value, strlen(value), &attributes); */
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_insert: %d(%s)\n",
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

아커스에서 B+tree는 가질 수 있는 최대 엘리먼트 개수가 제한되어 있다. 이 제한 범위 안에서 사용자가 직접 B+tree 크기를 지정할 수도 있는데(maxcount), 이러한 제약조건 때문에 가득 찬 B+tree에 새로운 엘리먼트를 입력하면 설정에 따라 기존의 엘리먼트가 삭제될 수 있다. 이렇게 암묵적으로 삭제되는 엘리먼트를 입력(insert, upsert)시점에 획득할 수 있는 기능을 제공한다.

하지만, C client에서는 이 기능을 아직 제공하지 않고 있다.

## B+Tree Element Upsert

B+Tree에 하나의 element를 upsert하는 함수들이다.
Upsert 연산은 해당 element가 없으면 insert하고, 있으면 update하는 연산이다.
전자는 8바이트 unsigned integer 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

```c
memcached_return_t
memcached_bop_upsert(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const uint64_t bkey,
                     const unsigned char *eflag, size_t eflag_length,
                     const char *value, size_t value_length,
                     memcached_coll_create_attrs_st *attributes)

memcached_return_t
memcached_bop_ext_upsert(memcached_st *ptr,
                         const char *key, size_t key_length,
                         const unsigned char *bkey, size_t bkey_length,
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

B+tree element를 upsert하는 예시는 아래와 같다.

```c
int arcus_btree_element_upsert(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const char *value= "value";
  const uint64_t bkey= 0;
  // const unsigned char bkey[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  const unsigned char eflag[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  memcached_return_t rc;

  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  rc= memcached_bop_upsert(memc, key, strlen(key), bkey, eflag, sizeof(eflag),
                           value, strlen(value), &attributes);
  /* rc= memcached_bop_ext_upsert(memc, key, strlen(key),
                                  bkey, sizeof(bkey), eflag, sizeof(eflag),
                                  value, strlen(value), &attributes); */
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_upsert: %d(%s)\n",
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

## B+Tree Element 변경

B+Tree에서 하나의 element를 변경하는 함수이다. Element의 eflag 그리고/또는 value를 변경한다.
전자는 8바이트 unsigned integer 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

```c
memcached_return_t
memcached_bop_update(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const uint64_t bkey,
                     memcached_coll_eflag_update_st *eflag_update,
                     const char *value, size_t value_length)

memcached_return_t
memcached_bop_ext_update(memcached_st *ptr,
                         const char *key, size_t key_length,
                         const unsigned char *bkey, size_t bkey_length,
                         memcached_coll_eflag_update_st *eflag_update,
                         const char *value, size_t value_length)
```

- key, key_length: b+tree item의 key
- bkey 또는 bkey, bkey_length: element의 bkey
- eflag_update: element의 eflag 변경 요청을 담은 구조체
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
  - MEMCACHED_EFLAG_MISMATCH: eflag_update에 명시된 eflag 데이터가 존재하지 않음.

B+tree element를 변경하는 예시는 아래와 같다.

```c
int arcus_btree_element_update(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const char *value= "value";
  const uint64_t bkey= 0;
  // const unsigned char bkey[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  memcached_return_t rc;

  const unsigned char new_eflag[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };

  memcached_coll_eflag_update_st eflag_update;
  memcached_coll_eflag_update_init(&eflag_update, new_eflag, sizeof(new_eflag));

  rc= memcached_bop_update(memc, key, strlen(key), bkey,
                           &eflag_update, value, strlen(value));
  /* rc= memcached_bop_ext_update(memc, key, strlen(key), bkey, sizeof(bkey),
                                  &eflag_update, value, strlen(value)); */
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_update: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_UPDATED);

  return 0;
}
```

## B+Tree Element 삭제

B+tree에서 element를 삭제하는 함수들은 두 유형이 있다.

첫째, b+tree에서 특정 bkey를 가진 element에 대해 eflag filter 조건을 만족하면 삭제하는 함수이다.

```c
memcached_return_t
memcached_bop_delete(memcached_st *ptr,
                     const char *key, size_t key_length,
                     const uint64_t bkey,
                     memcached_coll_eflag_filter_st *eflag_filter,
                     bool drop_if_empty)

memcached_return_t
memcached_bop_ext_delete(memcached_st *ptr,
                         const char *key, size_t key_length,
                         const unsigned char *bkey, size_t bkey_length,
                         memcached_coll_eflag_filter_st *eflag_filter,
                         bool drop_if_empty)
```

둘째, b+tree에서 bkey range에 해당하는 element들을 스캔하면서
eflag filter 조건을 만족하는 N개의 element들을 삭제하는 함수이다.

```c
memcached_return_t
memcached_bop_delete_by_range(memcached_st *ptr,
                              const char *key, size_t key_length,
                              const uint64_t from, const uint64_t to,
                              memcached_coll_eflag_filter_st *eflag_filter,
                              size_t count, bool drop_if_empty)

memcached_return_t
memcached_bop_ext_delete_by_range(memcached_st *ptr,
                              const char *key, size_t key_length,
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

대표적으로 b+tree에서 bkey range에 해당하는 element들을 스캔하면서
eflag filter 조건을 만족하는 N개의 element들을 삭제하는 예시는 아래와 같다.

```c
int arcus_btree_element_delete_by_range(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const uint64_t from= 0, to= UINT64_MAX;
  /* const unsigned char from[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
     const unsigned char to[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]
                  = {[0 ... MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH - 1] = 0xff}; */
  size_t count= 0;
  bool drop_if_empty= false;
  memcached_return_t rc;

  const size_t comp_offset= 0;
  const unsigned char comp_value[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  memcached_coll_comp_t comp_op= MEMCACHED_COLL_COMP_EQ;

  memcached_coll_eflag_filter_st eflag_filter;
  memcached_coll_eflag_filter_init(&eflag_filter, comp_offset,
                                   comp_value, sizeof(comp_value), comp_op);

  rc= memcached_bop_delete_by_range(memc, key, strlen(key),
                                    from, to, &eflag_filter, count, drop_if_empty);
  /* rc= memcached_bop_ext_delete_by_range(memc, key, strlen(key),
                                           from, sizeof(from), to, sizeof(to),
                                           &eflag_filter, count, drop_if_empty); */
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_delete_by_range: %d(%s)\n",
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

## B+Tree Element 값의 증감

B+tree element의 값을 증가/감소시키는 함수는 아래와 같다.
Element의 값은 숫자형 값이어야 한다.

전자는 8바이트 unsigned integer 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

```c
memcached_return_t
memcached_bop_incr(memcached_st *ptr,
                   const char *key, size_t key_length,
                   const uint64_t bkey,
                   const uint64_t delta, uint64_t *value)
memcached_return_t
memcached_bop_decr(memcached_st *ptr,
                   const char *key, size_t key_length,
                   const uint64_t bkey,
                   const uint64_t delta, uint64_t *value)

memcached_return_t
memcached_bop_ext_incr(memcached_st *ptr,
                   const char *key, size_t key_length,
                   const unsigned char *bkey, size_t bkey_length,
                   const uint64_t delta, uint64_t *value)
memcached_return_t
memcached_bop_ext_decr(memcached_st *ptr,
                       const char *key, size_t key_length,
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
  - MEMCACHED_OUT_OF_RANGE : 주어진 조회 범위에 해당하는 element가 없으나, 조회 범위가 overflow 정책에 의해 삭제되는 영역에 걸쳐 있음. 즉, B+tree 크기 제한으로 인해 삭제되어 조회되지 않은
element가 어딘가(DB)에 존재할 수도 있음을 뜻함.

대표적으로 B+tree element의 값을 증가시키는 예시는 다음과 같다.

```c
int arcus_btree_element_arithmetic(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const uint64_t bkey= 0;
  // const unsigned char bkey[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  uint64_t value= 0;
  uint64_t delta= 10;
  memcached_return_t rc;

  rc= memcached_bop_incr(memc, key, strlen(key), bkey, delta, &value);
  /* rc= memcached_bop_ext_incr(memc, key, strlen(key),
                                bkey, sizeof(bkey), delta, &value); */
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_incr: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  fprintf(stdout, "incremented value: %llu\n", value);
  return 0;
}
```

## B+Tree Element 개수 확인

B+tree element 개수를 확인하는 함수는 두 유형이 있다.

첫째, b+tree에서 특정 bkey를 가진 element에 대해 eflag filter 조건을 만족하는지를 확인하는 함수이다.

```c
memcached_return_t
memcached_bop_count(memcached_st *ptr,
                    const char *key, size_t key_length,
                    const uint64_t bkey,
                    memcached_coll_eflag_filter_st *eflag_filter,
                    size_t *count)

memcached_return_t
memcached_bop_ext_count(memcached_st *ptr,
                        const char *key, size_t key_length,
                        const unsigned char *bkey, size_t bkey_length,
                        memcached_coll_eflag_filter_st *eflag_filter,
                        size_t *count)
```

둘째, b+tree에서 bkey range에 해당하는 element들 중 eflag filter 조건을 만족하는 element 개수를 확인하는 함수이다.

```c
memcached_return_t
memcached_bop_count_by_range(memcached_st *ptr,
                             const char *key, size_t key_length,
                             const uint64_t from, const uint64_t to,
                             memcached_coll_eflag_filter_st *eflag_filter,
                             size_t *count)

memcached_return_t
memcached_bop_ext_count_by_range(memcached_st *ptr,
                                 const char *key, size_t key_length,
                                 const unsigned char *from, size_t from_length,
                                 const unsigned char *to, size_t to_length,
                                 memcached_coll_eflag_filter_st *eflag_filter,
                                 size_t *count)
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

대표적으로 b+tree에서 bkey range에 해당하는 element들 중 eflag filter 조건을 만족하는 element 개수를 확인하는 예시는 다음과 같다.

```c
int arcus_btree_element_count_by_range(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const uint64_t from= 0, to= UINT64_MAX;
  /* const unsigned char from[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
     const unsigned char to[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]
                        = {[0 ... MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH - 1] = 0xff}; */
  size_t count= 0;
  memcached_return_t rc;

  const size_t comp_offset= 0;
  const unsigned char comp_value[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  memcached_coll_comp_t comp_op= MEMCACHED_COLL_COMP_EQ;

  memcached_coll_eflag_filter_st eflag_filter;
  memcached_coll_eflag_filter_init(&eflag_filter, comp_offset,
                                   comp_value, sizeof(comp_value), comp_op);

  rc= memcached_bop_count_by_range(memc, key, strlen(key),
                                   from, to, &eflag_filter, &count);
  /* rc= memcached_bop_ext_count_by_range(memc, key, strlen(key),
                                          from, sizeof(from), to, sizeof(to),
                                          &eflag_filter, &count); */
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_count_by_range: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  fprintf(stdout, "count: %lu\n", count);
  return 0;
}
```

## B+Tree Element 조회

B+tree element를 조회하는 함수는 세 유형이 있다.

첫째, b+tree에서 특정 bkey를 가진 element에 대해 eflag filter 조건을 만족하면 조회하는 함수이다.

```c
memcached_return_t
memcached_bop_get(memcached_st *ptr,
                  const char *key, size_t key_length,
                  const uint64_t bkey,
                  memcached_coll_eflag_filter_st *eflag_filter,
                  bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)

memcached_return_t
memcached_bop_ext_get(memcached_st *ptr,
                  const char *key, size_t key_length,
                  const unsigned char *bkey, size_t bkey_length,
                  memcached_coll_eflag_filter_st *eflag_filter,
                  bool with_delete, bool drop_if_empty,
                  memcached_coll_result_st *result)
```

둘째, b+tree에서 bkey range에 해당하는 element들을 스캔하면서
eflag filter 조건을 만족하는 element들 중 offset 개를 skip한 후 count 개의 element를 조회하는 함수이다.

```c
memcached_return_t
memcached_bop_get_by_range(memcached_st *ptr,
                           const char *key, size_t key_length,
                           const uint64_t from, const uint64_t to,
                           memcached_coll_eflag_filter_st *eflag_filter,
                           const size_t offset, const size_t count,
                           bool with_delete, bool drop_if_empty,
                           memcached_coll_result_st *result)

memcached_return_t
memcached_bop_ext_get_by_range(memcached_st *ptr,
                           const char *key, size_t key_length,
                           const unsigned char *from, size_t from_length,
                           const unsigned char *to, size_t to_length,
                           memcached_coll_eflag_filter_st *eflag_filter,
                           const size_t offset, const size_t count,
                           bool with_delete, bool drop_if_empty,
                           memcached_coll_result_st *result)
```

셋째, query 구조체를 이용하여 b+tree element를 조회하는 함수이다.

```c
memcached_return_t
memcached_bop_get_by_query(memcached_st *ptr,
                           const char *key, size_t key_length,
                           memcached_bop_query_st *query
                           bool with_delete, bool drop_if_empty,
                           memcached_coll_result_st *result)
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

```c
memcached_coll_result_st *
memcached_coll_result_create(const memcached_st *ptr,
                             memcached_coll_result_st *result)
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

대표적으로 b+tree에서 bkey range에 해당하는 element들 중 eflag filter 조건을 만족하는 element를 조회하는 예시는 다음과 같다.

첫째, 각 인자를 직접 API에 전달하는 예시이다.

```c
int arcus_btree_element_get_by_range(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const uint64_t from= UINT64_MAX, to= 0;
  /* const unsigned char from[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]
                    = {[0 ... MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH - 1] = 0xff};
     const unsigned char to[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, }; */
  const size_t offset= 0;
  const size_t count= 0;
  bool with_delete= false;
  bool drop_if_empty= false;
  memcached_coll_result_st result;
  memcached_return_t rc;

  const size_t comp_offset= 0;
  const unsigned char comp_value[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  memcached_coll_comp_t comp_op= MEMCACHED_COLL_COMP_EQ;

  memcached_coll_eflag_filter_st eflag_filter;
  memcached_coll_eflag_filter_init(&eflag_filter, comp_offset,
                                   comp_value, sizeof(comp_value), comp_op);

  memcached_coll_result_create(memc, &result);

  do {
    rc= memcached_bop_get_by_range(memc, key, strlen(key), from, to,
                                   &eflag_filter, offset, count,
                                   with_delete, drop_if_empty, &result);
    /* rc= memcached_bop_ext_get_by_range(memc, key, strlen(key),
                                          from, sizeof(from), to, sizeof(to),
                                          &eflag_filter, offset, count,
                                          with_delete, drop_if_empty, &result); */
    if (memcached_failed(rc)) {
      fprintf(stderr, "Failed to memcached_bop_get_by_range: %d(%s)\n",
              rc, memcached_strerror(memc, rc));
      break;
    }

    memcached_return_t last_response= memcached_get_last_response_code(memc);
    assert(rc == MEMCACHED_SUCCESS);
    assert(last_response == MEMCACHED_END ||
           last_response == MEMCACHED_TRIMMED ||
           last_response == MEMCACHED_DELETED ||
           last_response == MEMCACHED_DELETED_DROPPED);

    for (size_t i=0; i<memcached_coll_result_get_count(&result); i++) {
      uint64_t bkey= memcached_coll_result_get_bkey(&result, i);
      /* memcached_hexadecimal_st *hex_bkey;
         char str_bkey[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
         hex_bkey = memcached_coll_result_get_bkey_ext(&result, i);
         memcached_hexadecimal_to_str(hex_bkey, str_bkey, sizeof(str_bkey)); */
      const char* value= memcached_coll_result_get_value(&result, i);
      fprintf(stdout, "memcached_bop_get_by_range: %s : %llu => %s\n",
              key, bkey, value);
      /* fprintf(stdout, "memcached_bop_get_by_range: %s : %s => %s\n",
                 key, str_bkey, value); */
    }
  } while(0);

  memcached_coll_result_free(&result);
  return (rc == MEMCACHED_SUCCESS) ? 0 : -1;
}
```

둘째, query 구조체를 이용하는 예시이다.

```c
int arcus_btree_element_get_by_query(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const uint64_t from= UINT64_MAX, to= 0;
  /* const unsigned char from[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]
                    = {[0 ... MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH - 1] = 0xff};
     const unsigned char to[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, }; */
  const size_t offset= 0;
  const size_t count= 0;
  bool with_delete= false;
  bool drop_if_empty= false;
  memcached_coll_result_st result;
  memcached_return_t rc;

  const size_t comp_offset= 0;
  const unsigned char comp_value[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  memcached_coll_comp_t comp_op= MEMCACHED_COLL_COMP_EQ;

  memcached_coll_eflag_filter_st eflag_filter;
  memcached_coll_eflag_filter_init(&eflag_filter, comp_offset,
                                   comp_value, sizeof(comp_value), comp_op);

  memcached_coll_query_st query;
  memcached_bop_range_query_init(&query, from, to, &eflag_filter, offset, count);
  /* memcached_bop_ext_range_query_init(&query, from, sizeof(from), to, sizeof(to),
                                        &eflag_filter, offset, count); */

  memcached_coll_result_create(memc, &result);

  do {
    rc= memcached_bop_get_by_query(memc, key, strlen(key), &query,
                                   with_delete, drop_if_empty, &result);
    if (memcached_failed(rc)) {
      fprintf(stderr, "Failed to memcached_bop_get_by_query: %d(%s)\n",
              rc, memcached_strerror(memc, rc));
      break;
    }

    memcached_return_t last_response= memcached_get_last_response_code(memc);
    assert(rc == MEMCACHED_SUCCESS);
    assert(last_response == MEMCACHED_END ||
           last_response == MEMCACHED_TRIMMED ||
           last_response == MEMCACHED_DELETED ||
           last_response == MEMCACHED_DELETED_DROPPED);

    for (size_t i=0; i<memcached_coll_result_get_count(&result); i++) {
      uint64_t bkey= memcached_coll_result_get_bkey(&result, i);
      /* memcached_hexadecimal_st *hex_bkey;
         char str_bkey[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
         hex_bkey= memcached_coll_result_get_bkey_ext(&result, i)
         memcached_hexadecimal_to_str(hex_bkey, str_bkey, sizeof(str_bkey)); */
      const char* value= memcached_coll_result_get_value(&result, i);
      fprintf(stdout, "memcached_bop_get_by_query: %s : %llu => %s\n",
              key, bkey, value);
      /* fprintf(stdout, "memcached_bop_get_by_query: %s : %s => %s\n",
                 key, str_bkey, value); */
    }
  } while(0);

  memcached_coll_result_free(&result);
  return (rc == MEMCACHED_SUCCESS) ? 0 : -1;
}
```

## B+Tree Element 일괄 삽입

B+tree에 여러 element를 한번에 삽입하는 함수는 두 유형이 있다.

첫째, 하나의 key가 가리키는 b+tree에 다수의 element들을 삽입하는 함수이다.
전자는 8바이트 unsigned integer 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

```c
memcached_return_t
memcached_bop_piped_insert(memcached_st *ptr,
                           const char *key,
                           const size_t key_length,
                           const size_t number_of_piped_items,
                           const uint64_t *bkeys,
                           const unsigned char * const *eflags,
                           const size_t *eflags_length,
                           const char * const *values,
                           const size_t *values_length,
                           memcached_coll_create_attrs_st *attributes,
                           memcached_return_t *results,
                           memcached_return_t *piped_rc)

memcached_return_t
memcached_bop_ext_piped_insert(memcached_st *ptr,
                               const char *key,
                               const size_t key_length,
                               const size_t number_of_piped_items,
                               const unsigned char * const *bkeys,
                               const size_t *bkeys_length,
                               const unsigned char * const *eflags,
                               const size_t *eflags_length,
                               const char * const *values,
                               const size_t *values_length,
                               memcached_coll_create_attrs_st *attributes,
                               memcached_return_t *results,
                               memcached_return_t *piped_rc)
```

- key, key_length: b+tree item의 key
- number_of_piped_items: 한번에 삽입할 element 개수
- bkeys 또는 bkeys, bkeys_length: element 개수만큼의 bkey array (필수)
- eflags, eflags_length: element 개수만큼의 eflag array (옵션)
- values, values_length: element 개수만큼의 value array (필수)
- attributes: B+tree 없을 시에 attributes에 따라 empty b+tree를 생성 후에 element를 삽입한다.

둘째, 여러 key들이 가리키는 b+tree들에 각각 하나의 element를 삽입하는 함수이다.
전자는 8바이트 unsigned integer 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

```c
memcached_return_t
memcached_bop_piped_insert_bulk(memcached_st *ptr,
                                const char * const *keys,
                                const size_t *key_length,
                                const size_t number_of_keys,
                                const uint64_t bkey,
                                const unsigned char *eflag, size_t eflag_length,
                                const char *value, size_t value_length,
                                memcached_coll_create_attrs_st *attributes,
                                memcached_return_t *results,
                                memcached_return_t *piped_rc)

memcached_return_t
memcached_bop_ext_piped_insert_bulk(memcached_st *ptr,
                                    const char * const *keys,
                                    const size_t *key_length,
                                    const size_t number_of_keys,
                                    const unsigned char *bkey, size_t bkey_length,
                                    const unsigned char *eflag, size_t eflag_length,
                                    const char *value, size_t value_length,
                                    memcached_coll_create_attrs_st *attributes,
                                    memcached_return_t *results,
                                    memcached_return_t *piped_rc)
```

- keys, key_length: 다수 b+tree items의 key array
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

B+tree에 여러 element를 한번에 삽입하는 예시는 다음과 같다.

첫째, 하나의 key가 가리키는 b+tree에 다수의 element들을 삽입하는 예시이다.

```c
int arcus_btree_element_piped_insert(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const char * const values[]= {"value1", "value2", "value3"};
  const uint64_t bkeys[]= {0, 1, 2};
  /* unsigned char bkeys[][MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {{0, }, {1, }, {2, }};
     unsigned char *bkeys_ptr[]= {bkeys[0], bkeys[1], bkeys[2]}; */
  unsigned char eflags[3][MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {{0,}};
  unsigned char *eflags_ptr[3]= {eflags[0], eflags[1], eflags[2]};
  size_t number_of_piped_items= 3;
  size_t bkeys_len[3];
  size_t values_len[3];
  size_t eflags_len[3];
  memcached_return_t rc;
  memcached_return_t piped_rc;
  memcached_return_t results[3];

  uint32_t flags= 0;
  uint32_t exptime= 600;
  uint32_t maxcount= 1000;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  for(size_t i=0; i<number_of_piped_items; i++)
  {
    values_len[i]= strlen(values[i]);
    bkeys_len[i]= sizeof(bkeys[i]);
    eflags_len[i]= sizeof(eflags[i]);
  }

  rc= memcached_bop_piped_insert(memc, key, strlen(key), number_of_piped_items,
                                 bkeys, eflags_ptr, eflags_len, values, values_len,
                                 &attributes, results, &piped_rc);
  /* rc= memcached_bop_ext_piped_insert(memc, key, strlen(key), number_of_piped_items,
                                        bkeys_ptr, bkeys_len, eflags_ptr, eflags_len,
                                        values, values_len, &attributes, results,
                                        &piped_rc); */
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_piped_insert: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  if (piped_rc == MEMCACHED_ALL_FAILURE) {
    fprintf(stderr, "Failed to memcached_bop_piped_insert: All Failures\n");
    return -1;
  }

  if (piped_rc == MEMCACHED_SOME_SUCCESS) {
    for (size_t i=0; i<number_of_piped_items; i++) {
      fprintf(stderr, "Failed to memcached_bop_piped_insert: "
                      "%s : %llu => %s %d(%s)\n",
              key, bkeys[i], values[i], results[i],
              memcached_strerror(memc, results[i]));
      /* memcached_hexadecimal_st hex_bkey= {(unsigned char *)bkeys[i],
                                             sizeof(bkeys[i]), 0};
         char str_bkey[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
         memcached_hexadecimal_to_str(&hex_bkey, str_bkey, sizeof(str_bkey));
         fprintf(stderr, "Failed to memcached_bop_piped_insert: "
                         "%s : %s => %s %d(%s)\n",
                 key, str_bkey, values[i], results[i],
                 memcached_strerror(memc, results[i])); */
    }
  }

  assert(rc == MEMCACHED_SUCCESS);
  return 0;
}
```

둘째, 여러 key들이 가리키는 b+tree들에 각각 하나의 element를 삽입하는 예시이다.

```c
int arcus_btree_element_piped_insert_bulk(memcached_st *memc)
{
  const char * const keys[]= { "btree:a_key1", "btree:a_key2", "btree:a_key3" };
  const uint64_t bkey= 0;
  // const unsigned char bkey[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  const char *value= "value";
  const unsigned char eflag[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
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

  rc= memcached_bop_piped_insert_bulk(memc, keys, keys_len, number_of_keys,
                                      bkey, eflag, sizeof(eflag),
                                      value, strlen(value), &attributes,
                                      results, &piped_rc);
  /* rc= memcached_bop_ext_piped_insert_bulk(memc, keys, keys_len, number_of_keys,
                                             bkey, sizeof(bkey), eflag, sizeof(eflag),
                                             value, strlen(value), &attributes,
                                             results, &piped_rc); */

  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_piped_insert_bulk: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  if (piped_rc == MEMCACHED_ALL_FAILURE) {
    fprintf(stderr, "Failed to memcached_bop_piped_insert_bulk: All Failures\n");
    return -1;
  }

  if (piped_rc == MEMCACHED_SOME_SUCCESS) {
    /* memcached_hexadecimal_st hex_bkey= {(unsigned char *)bkey, sizeof(bkey), 0};
       char str_bkey[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
       memcached_hexadecimal_to_str(&hex_bkey, str_bkey, sizeof(str_bkey)); */
    for (size_t i=0; i<number_of_keys; i++) {
      fprintf(stderr, "Failed to memcached_bop_piped_insert_bulk: "
                      "%s : %llu => %s %d(%s)\n",
              keys[i], bkey, value, results[i],
              memcached_strerror(memc, results[i]));
      /* fprintf(stderr, "Failed to memcached_bop_piped_insert_bulk: "
                         "%s : %s => %s %d(%s)\n",
                 keys[i], str_bkey, value, results[i],
                 memcached_strerror(memc, results[i])); */
    }
  }

  assert(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_SOME_ERRORS);
  return 0;
}
```

## B+tree Element 일괄 조회

서로 다른 key로 분산되어 있는 b+tree들의 element들을 한 번의 요청으로 조회할 수 있는 기능이다.
이 기능은 비동기(asynchronous) 방식으로 수행하며,
(1) 다수 b+tree들의 element 조회 요청을 보내는 단계와
(2) 조회 결과를 받아내는 단계로 구분된다.

첫째 단계로, 다수 b+tree들에 대한 element 조회 요청을 보내는 함수는 아래와 같다.
B+tree element 조회 조건는 query 구조체를 이용하여 명시한다.

```c
memcached_return_t
memcached_bop_mget(memcached_st *ptr,
                   const char * const *keys,
                   const size_t *key_length,
                   const size_t number_of_keys,
                   memcached_coll_query_st *query)
```

- keys, key_length: 다수 b+tree item들의 key array
- number_of_keys: key 개수
  - key array에 담을 수 있는 최대 key 개수는 200개로 제한된다.
- query: 조회 조건을 가진 query 구조체
  - query 구조체를 구성할 때 count는 1 이상, 50 이하의 값을 가져야 한다.

Response code는 아래와 같다.

- MEMCACHED_SUCCESS : 각 key를 담당하는 모든 캐시 서버에 성공적으로 조회 요청을 보냄.
- MEMCACHED_SOME_SUCCESS : 각 key를 담당하는 캐시 서버로의 요청이 일부 실패함.
- MEMCACHED_FAILURE : 각 key를 담당하는 모든 캐시 서버로의 요청이 모두 실패함.

둘째 단계로, element 조회 결과를 iteration 방식으로 하나씩 가져오기 위한 함수는 아래와 같다.

```c
memcached_coll_result_st *
memcached_coll_fetch_result(memcached_st *ptr,
                            memcached_coll_result_st *result,
                            memcached_return_t *error)
```

조회 결과는 다음과 같다.

- result != null
  - MEMCACHED_SUCCESS: 정상적으로 element를 조회함.
  - MEMCACHED_TRIMMED: 정상적으로 element를 조회하였으나, 조회 범위가 특정 B+tree의 overflow 정책에 의해 삭제되는 영역에 걸쳐 있음. 즉, 해당 B+tree 크기 제한으로 인해 삭제되어
조회되지 않은 element가 존재할 수 있음.
- result == null
  - MEMCACHED_NOT_FOUND: 주어진 key를 찾을 수 없음.
  - MEMCACHED_NOT_FOUND_ELEMENT: 조회 조건에 해당하는 element를 찾을 수 없음.
  - MEMCACHED_OUT_OF_RANGE: 주어진 조회 조건에 해당하는 element가 없으나,
조회 범위가 overflow 정책에 의해 삭제되는 영역에 걸쳐 있음.
즉, 해당 B+tree 크기 제한으로 인해 삭제되어 조회되지 않은 element가 존재할 수 있음.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.

B+tree element 일괄 조회하는 예시는 아래와 같다.

```c
int arcus_btree_element_mget(memcached_st *memc)
{
  const char * const keys[]= {"btree:a_key1", "btree:a_key2", "btree:a_key3"};
  const uint64_t from= UINT64_MAX, to= 0;
  /* const unsigned char from[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]
                  = {[0 ... MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH - 1] = 0xff};
     const unsigned char to[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, }; */
  size_t number_of_keys= 3;
  size_t keys_len[3];
  size_t offset= 0;
  size_t count= 50;
  memcached_return_t rc;

  const size_t comp_offset= 0;
  const unsigned char comp_value[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  memcached_coll_comp_t comp_op= MEMCACHED_COLL_COMP_EQ;

  memcached_coll_eflag_filter_st eflag_filter;
  memcached_coll_eflag_filter_init(&eflag_filter, comp_offset,
                                   comp_value, sizeof(comp_value), comp_op);

  memcached_coll_query_st query;
  memcached_bop_range_query_init(&query, from, to, &eflag_filter, offset, count);
  /* memcached_bop_ext_range_query_init(&query, from, sizeof(from), to, sizeof(to),
                                        &eflag_filter, offset, count); */

  for(size_t i=0; i<number_of_keys; i++)
  {
    keys_len[i]= strlen(keys[i]);
  }

  rc= memcached_bop_mget(memc, keys, keys_len, number_of_keys, &query);
  if (rc == MEMCACHED_FAILURE) {
    fprintf(stderr, "Failed to memcached_bop_mget: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_SOME_SUCCESS);

  memcached_coll_result_st result, *ptr;
  memcached_coll_result_create(memc, &result);
  while (memcached_coll_fetch_result(memc, &result, &rc) != NULL)
  {
    const char *key= memcached_coll_result_get_key(&result);
    if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_TRIMMED) {
      fprintf(stderr, "Failed to memcached_coll_fetch_result: %s %d(%s)\n",
              key, rc, memcached_strerror(memc, rc));
    } else {
      for (size_t i=0; i<memcached_coll_result_get_count(&result); i++) {
        uint64_t bkey= memcached_coll_result_get_bkey(&result, i);
        /* memcached_hexadecimal_st *hex_bkey;
           char str_bkey[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
           hex_bkey= memcached_coll_result_get_bkey_ext(&result, i);
           memcached_hexadecimal_to_str(hex_bkey, str_bkey, sizeof(str_bkey)); */
        const char *value= memcached_coll_result_get_value(&result, i);
        fprintf(stdout, "memcached_coll_fetch_result: %s : %llu => %s\n",
                key, bkey, value);
        /* fprintf(stderr, "memcached_coll_fetch_result: %s : %s => %s\n",
                   key, str_bkey, value); */
      }
    }
    memcached_coll_result_free(&result);
  }

  return 0;
}
```

## B+tree Element Sort-Merge 조회

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

```c
memcached_return_t
memcached_bop_smget(memcached_st *ptr,
                    const char * const *keys,
                    const size_t *key_length,
                    const size_t number_of_keys,
                    memcached_bop_query_st *query,
                    memcached_coll_smget_result_st *result);
```
- keys, key_length: 다수 b+tree item들의 key array
- number_of_keys: key 개수
  - key array에 담을 수 있는 최대 key 개수는 2000 개로 제한된다.
- query: 조회 조건을 가진 query 구조체
  - query 구조체를 구성할 때 count는 1 이상의 값을 가져야 하며, offset + count가 1000을 초과해서는 아니 된다.
- result: sort-merge 조회 결과는 담는 구조체

Sort-Merge 조회 질의를 표현하는 memcached_bop_query_st 구조체 생성 방법은
기존 sort-merge 조회와 신규 sort-merge 조회에 따라 다르다.
기존 sort-merge 조회에서 memcached_bop_query_st 구조체 생성 방법은
앞서 설명한 [B+Tree Query 구조체](07-btree-API.md#btree-query-구조체) 참고하기 바란다.
신규 sort-merge 조회에서는 아래의 sort-merge 질의 생성하는 전용 API를 사용해
bkey range, element flag, count 그리고 unique를 명시하여 query 구조체를 생성한다.
마지막 인자인 unique가 false이면 중복 bkey를 허용하여 조회하며,
true이면 중복 bkey를 제거하여 unique bkey만을 조회한다.

```c
memcached_return_t
memcached_bop_smget_query_init(memcached_bop_query_st *ptr,
                               const uint64_t bkey_from, const uint64_t bkey_to,
                               memcached_coll_eflag_filter_st *eflag_filter,
                               size_t count, bool unique)
memcached_return_t
memcached_bop_ext_smget_query_init(memcached_bop_query_st *ptr,
                               const unsigned char *bkey_from, size_t bkey_from_length,
                               const unsigned char *bkey_to, size_t bkey_to_length,
                               memcached_coll_eflag_filter_st *eflag_filter,
                               size_t count, bool unique)
```

Response code는 아래와 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: 여러 B+tree에서 정상적으로 element를 조회하였음.
  - MEMCACHED_DUPLICATED: 여러 B+tree에서 정상적으로 element를 조회하였으나 중복된 bkey가 존재함.
  - MEMCACHED_TRIMMED (기존 sort-merge 조회에 한정)
    - 정상적으로 element를 조회하였으나, 조회 범위가 특정 B+tree의 overflow 정책에 의해 삭제되는 영역에 걸쳐 있음.
    - 즉, 해당 B+tree 크기 제한으로 인해 삭제되어 조회되지 않은 element가 어딘가(DB)에 존재할 수도 있음을 뜻함.
  - MEMCACHED_DUPLICATED_TRIMMED (기존 sort-merge 조회에 한정)
    - MEMCACHED_DUPLICATED 상태와 MEMCACHED_TRIMMED 상태가 모두 존재.
- not MEMCACHED_SUCCESS
  - MEMCACHED_OUT_OF_RANGE (기존 sort-merge 조회에 한정)
    - 주어진 조회 범위에 해당하는 element가 없으나, 조회 범위가 overflow 정책에 의해 삭제되는 영역에 걸쳐 있음.
    - 즉, B+tree 크기 제한으로 인해 삭제되어 조회되지 않은 element가 어딘가(DB)에 존재할 수도 있음을 뜻함.
  - MEMCACHED_TYPE_MISMATCH: 주어진 key에 해당하는 자료구조가 B+tree가 아님.
  - MEMCACHED_BKEY_MISMATCH: 주어진 bkey 유형과 해당 B+tree의 bkey 유형이 다름.
  - MEMCACHED_ATTR_MISMATCH: smget에 참여하는 B+tree들의 attribute가 서로 다름.
    - 참고로 smget에 참여하는 B+tree들은 maxcount, maxbkeyrange, overflowaction이 모두 동일해야 함.
    - arcus-memcached 1.11.3 이후로 attribute 통일 제약이 사라짐.

Sort-merge 조회 결과는 memcached_coll_smget_result_st 구조체로 받아온다.
Sort-merge 조회하기 전에 memcached_coll_smget_result_st 구조체를 초기화하여 사용해야 하고,
memcached_coll_smget_result_st 구조체에서 조회 결과를 모두 얻은 후에는 다시 반환하여야 한다.
이를 수행하는 두 함수는 아래와 같다.

```c
memcached_coll_smget_result_st *
memcached_coll_smget_result_create(const memcached_st *ptr,
                                   memcached_coll_smget_result_st *result)
void
memcached_coll_smget_result_free(memcached_coll_smget_result_st *result)
```

함수                               | 기능
---------------------------------- | ---------------------------------------------------------------
memcached_coll_smget_result_create | result 구조체를 초기화한다. result에 NULL을 넣으면 새로 allocate 하여 반환한다.
memcached_coll_smget_result_free   | result 구조체를 초기화하고 allocate 된 경우 free 한다.

memcached_coll_smget_result_st 구조체에서 조회 결과를 얻기 위한 함수들은 아래와 같다.

```c
size_t
memcached_coll_smget_result_get_count(memcached_coll_smget_result_st *result)
const char *
memcached_coll_smget_result_get_key(memcached_coll_smget_result_st *result, size_t index)
uint64_t
memcached_coll_smget_result_get_bkey(memcached_coll_smget_result_st *result, size_t index)
memcached_hexadecimal_st *
memcached_coll_smget_result_get_bkey_ext(memcached_coll_smget_result_st *result, size_t index)
memcached_hexadecimal_st *
memcached_coll_smget_result_get_eflag(memcached_coll_smget_result_st *result, size_t index)
const char *
memcached_coll_smget_result_get_value(memcached_coll_smget_result_st *result, size_t index)
size_t
memcached_coll_smget_result_get_value_length(memcached_coll_smget_result_st *result, size_t index)
size_t
memcached_coll_smget_result_get_missed_key_count(memcached_coll_smget_result_st *result)
const char *
memcached_coll_smget_result_get_missed_key(memcached_coll_smget_result_st *result, size_t index)
size_t
memcached_coll_smget_result_get_missed_key_length(memcached_coll_smget_result_st *result, size_t index)
memcached_return_t
memcached_coll_smget_result_get_missed_cause(memcached_coll_smget_result_st *result, size_t index)
size_t
memcached_coll_smget_result_get_trimmed_key_count(memcached_coll_smget_result_st *result)
const char *
memcached_coll_smget_result_get_trimmed_key(memcached_coll_smget_result_st *result, size_t index)
size_t
memcached_coll_smget_result_get_trimmed_key_length(memcached_coll_smget_result_st *result, size_t index)
uint64_t
memcached_coll_smget_result_get_trimmed_bkey(memcached_coll_smget_result_st *result, size_t index)
memcached_hexadecimal_st *
memcached_coll_smget_result_get_trimmed_bkey_ext(memcached_coll_smget_result_st *result, size_t index)
```

함수                                               | 기능
-------------------------------------------------- | ---------------------------------------------------------------
memcached_coll_smget_result_get_count              | 조회 결과의 element 개수
memcached_coll_smget_result_get_key                | Element array에서 index 위치에 있는 element의 key
memcached_coll_smget_result_get_bkey               | Element array에서 index 위치에 있는 element의 unsigned int bkey
memcached_coll_smget_result_get_bkey_ext           | Element array에서 index 위치에 있는 element의 byte array bkey
memcached_coll_smget_result_get_eflag              | Element array에서 index 위치에 있는 element의 eflag
memcached_coll_smget_result_get_value              | Element array에서 index 위치에 있는 element의 value
memcached_coll_smget_result_get_value_length       | Element array에서 index 위치에 있는 element의 value 길이
memcached_coll_smget_result_get_missed_key_count   | 조회 결과의 missed key 개수
memcached_coll_smget_result_get_missed_key         | Missed key array에서 index 위치에 있는 key
memcached_coll_smget_result_get_missed_key_length  | Missed key array에서 index 위치에 있는 key 길이
memcached_coll_smget_result_get_missed_cause       | 해당 missed key가 miss된 원인 (신규 sort-merge 조회 한정)
memcached_coll_smget_result_get_trimmed_key_count  | 조회 결과의 trimmed key 개수 (신규 sort-merge 조회 한정)
memcached_coll_smget_result_get_trimmed_key_key    | Trimmed key array에서 index 위치에 있는 key (신규 sort-merge 조회 한정)
memcached_coll_smget_result_get_trimmed_key_length | Trimmed key array에서 index 위치에 있는 key 길이 (신규 sort-merge 조회 한정)
memcached_coll_smget_result_get_trimmed_bkey       | 해당 trimmed key의 마지막 unsigned int bkey (신규 sort-merge 조회 한정)
memcached_coll_smget_result_get_trimmed_bkey_ext   | 해당 trimmed key의 마지막 byte array bkey (신규 sort-merge 조회 한정)

B+tree element sort-merge 조회하는 예시는 아래와 같다.

```c
int arcus_btree_element_smget(memcached_st *memc)
{
  const char * const keys[]= { "btree:a_key1", "btree:a_key2", "btree:a_key3" };
  const uint64_t from= UINT64_MAX, to= 0;
  /* const unsigned char from[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]
                  = {[0 ... MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH - 1] = 0xff};
     const unsigned char to[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, }; */
  size_t number_of_keys= 3;
  size_t keys_len[3];
  size_t count= 50;
  bool unique= false;
  memcached_coll_smget_result_st result;
  memcached_return_t rc;

  const size_t comp_offset= 0;
  const unsigned char comp_value[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  memcached_coll_comp_t comp_op= MEMCACHED_COLL_COMP_EQ;

  memcached_coll_eflag_filter_st eflag_filter;
  memcached_coll_eflag_filter_init(&eflag_filter, comp_offset,
                                   comp_value, sizeof(comp_value), comp_op);

  memcached_bop_query_st query;
  memcached_bop_smget_query_init(&query, from, to, &eflag_filter, count, unique);
  /* memcached_bop_ext_smget_query_init(&query, from, sizeof(from), to, sizeof(to),
                                        &eflag_filter, count, unique); */

  memcached_coll_smget_result_create(memc, &result);

  for(size_t i=0; i<number_of_keys; i++)
  {
    keys_len[i]= strlen(keys[i]);
  }

  rc= memcached_bop_smget(memc, keys, keys_len, number_of_keys, &query, &result);
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_smget: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  memcached_return_t last_response= memcached_get_last_response_code(memc);
  assert(rc == MEMCACHED_SUCCESS);
  assert(last_response == MEMCACHED_END ||
         last_response == MEMCACHED_DUPLICATED ||
         last_response == MEMCACHED_TRIMMED ||
         last_response == MEMCACHED_DUPLICATED_TRIMMED);

  for(size_t i=0; i<memcached_coll_smget_result_get_count(&result); i++)
  {
    const char *key= memcached_coll_smget_result_get_key(&result, i);
    uint64_t bkey= memcached_coll_smget_result_get_bkey(&result, i);
    /* memcached_hexadecimal_st *hex_bkey;
       char str_bkey[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
       hex_bkey= memcached_coll_smget_result_get_bkey_ext(&result, i);
       memcached_hexadecimal_to_str(hex_bkey, str_bkey, sizeof(str_bkey)); */
    const char *value= memcached_coll_smget_result_get_value(&result, i);

    fprintf(stdout, "memcached_bop_smget: %s : %llu => %s\n", key, bkey, value);
    // fprintf(stdout, "memcached_bop_smget: %s : %s => %s\n", key, str_bkey, value);
  }

  for(size_t i=0; i<memcached_coll_smget_result_get_missed_key_count(&result); i++)
  {
    const char *key= memcached_coll_smget_result_get_missed_key(&result, i);
    memcached_return_t cause= memcached_coll_smget_result_get_missed_cause(&result, i);

    fprintf(stdout, "missed key: %s (%s)\n", key, memcached_strerror(memc, cause));
  }

  for(size_t i=0; i<memcached_coll_smget_result_get_trimmed_key_count(&result); i++)
  {
    const char *key= memcached_coll_smget_result_get_trimmed_key(&result, i);
    uint64_t bkey= memcached_coll_smget_result_get_trimmed_bkey(&result, i);
    /* memcached_hexadecimal_st *hex_bkey=
                memcached_coll_smget_result_get_trimmed_bkey_ext(&result, i);
       char str_bkey[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
       memcached_hexadecimal_to_str(hex_bkey, str_bkey, sizeof(str_bkey)); */
    fprintf(stdout, "trimmed key: %s : %llu\n", key, bkey);
    // fprintf(stdout, "trimmed key: %s : %s\n", key, str_bkey);
  }

  memcached_coll_smget_result_free(&result);
  return 0;
}
```

## B+Tree Element 순위 조회

B+Tree element 순위를 조회하는 함수는 아래와 같다.
전자는 8바이트 unsigned integer 타입의 bkey를, 후자는 최대 31 크기의 byte array 타입의 bkey를 사용한다.

```c
memcached_return_t
memcached_bop_find_position(memcached_st *ptr,
                            const char *key, size_t key_length,
                            const uint64_t bkey,
                            memcached_coll_order_t order,
                            size_t *position)

memcached_return_t
memcached_bop_ext_find_position(memcached_st *ptr,
                            const char *key, size_t key_length,
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

B+Tree element 순위를 조회하는 예시는 아래와 같다.

```c
int arcus_btree_find_position(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const uint64_t bkey= 1000;
  // const unsigned char bkey[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0x10, };
  memcached_coll_order_t order= MEMCACHED_COLL_ORDER_ASC;
  memcached_return_t rc;

  size_t position= -1;
  rc= memcached_bop_find_position(memc, key, strlen(key), bkey, order, &position);
  /* rc= memcached_bop_ext_find_position(memc, key, strlen(key),
                                         bkey, sizeof(bkey), order, &position); */
  if (memcached_failed(rc)) {
    fprintf(stderr, "Failed to memcached_bop_find_position: %d(%s)\n",
            rc, memcached_strerror(memc, rc));
    return -1;
  }

  assert(rc == MEMCACHED_SUCCESS);
  fprintf(stdout, "memcached_bop_find_position: %d\n", (int)position);
  return 0;
}
```

## B+Tree 순위 기반의 Element 조회

B+Tree에서 순위 범위로 element를 조회하는 함수는 아래와 같다.

```c
memcached_return_t
memcached_bop_get_by_position(memcached_st *ptr,
                              const char *key, size_t key_length,
                              memcached_coll_order_t order,
                              size_t from_position, size_t to_position,
                              memcached_coll_result_st *result)
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

B+Tree에서 순위 범위로 element를 조회하는 예시는 아래와 같다.

```c
int arcus_btree_get_by_position(memcached_st *memc)
{
  const char *key= "btree:a_key";
  size_t from= 0, to= 99;
  memcached_coll_order_t order= MEMCACHED_COLL_ORDER_ASC;
  memcached_coll_result_st result;
  memcached_return_t rc;

  memcached_coll_result_create(memc, &result);

  do {
    rc= memcached_bop_get_by_position(memc, key, strlen(key), order, from, to, &result);
    if (memcached_failed(rc)) {
      fprintf(stderr, "Failed to memcached_bop_get_by_position: %d(%s)\n",
              rc, memcached_strerror(memc, rc));
      break;
    }

    assert(rc == MEMCACHED_SUCCESS);

    for (size_t i=0; i<memcached_coll_result_get_count(&result); i++) {
      const char *value= memcached_coll_result_get_value(&result, i);
      if (result.sub_key_type != MEMCACHED_COLL_QUERY_BOP_EXT) {
        uint64_t bkey= memcached_coll_result_get_bkey(&result, i);
        fprintf(stdout, "memcached_bop_get_by_position: %s : %llu => %s\n",
                key, bkey, value);
      } else {
        memcached_hexadecimal_st *hex_bkey;
        char str_bkey[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
        hex_bkey= memcached_coll_result_get_bkey_ext(&result, i);
        memcached_hexadecimal_to_str(hex_bkey, str_bkey, sizeof(str_bkey));
        fprintf(stdout, "memcached_bop_get_by_position: %s : %s => %s\n",
                key, str_bkey, value);
      }
    }
  } while(0);

  memcached_coll_result_free(&result);
  return (rc == MEMCACHED_SUCCESS) ? 0 : -1;
}
```

## B+Tree 순위와 Element 동시 조회

B+Tree에서 주어진 bkey에 대한 순위를 조회하면서 그 bkey의 element를 포함하여
앞뒤 양방향으로 각 N개의 elements를 함께 조회하는 함수는 아래와 같다.
전자는 8바이트 unsigned integer 타입의 bkey를 사용하는 함수이고,
후자는 최대 31 크기의 byte array 타입의 bkey를 사용하는 함수이다.

```c
memcached_return_t
memcached_bop_find_position_with_get(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     const uint64_t bkey,
                                     memcached_coll_order_t order, size_t count,
                                     memcached_coll_result_st *result)

memcached_return_t
memcached_bop_ext_find_position_with_get(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     const unsigned char *bkey, size_t bkey_length,
                                     memcached_coll_order_t order, size_t count,
                                     memcached_coll_result_st *result)
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
앞뒤 양방향으로 N개 element들을 조회하는 예시는 아래와 같다.

```c
int arcus_btree_find_position_with_get(memcached_st *memc)
{
  const char *key= "btree:a_key";
  const uint64_t bkey= 0;
  // const unsigned char bkey[MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH]= {0, };
  memcached_coll_order_t order= MEMCACHED_COLL_ORDER_ASC;
  size_t count= 10;
  memcached_coll_result_st result;
  memcached_return_t rc;

  memcached_coll_result_create(memc, &result);

  do {
    rc= memcached_bop_find_position_with_get(memc, key, strlen(key),
                                             bkey, order, count, &result);
    /* rc= memcached_bop_ext_find_position_with_get(memc, key, strlen(key),
                                                    bkey, sizeof(bkey),
                                                    order, count, &result); */

    if (memcached_failed(rc)) {
      fprintf(stderr, "Failed to memcached_bop_find_position_with_get: %d(%s)\n",
              rc, memcached_strerror(memc, rc));
      break;
    }

    assert(rc == MEMCACHED_SUCCESS);

    for (size_t i=0; i<memcached_coll_result_get_count(&result); i++) {
      const uint64_t bkey= memcached_coll_result_get_bkey(&result, i);
      /* memcached_hexadecimal_st *hex_bkey;
         char str_bkey[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
         hex_bkey= memcached_coll_result_get_bkey_ext(&result, i);
         memcached_hexadecimal_to_str(hex_bkey, str_bkey, sizeof(str_bkey)); */
      const char *value= memcached_coll_result_get_value(&result, i);
      fprintf(stdout, "memcached_bop_find_position_with_get: "
                      "%s : %llu => %s\n", key, bkey, value);
      /* fprintf(stdout, "memcached_bop_find_position_with_get: "
                         "%s : %s => %s\n", key, str_bkey, value); */
    }
  } while(0);

  memcached_coll_result_free(&result);
  return (rc == MEMCACHED_SUCCESS) ? 0 : -1;
}
```
