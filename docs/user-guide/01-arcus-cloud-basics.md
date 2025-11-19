# ARCUS Cloud 기본 사항

ARCUS는 확장된 key-value 데이터 모델을 제공한다.
하나의 key는 하나의 데이터만을 가지는 simple key-value 유형 외에도
하나의 key가 여러 데이터를 구조화된 형태로 저장하는 collection 유형을 제공한다.

ARCUS cache server의 key-value 모델은 아래의 기본 제약 사항을 가진다.

- 기존 key-value 모델의 제약 사항
  - Key의 최대 크기는 4000 character이다.
  - Value의 최대 크기는 1MB이다.
- Collection 제약 사항
  - 하나의 collection이 가지는 최대 element 개수는 50,000개이다.
  - Collection element가 저장하는 value의 최대 크기는 16KB이다.


아래에서 ARCUS cloud를 이해하는 데 있어 기본 사항들을 기술한다.

- [서비스코드](01-arcus-cloud-basics.md#%EC%84%9C%EB%B9%84%EC%8A%A4%EC%BD%94%EB%93%9C)
- [ARCUS Admin](01-arcus-cloud-basics.md#arcus-admin)
- [Cache Key](01-arcus-cloud-basics.md#cache-key)
- [Cache Item](01-arcus-cloud-basics.md#cache-item)
- [Expiration](01-arcus-cloud-basics.md#expiration)
- [Eviction](01-arcus-cloud-basics.md#eviction)
- [Value Flags](01-arcus-cloud-basics.md#value-flags)


## 서비스코드

서비스코드(service code)는 ARCUS에서 cache cloud를 구분하는 코드이다.
ARCUS cache cloud 서비스를 응용들에게 제공한다는 의미에서 "서비스코드"라는 용어를 사용하게 되었다.

하나의 응용에서 하나 이상의 ARCUS cache cloud를 구축하여 사용할 수 있다.
ARCUS java client 객체는 하나의 ARCUS 서비스코드만을 가지며, 하나의 ARCUS cache cloud에만 접근할 수 있다.
해당 응용이 둘 이상의 ARCUS cache cloud에 접근해야 한다면,
각 ARCUS cache cloud의 서비스코드를 가지는 ARCUS java client 객체를 따로 생성하여 사용하여야 한다.

## ARCUS Admin

ARCUS admin은 ZooKeeper를 이용하여 각 서비스 코드에 해당하는 ARCUS cache cloud를 관리한다.
특정 서비스 코드에 대한 cache server list를 관리하며,
cache server 추가 및 삭제에 대해 cache server list를 최신 상태로 유지하며,
서비스 코드에 대한 cache server list 정보를 ARCUS client에게 전달한다.
ARCUS admin은 highly available하여야 하므로,
여러 ZooKeeper 서버들을 하나의 ZeeKeeper ensemble로 구성하여 사용한다.

## Cache Key

Cache key는 ARCUS cache에 저장하는 cache item을 유일하게 식별한다. Cache key 형식은 아래와 같다.

```
  Cache Key : [<prefix>:]<subkey>
```

- \<prefix\> - Cache key의 앞에 붙는 namespace이다.
  - Prefix 단위로 cache server에 저장된 key들을 그룹화하여 flush하거나 통계 정보를 볼 수 있다.
  - Prefix를 생략할 수 있지만, 가급적 사용하길 권한다.
- delimiter - Prefix와 subkey를 구분하는 문자로 default delimiter는 콜론(‘:’)이다.
- \<subkey\> - 일반적으로 응용에서 사용하는 Key이다.

Prefix와 subkey는 아래의 명명 규칙을 가진다.

- Prefix는 영문 대소문자, 숫자, 언더바(_), 하이픈(-), 플러스(+), 점(.) 문자만으로 구성될 수 있으며,
  이 중에 하이픈(-)은 prefix 명의 첫번째 문자로 올 수 없다.
- Subkey는 공백을 포함할 수 없으며, 기본적으로 alphanumeric만을 사용하길 권장한다.

## Cache Item

ARCUS cache는 simple key-value item 외에 다양한 collection item 유형을 가진다.

- simple key-value item - 기존 key-value item
- collection item
  - list item - 데이터들의 linked list을 가지는 item
  - set item - 유일한 데이터들의 집합을 가지는 item
  - map item - \<mkey, value\>쌍으로 구성된 데이터 집합을 가지는 item
  - b+tree item - b+tree key 기반으로 정렬된 데이터 집합을 가지는 item

## Expiration

각 cache item은 expiration time 속성을 가지며, 자동으로 만료할 시간을 나타낸다.
Expiration time은 다음과 같이 지정할 수 있다.

- expiration time = 0
  - 해당 아이템은 만료되지 않는다.
- 0 < expiration time ≤ (30 * 24 * 60 * 60)초 /* 30일 */
  - 현재 시각으로부터 지정된 초만큼 뒤의 시각을 expiration time으로 설정한다.
- expiration time > (30 * 24 * 60 * 60)초 /* 30일 */
  - 주어진 값을 unix time으로 해석하여 expiration time을 설정한다.
  - 만약 unix time이 현재 시각보다 이전이면 즉시 expire되므로 주의해야 한다.

## Eviction

ARCUS cache는 memory cache이며, 한정된 메모리 공간을 사용하여 데이터를 caching한다.
메모리 공간이 모두 사용된 상태에서 새로운 cache item 저장 요청이 들어오면,
ARCUS cache는 "out of memory" 오류를 내거나
LRU(least recently used) 기반으로 오랫동안 접근되지 않은 cache item을 evict시켜
available 메모리 공간을 확보한 후에 새로운 cache item을 저장한다.

## Value Flags

각 cache item은 value와 함께 저장하거나 조회하는 정수형의 flags 값을 가진다.
Flags 값은 주로 value에 관한 정보를 나타내는 용도이며, 예시로 value에 저장된 데이터의 유형이나 압축(compression) 여부 등을 나타내는 비트 필드로 활용할 수 있다.

주의 사항으로, Cache Item 타입에 따라 flags를 저장하거나 조회하는 방식이 아래와 같이 다르다.

- Key-Value Item에 관한 API에서는 저장 시에 value와 함께 flags 값을 저장하고, 조회 시에 value와 함께 flags 값을 조회한다.
- Collection Item에 관한 API에서는 collection item 생성 시에 collection attributes에 지정한 flags 값을 저장하고, elements 조회 시에 result 구조체에서 flags 값을 조회한다.
