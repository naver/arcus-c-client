# ARCUS C Client

ARCUS client는 ARCUS admin과 ARCUS cache server군 들과의 연결을 유지하며 client로 들어온 명령을 처리하여 그 결과를 반환한다

ARCUS C client는 C/C++ 개발환경에서 ARCUS를 사용하기 위한 라이브러리로서,
대표적인 memcached C client인 [libmemcached](https://code.launchpad.net/libmemcached)를 기반으로 개발하였다.
따라서 libmemcached의 기능을 대부분 사용할 수 있으며,
ARCUS cache server에서 제공하는 failover 기능과 collection 기능 등을 추가로 지원한다.

지원되는 추가 기능은 다음과 같다.

* Cache Cluster-awareness: ARCUS admin 서버에 연결하여 자신이 사용하는 캐시 서버 클러스터의 변경사항을 자동으로 인식한다.
* Collection APIs: ARCUS cache server에서 지원하는 List, Set, B+tree 형태의 자료구조를 사용할 수 있다.
* Prefix: 특정 prefix를 가지는 모든 item을 삭제할 수 있다.

아래의 순서로 ARCUS C Client 사용법을 설명한다.

- [서버 모델에 따른 초기화](02-arcus-c-client.md#%EC%84%9C%EB%B2%84-%EB%AA%A8%EB%8D%B8%EC%97%90-%EB%94%B0%EB%A5%B8-%EC%B4%88%EA%B8%B0%ED%99%94)
- [Client 설정과 사용](02-arcus-c-client.md#client-%EC%84%A4%EC%A0%95%EA%B3%BC-%EC%82%AC%EC%9A%A9)

## 서버 모델에 따른 초기화

서버 모델에 따른 초기화 메소드는 아래와 같다.

- Single-Threaded

  ```c
  arcus_return_t arcus_connect(memcached_st *mc, const char *ensemble_list, const char *svc_code)
  ```
  싱글 스레드 서버에서 ARCUS에 연결하기 위해 사용한다.

- Multi-Threaded

  ```c
  arcus_return_t arcus_pool_connect(memcached_pool_st *pool, const char *ensemble_list, const char *svc_code)
  ```

  멀티 스레드 서버에서 ARCUS에 연결하기 위해 사용한다.

- Multi-Process

  ```c
  arcus_return_t arcus_proxy_create(memcached_st *mc, const char *ensemble_list, const char *svc_code)
  arcus_return_t arcus_proxy_connect(memcached_st *mc, memcached_pool_st *pool, memcached_st *proxy)
  ```

  `arcus_proxy_create` 함수는
  멀티 프로세스 서버의 부모 프로세스가 ARCUS에 연결한 뒤, 자식 프로세스들이 사용할 proxy를 생성하기 위해 사용한다.
  `arcus_proxy_connect` 함수는
  멀티 프로세스 서버의 자식 프로세스에서 부모 프로세스가 생성한 proxy에 연결하기 위해 사용한다.
  참고 사항으로, 멀티 프로세스 서버이지만 각 자식 프로세스가 멀티 쓰레드로 동작하는 경우에는
  pool을 생성하여 사용할 수 있다.

ARCUS C client는 서비스에서 채용한 서버 모델에 따라 다양한 초기화 API를 제공한다.
초기화 API는 ARCUS admin에 접속하여 주어진 서비스코드에 해당하는 ARCUS cache server 리스트를 가져와서,
consistent hashing을 위한 초기화 작업을 수행한다.

초기화 API에서 공통적으로 사용되는 파라미터의 의미는 다음과 같다.

* ensemble_list : ARCUS admin의 주소.
* svc_code : 부여 받은 서비스코드.

### Multi-Threaded Example

많은 서비스에서 사용되는 Multi-threaded 서버에서는 다음과 같이 초기화 할 수 있다.

```c
#include "libmemcached/memcached.h"

int main(int argc, char** argv)
{
    int initial = 4;
    int max = 16;

    memcached_st *master_mc = NULL;
    memcached_pool_st *pool = NULL;

    // 1. Pool을 구성하기 위한 기준이 될 memcached_st 구조체를 생성한다.
    master_mc = memcached_create(NULL);

    // 2. Pool을 구성한다. 기준 memcached_st 구조체의 포인터와 pool의 초기 및 최대 크기를 입력한다.
    pool = memcached_pool_create(master_mc, initial, max);

    // 3. ARCUS admin에 연결한다.
    arcus_return_t error = arcus_pool_connect(pool, "dev.arcuscloud.nhncorp.com:17288", "dev");

    if (error != ARCUS_SUCCESS) {
        fprintf(stderr, "arcus_connect() failed, reason=%s\n", arcus_strerror(error));
        exit(1);
    }

    // 생성된 자원을 반환한다.
    arcus_pool_close(pool);
    memcached_pool_destroy(pool);
    memcached_free(master_mc);

    return EXIT_SUCCESS;
}
```

위 코드는 Multi-threaded 서버에서 ARCUS를 사용하기 위해 memcached_st 구조체에 대한 pool을 구성한다.
memcached_st 구조체는 ARCUS cache server 연결 정보 및 각종 설정이 포함된 기본 자료구조로서 모든 캐시 요청 API에서 사용된다.
완전한 예제는 소스 패키지에 포함된 arcus/multi_threaded.c를 참고하기 바란다.

### Multi-Process Example

일부 서비스에서는 Apache와 비슷한 프로세스 prefork 모델을 이용하기도 한다.
이 같은 멀티 프로세스 방식의 서버에서 ARCUS C client를 초기화 하는 방법은 다음과 같다.

```c
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>

#include "libmemcached/memcached.h"

#define NUM_OF_CHILDREN 10
#define NUM_OF_WORKERS 10

/* 각 자식 프로세스에서 동작하는 쓰레드 */
static void *my_app_thread(void *ctx_pool)
{
    memcached_pool_st *pool = (memcached_pool_st *)ctx_pool;
    memcached_st *mc;
    memcached_return_t rc;

    int count = 0;

    while (count++ < 10000) {
        struct timespec wait = { 0, 0 };
        // pool에서 memcached_st 구조체 하나를 꺼내온다.
        mc = memcached_pool_fetch(pool, &wait, &rc);

        if (mc) {
            char key[256];
            uint64_t value = 100;

            snprintf(key, 100, "test:kv_%d", getpid());
            rc = memcached_set(mc, key, strlen(key), (char *)&value, sizeof(value), 600, 0);
            if (rc != MEMCACHED_SUCCESS) {
                fprintf(stderr, "memcached_set: %s", memcached_detail_error_message(mc, rc));
            }
        }

        // pool에 memcached_st 구조체를 반환한다.
        rc = memcached_pool_release(pool, mc);
        if (rc != MEMCACHED_SUCCESS) {
            fprintf(stderr, "memcached_pool_release: %s\n", memcached_strerror(NULL, rc));
        }
    }

    fprintf(stderr, "[pid:%d] done\n", getpid());
}

/* 자식 프로세스 */
static inline void process_child(memcached_st *proxy_mc)
{
    fprintf(stderr, "[pid:%d] begin : child_process\n", getpid());

    // 자식 프로세스가 사용할 memcached_st 구조체를 생성한다.
    memcached_st *per_child_mc = memcached_create(NULL);

    // 자식 프로세스가 멀티 쓰레드로 동작한다면 memcached_st 구조체에 대한 pool을 생성한다.
    memcached_pool_st *pool = memcached_pool_create(per_child_mc, NUM_OF_WORKERS/2, NUM_OF_WORKERS);
  // 부모 프로세스의 memcached_st 구조체를 이용하여 캐시 서버 리스트를 업데이트 받는다.
    arcus_proxy_connect(per_child_mc, pool, proxy_mc);

    if (!pool) {
        fprintf(stderr, "memcached_pool_create: failed\n");
        goto RETURN;
    }

    pthread_t tid[NUM_OF_WORKERS];

    for (int id=0; id<NUM_OF_WORKERS; id++) {
        pthread_create(&tid[id], NULL, my_app_thread, pool);
    }

    for (int id=0; id<NUM_OF_WORKERS; id++) {
        pthread_join(tid[id], NULL);
    }

    memcached_pool_destroy(pool);
    memcached_free(per_child_mc);

RETURN:
    fprintf(stderr, "[pid:%d] end : child_process\n", getpid());
    arcus_proxy_close(per_child_mc);
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    memcached_st *proxy_mc;
    arcus_return_t rc;

    int i;

    // 부모 프로세스가 사용할 memcached_st 구조체를 생성한다.
    proxy_mc = memcached_create(NULL);

    // ARCUS admin과 연결을 유지하는 쓰레드를 생성하여 캐시 서버 정보를 업데이트 받는다.
    rc = arcus_proxy_create(proxy_mc, "dev.arcuscloud.nhncorp.com:17288", "test1_6");

    if (rc != ARCUS_SUCCESS) {
        goto RELEASE;
    }

    pid_t pid;

    // 자식 프로세스를 fork 한다.
    for (i=0; i<NUM_OF_CHILDREN; i++) {
        pid = fork();

        switch (pid) {
            case 0:
                process_child(proxy_mc);
                exit(EXIT_SUCCESS);
            case -1:
                perror("fork error");
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }

    // 자식 프로세스를 기다린다.
    siginfo_t info;
    waitid(P_ALL, 0, &info, WEXITED | WSTOPPED | WCONTINUED);
    //sleep(20);

RELEASE:
    arcus_proxy_close(proxy_mc);
    memcached_free(proxy_mc);

    return EXIT_SUCCESS;
}
```

위 코드는 멀티 프로세스 서버에서 ARCUS를 사용하기 위한 기본 초기화 방법이다.
부모 프로세스에서 ARCUS admin과 연결을 유지하는 쓰레드를 생성하여 캐시 서버 리스트의 변경 사항을 업데이트 받도록 하고,
자식 프로세스에서는 부모의 memcached_st 구조체를 이용하여 ARCUS admin과의 연결 없이 캐시 서버 리스트를 얻어 온다.
특히, 각 자식 프로세스가 내부적으로 멀티 쓰레드로 동작하는 상황에서 pool을 사용하는 방법도 확인할 수 있다.

## Client 설정과 사용

### 로그 남기기

ARCUS C client는 ARCUS admin과의 연결 상태 및 ARCUS cache server 리스트의 변경 사항에 대해 로그를 남긴다.
로그는 ZooKeeper client에 내장된 로깅 API를 사용하고 있으며 기본적으로 표준 에러(stderr)로 출력된다.
ARCUS cache server 리스트 변경에 대한 로그는 문제 상황 발생 시 귀중한 힌트가 될 수 있으므로
별도의 파일로 남기는 것을 추천한다.

만약 표준 에러를 파일로 남기기 힘들거나 다른 로그 파일과 분리하여 기록하고 싶은 경우에는 다음과 같이 설정할 수 있다.

``` c
void arcus_set_log_stream(memcached_st *mc, FILE *logfile);
```

ARCUS 관련 로그를 기록하기 위한 FILE stream을 지정한다.
위 API는 아래와 같이 memcached_st 구조체 생성 코드 바로 다음에 추가한다.

``` c
mc = memcached_create(NULL);
arcus_set_log_stream(mc, logfile);
```

Operation 수행 중 발생하는 오류는 ARCUS C client 내부에서 보관하며,
서비스의 로깅 시스템을 통해 출력할 수 있도록 아래와 같은 API를 제공한다.

``` c
const char *memcached_strerror(memcached_st *, memcached_return_t rc);
const char *memcached_last_error_message(memcached_st *mc);
const char *memcached_detail_error_message(memcached_st *mc, memcached_return_t rc);
```

  - `memcached_strerror` : memcached return code인 rc에 대응하는 return message 출력을 위해 사용한다.
    - 출력 형식은 `<rc string>` 형태이다.
    - `memcached_st *` 인자는 사용되지 않는 값이며, NULL로 설정해 사용한다.
  - `memcached_last_error_message` : 하나의 operation 수행 중 마지막으로 발생한 error message 출력을 위해 사용한다.
  - `memcached_detail_error_message` : 하나의 operation 수행 중 발생한 모든 error message 출력을 위해 사용한다.
    - 출력 형식은 `<time> <mc_id> <mc_qid> <er_qid> <error message>` 형태이다.
    - time : error message가 생성된 시각
    - mc_id : 서로다른 mc를 구분하기 위해 사용하는 구분자로, mc 생성마다 1 씩 증가하는 값
    - mc_qid : mc가 현재 수행하는 operation의 query id
    - er_qid : error message를 생성한 operation의 query id
    - error message : 실제 오류 원인이 되는 error message string

위 API는 아래와 같은 방법으로 사용이 가능하며,
정확한 오류 출력을 위해 `memcached_detail_error_message(memcached_st *mc, memcached_return_t rc)`의 사용을 추천한다.

``` c
rc = memcached_set(mc, key, strlen(key), (char *)&value, sizeof(value), 600, 0);
if (rc != MEMCACHED_SUCCESS) {
    fprintf(stderr, "memcached_set: %s", memcached_detail_error_message(mc, rc));
}
```

### 캐시 명령에 대한 OPERATION TIMEOUT 지정

캐시 명령을 보내고 응답을 받기까지의 timeout 시간을 지정할 수 있다.

``` c
mc = memcached_create(NULL);
memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, (uint64_t)timeout);
```

timeout 시간은 밀리초(ms) 단위이며, 기본 값은 MEMCACHED_DEFAULT_TIMEOUT (500ms) 이다.

### 캐시 노드에 대한 CONNECTION TIMEOUT 지정

캐시연결이 끊어진 후 재연결 요청 시의 timeout 시간을 지정할 수 있다.

``` c
mc = memcached_create(NULL);
memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, (uint64_t)timeout);
```

timeout 시간은 밀리초(ms) 단위이며, 기본 값은 MEMCACHED_DEFAULT_CONNECT_TIMEOUT (1000ms) 이다.

connection timeout이 발생하면 RETRY_TIMEOUT 시간 후에 해당 캐시 노드로 재연결을 시도한다.
RETRY_TIMEOUT이 0이면, connection timeout이 발생할 때마다 즉시 재연결을 시도한다.
이러한 RETRY_TIMEOUT 시간을 지정할 수 있다.

``` c
mc = memcached_create(NULL);
memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, (uint64_t)timeout);
```
timeout 시간은 초(s) 단위이며, 기본 값은 MEMCACHED_SERVER_FAILURE_RETRY_TIMEOUT (1초) 이다.

참고 사항으로, 재연결 시도는 무한히 반복한다. 만약 해당 캐시 노드가 failure 상태라면,
ARCUS의 admin인 ZooKeeper에 의해 failed 캐시 노드로 감지되어 cache node list에서 제거되어,
그 캐시 노드로의 재연결 요청은 중단되게 된다.

그리고, 정상적으로 연결되지 않은 캐시 노드로의 요청에 대해서는
MEMCACHED_SERVER_TEMPORARILY_DISABLED (“SERVER HAS FAILED AND IS DISABLED UNTIL TIMED RETRY”) 오류가 발생한다.

### 캐시 API의 응답코드 확인

캐시 명령을 실행한 후에 캐시 서버로부터 받은 응답 코드를 확인할 수 있다.
이 응답코드는 명령의 실행 결과에 대한 추가 정보를 제공한다.

``` c
memcached_return_t res = memcached_get_last_response_code(mc);
```

확인 가능한 응답 코드는 각 API 설명에 포함되어 있으며 다음과 같은 형식으로 표현되어 있다.

- Response Codes
  - MEMCACHED_SUCCESS (API의 리턴값이 MEMCACHED_SUCCESS 인 경우. 즉 API가 성공한 경우)
    - MEMCACHED_STORED (가능한 응답코드)
  - not MEMCACHED_SUCCESS (API의 리턴값이 MEMCACHED_SUCCESS가 아닌 경우, 즉 API가 실패한 경우)
    - MEMCACHED_NOTFOUND (가능한 응답코드)


