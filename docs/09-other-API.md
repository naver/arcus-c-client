# Other API

본 절에서는 아래의 나머지 API들을 설명한다.

- [Flush](09-other-API.md#flush)


## Flush

Arcus는 cache server에 있는 모든 items 또는 특정 prefix의 items을 flush(or delete)하는 기능을 제공한다.
전자의 함수는 모든 items을 flush하고 후자의 함수는 특정 prefix의 items을 flush한다.

```C
memcached_return_t memcached_flush(memcached_st *ptr, time_t expiration);
memcached_return_t memcached_flush_by_prefix(memcached_st *ptr,
                                             const char *prefix, size_t prefix_length,
                                             time_t expiration);

```

- prefix, prefix_length: flush할 prefix 정보
- expiration: delayed flush할 시에 지연할 시간(단위: 초)를 나타낸다.

**특정 prefix의 모든 items을 삭제하므로 그 사용에 주의하여야 한다.**
**특히, prefix를 명시하지 않는 flush 함수는 cache node의 모든 items을 삭제하므로 공용으로 사용하는 cloud에선 각별히 주의해야 한다.**

