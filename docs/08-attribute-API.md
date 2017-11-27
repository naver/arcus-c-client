## Item Attributes

Item attributes는 각 cache item의 메타데이터를 의미한다.
Item attributes의 기본 설명은 [Arcus cache server의 item attributes 부분](https://github.com/naver/arcus-memcached/blob/master/doc/arcus-item-attribute.md)을 참고하길 바란다.

Item attributes를 변경하거나 조회하는 함수들을 설명한다.

- [Attribute 변경](08-attribute-API.md#attribute-%EB%B3%80%EA%B2%BD)
- [Attribute 조회](08-attribute-API.md#attribute-%EC%A1%B0%ED%9A%8C)


### Attribute 변경

주어진 key의 attributes를 변경하는 함수이다.

``` c
memcached_return_t memcached_set_attrs(memcached_st *ptr, const char *key, size_t key_length,
const memcached_coll_attrs_st *attrs);
```

Return codes는 다음과 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: Attribute 정보를 성공적으로 변경함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 item이 존재하지 않음.
  - MEMCACHED_ATTR_ERROR_BAD_VALUE: 잘못된 설정값을 지정하였음.


변경할 attributes 정보를 가지는 memcached_coll_attrs_st 구조체는 아래 API를 통해 초기화하고 설정할 수 있다.

``` c
memcached_return_t memcached_coll_attrs_init(memcached_coll_attrs_st *attrs);
memcached_return_t memcached_coll_attrs_set_flags(memcached_coll_attrs_st *attrs, uint32_t flags);
memcached_return_t memcached_coll_attrs_set_expiretime(memcached_coll_attrs_st *attrs, uint32_t expiretime);
memcached_return_t memcached_coll_attrs_set_overflowaction(memcached_coll_attrs_st *attrs,
                                            memcached_coll_overflowaction_t overflowaction);
memcached_return_t memcached_coll_attrs_set_maxcount(memcached_coll_attrs_st *attrs, uint32_t maxcount);
memcached_return_t memcached_coll_attrs_set_maxbkeyrange(memcached_coll_attrs_st *attrs, uint32_t maxbkeyrange);
memcached_return_t memcached_coll_attrs_set_maxbkeyrange_by_byte(memcached_coll_attrs_st *attrs,
                                            unsigned char *maxbkeyrange, size_t maxbkeyrange_size);
memcached_return_t memcached_coll_attrs_set_readable(memcached_coll_attrs_st *attrs);
```
- memcached_coll_attrs_init : memcached_coll_sttrs_st 구초체를 초기화한다.
- memcached_coll_attrs_set_flags : 변경할 Flag 값을 설정한다.
- memcached_coll_attrs_set_expiretime : 변경할 Expire time 값을 설정한다.
- memcached_coll_attrs_set_overflowaction : 변경할 Overflowaction을 설정한다.
- memcached_coll_attrs_set_maxcount : 변경할 maxcount 값을 설정한다.
- memcached_coll_attrs_set_maxbkeyrange : 변경할 maxbkeyrange를 설정한다. (B+tree에만 적용 가능)
- memcached_coll_attrs_set_maxbkeyrange_by_byte : 변경할 maxbkeyrange를 설정한다. (B+tree에만 적용 가능)
- memcached_coll_attrs_set_readable : Attribute를 Readable 상태로 변경하도록 설정한다.


### Attribute 조회

주어진 key의 attributes를 조회하는 함수이다.

``` c
memcached_return_t memcached_get_attrs(memcached_st *ptr, const char *key, size_t key_length, memcached_coll_attrs_st *attrs);
```

Response codes는 다음과 같다.

- MEMCACHED_SUCCESS
  - MEMCACHED_END: Attribute 정보를 성공적으로 조회함.
- not MEMCACHED_SUCCESS
  - MEMCACHED_NOTFOUND: 주어진 key에 해당하는 item이 존재하지 않음.

조회한 attributes에서 아래의 함수들을 사용하여 각 attribute를 확인할 수 있다.

``` c
uint32_t                        memcached_coll_attrs_get_flags(memcached_coll_attrs_st *attrs);
uint32_t                        memcached_coll_attrs_get_expiretime(memcached_coll_attrs_st *attrs);
memcached_coll_overflowaction_t memcached_coll_attrs_get_overflowaction(memcached_coll_attrs_st *attrs);
uint32_t                        memcached_coll_attrs_get_maxcount(memcached_coll_attrs_st *attrs);
bool                            memcached_cool_attrs_is_readable(memcached_coll_attrs_st *attrs);
uint32_t                        memcached_coll_attrs_get_maxbkeyrange(memcached_coll_attrs_st *attrs);
memcached_return_t              memcached_coll_attrs_get_maxbkeyrange_by_byte(memcached_coll_attrs_st *attrs,
                                                         unsigned char **maxbkeyrange, size_t maxbkeyrange_size);
uint32_t                        memcached_coll_attrs_get_minbkey(memcached_coll_attrs_st *attrs);
memcached_return_t              memcached_coll_attrs_get_minbkey_by_byte(memcached_coll_attrs_st *attrs,
                                                         unsigned char **bkey, size_t *size);
uint32_t                        memcached_coll_attrs_get_maxbkey(memcached_coll_attrs_st *attrs);
memcached_return_t              memcached_coll_attrs_get_maxbkey_by_byte(memcached_coll_attrs_st *attrs,
                                                         unsigned char **bkey, size_t *size);
uint32_t                        memcached_coll_attrs_get_trimmed(memcached_coll_attrs_st *attrs);
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


