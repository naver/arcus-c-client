## Appendix

### 문제 해결

32-bit 환경에서는 ./configure 옵션에 다음을 추가한다.

```
--disable-64bit CFLAGS="-O2 -march=i686"
```

GCC3, GCC4가 함께 설치된 환경에서는 ./configure 옵션에 다음을 추가한다.

```
CC=gcc4 CXX=g++4
```

멀티프로세스 샘플($SRC/arcus/multi_process)이 "Cannot create proxy lock : No space left on device" 메시지와 함께 실행되지 않는다면 다음 명령을 실행한다. USERID는 사용자 계정으로 대치한다.

```
$ for i in `ipcs -s | awk '/USERID/ {print $2}'`; do (ipcrm -s $i); done
```

### 설치 확인 : 샘플 프로그램을 정적 링크하기

(sample applications can be found in $SRC/arcus)

```
$ g++ multi_threaded.cc /usr/lib64/libm.a /usr/local/lib/libmemcached.a /usr/local/lib/libmemcachedutil.a /usr/local/lib/libzookeeper_mt.a /usr/local/lib/libapr-1.a \
   -o multi_threaded -I. -I/usr/local/include -I/usr/local/include/c-client-src -pthread --static
```
