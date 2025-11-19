# Item Attributes

Item attributes는 각 cache item의 메타데이터를 의미한다.
Item attributes의 기본 설명은 [ARCUS cache server의 item attributes 부분](https://github.com/naver/arcus-memcached/blob/master/doc/ch03-item-attributes.md)을 참고하길 바란다.

Item attributes를 변경하거나 조회하는 함수들을 설명한다.

- [Attribute 생성](08-attribute-API.md#attribute-%EC%83%9D%EC%84%B1)
- [Attribute 변경](08-attribute-API.md#attribute-%EB%B3%80%EA%B2%BD)
- [Attribute 조회](08-attribute-API.md#attribute-%EC%A1%B0%ED%9A%8C)

## Attribute 생성

Key-value item 생성 시에는 exptime 같은 item 속성 정보를 생성 함수의 인자로 직접 전달한다.

Collection item 생성 시에는 collection item의 다양한 속성 정보를 담아 전달하기 위해 별도의 attributes 구조체를 사용한다.
아래 API를 사용하여 해당 attributes 구조체를 초기화하며, 초기화 시에는 필수 속성 정보인 flags, exptime, maxcount 만을 설정할 수 있다.

``` c
memcached_return_t
memcached_coll_create_attrs_init(memcached_coll_create_attrs_st *attributes,
                                 uint32_t flags, uint32_t exptime, uint32_t maxcount)
```

그 외의 선택적 속성들은 attributes 구조체를 초기화한 후, 아래의 API를 사용하여 개별적으로 지정할 수 있다.

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
- memcached_coll_create_attrs_set_expiretime : attributes에 expiration time을 설정한다.
- memcached_coll_create_attrs_set_maxcount : attributes에 maxcount 값을 설정한다.
- memcached_coll_create_attrs_set_overflowaction : Collection Element 개수가 maxcount를 넘어 섰을 때(overflow)의 동작을 설정한다.
  - OVERFLOWACTION_ERROR : overflow가 발생하면 오류(MEMCACHED_OVERFLOWED)를 반환한다.
  - OVERFLOWACTION_HEAD_TRIM : list collection에서 overflow 발생 시, 가장 작은 index의 element를 삭제한다.
  - OVERFLOWACTION_TAIL_TRIM : list collection에서 overflow 발생 시, 가장 큰 index의 element를 삭제한다.
  - OVERFLOWACTION_SMALLEST_TRIM : b+ree collection에서 overflow 발생 시, 가장 작은 bkey의 element를 삭제한다.
  - OVERFLOWACTION_LARGEST_TRIM : b+ree collection에서 overflow 발생 시, 가장 큰 bkey의 element를 삭제한다.
  - OVERFLOWACTION_SMALLEST_SILENT_TRIM : OVERFLOWACTION_SMALLEST_TRIM과 동일하게 동작하나 trim 발생 여부는 알려주지 않는다.
  - OVERFLOWACTION_LARGEST_SILENT_TRIM : OVERFLOWACTION_LARGEST_TRIM 동일하게 동작하나 trim 발생 여부는 알려주지 않는다.
- memcached_coll_create_set_unreadable : 생성 시 unreadable 상태로 만들 것인지 설정한다.
  Unreadable 상태로 생성된 collection item은 readable 상태가 되기 전까지 조회할 수 없다.
  이렇게 unreadable 상태로 생성된 item을 readable 상태로 만들기 위해서는 Attributes 변경 API를 사용해야 한다.

## Attribute 변경

주어진 key의 attributes를 변경하는 함수이다.

``` c
memcached_return_t
memcached_set_attrs(memcached_st *ptr,
                    const char *key, size_t key_length,
                    const memcached_coll_attrs_st *attrs)
```

Return codes는 다음과 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: Attribute 정보를 성공적으로 변경함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 item이 존재하지 않음.
  - MEMCACHED_ATTR_ERROR_BAD_VALUE: 잘못된 설정값을 지정하였음.

변경할 attributes 정보를 가지는 memcached_coll_attrs_st 구조체는 아래 API를 통해 초기화하고 설정할 수 있다.

``` c
memcached_return_t
memcached_coll_attrs_init(memcached_coll_attrs_st *attrs)
memcached_return_t
memcached_coll_attrs_set_flags(memcached_coll_attrs_st *attrs, uint32_t flags)
memcached_return_t
memcached_coll_attrs_set_expiretime(memcached_coll_attrs_st *attrs, uint32_t expiretime)
memcached_return_t
memcached_coll_attrs_set_overflowaction(memcached_coll_attrs_st *attrs,
                                        memcached_coll_overflowaction_t overflowaction)
memcached_return_t
memcached_coll_attrs_set_maxcount(memcached_coll_attrs_st *attrs, uint32_t maxcount)
memcached_return_t
memcached_coll_attrs_set_maxbkeyrange(memcached_coll_attrs_st *attrs, uint32_t maxbkeyrange)
memcached_return_t
memcached_coll_attrs_set_maxbkeyrange_by_byte(memcached_coll_attrs_st *attrs,
                                              unsigned char *maxbkeyrange, size_t maxbkeyrange_size)
memcached_return_t
memcached_coll_attrs_set_readable(memcached_coll_attrs_st *attrs)
```

- memcached_coll_attrs_init : memcached_coll_attrs_st 구초체를 초기화한다.
- memcached_coll_attrs_set_flags : 변경할 Flag 값을 설정한다.
- memcached_coll_attrs_set_expiretime : 변경할 Expire time 값을 설정한다.
- memcached_coll_attrs_set_overflowaction : 변경할 Overflowaction을 설정한다.
- memcached_coll_attrs_set_maxcount : 변경할 maxcount 값을 설정한다.
- memcached_coll_attrs_set_maxbkeyrange : 변경할 maxbkeyrange를 설정한다. (B+tree에만 적용 가능)
- memcached_coll_attrs_set_maxbkeyrange_by_byte : 변경할 maxbkeyrange를 설정한다. (B+tree에만 적용 가능)
- memcached_coll_attrs_set_readable : Attribute를 Readable 상태로 변경하도록 설정한다.

## Attribute 조회

주어진 key의 attributes를 조회하는 함수이다.

``` c
memcached_return_t
memcached_get_attrs(memcached_st *ptr,
                    const char *key, size_t key_length,
                    memcached_coll_attrs_st *attrs)
```

Response codes는 다음과 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: Attribute 정보를 성공적으로 조회함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 item이 존재하지 않음.

조회한 attributes에서 아래의 함수들을 사용하여 각 attribute를 확인할 수 있다.

``` c
uint32_t
memcached_coll_attrs_get_flags(memcached_coll_attrs_st *attrs)
uint32_t
memcached_coll_attrs_get_expiretime(memcached_coll_attrs_st *attrs)
memcached_coll_overflowaction_t
memcached_coll_attrs_get_overflowaction(memcached_coll_attrs_st *attrs)
uint32_t
memcached_coll_attrs_get_maxcount(memcached_coll_attrs_st *attrs)
bool
memcached_cool_attrs_is_readable(memcached_coll_attrs_st *attrs)
uint32_t
memcached_coll_attrs_get_maxbkeyrange(memcached_coll_attrs_st *attrs)
memcached_return_t
memcached_coll_attrs_get_maxbkeyrange_by_byte(memcached_coll_attrs_st *attrs,
                                              unsigned char **maxbkeyrange,
                                              size_t maxbkeyrange_size)
uint32_t
memcached_coll_attrs_get_minbkey(memcached_coll_attrs_st *attrs)
memcached_return_t
memcached_coll_attrs_get_minbkey_by_byte(memcached_coll_attrs_st *attrs,
                                         unsigned char **bkey, size_t *size)
uint32_t
memcached_coll_attrs_get_maxbkey(memcached_coll_attrs_st *attrs)
memcached_return_t
memcached_coll_attrs_get_maxbkey_by_byte(memcached_coll_attrs_st *attrs,
                                         unsigned char **bkey, size_t *size)
uint32_t
memcached_coll_attrs_get_trimmed(memcached_coll_attrs_st *attrs)
```

- memcached_coll_attrs_get_flags : Flag 값을 얻는다.
- memcached_coll_attrs_get_expiretime : Expire time 값을 얻는다.
- memcached_coll_attrs_get_overflowaction : Overflowaction을 얻는다.
- memcached_coll_attrs_get_maxcount : Maxcount 값을 얻는다.
- memcached_cool_attrs_is_readable : Attribute가 readable 상태인지 아닌지를 얻는다.
- memcached_coll_attrs_get_maxbkeyrange : Maxbkeyrange 값을 얻는다.
- memcached_coll_attrs_get_maxbkeyrange_by_byte : Maxbkeyrange 값을 얻는다. (byte 타입)
- memcached_coll_attrs_get_minbkey : Min bkey 값을 얻는다.
- memcached_coll_attrs_get_minbkey_by_byte : Min bkey 값을 얻는다. (byte 타입)
- memcached_coll_attrs_get_maxbkey : Max bkey 값을 얻는다.
- memcached_coll_attrs_get_maxbkey_by_byte : Max bkey 값을 얻는다. (byte 타입)
- memcached_coll_attrs_get_trimmed : btree의 trimmed 여부를 얻는다.
