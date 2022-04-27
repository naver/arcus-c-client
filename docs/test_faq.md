## Test
- 전체 테스트를 수행하는 방법
    ```
    make test
    ```
- 개별 테스트를 수행하는 방법
    ```
    make check TESTS='testname'
    ```
    - 예시
        ```
        make check TESTS='libtest/unittest' // 1개 테스트 수행
        make check TESTS='libtest/unittest tests/cycle' // 2개 테스트 수행
        ```
- 테스트 실패 케이스
    - 예시 1
        ```
        make check TESTS='libtest/unittest tests/testplus tests/failure'

        CXXLD    libtest/unittest
        FAIL: libtest/unittest
        CXXLD    tests/testplus
        FAIL: tests/testplus
        CXXLD    tests/failure
        FAIL: tests/failure

        # TOTAL: 3
        # PASS:  0
        # SKIP:  0
        # XFAIL: 0
        # FAIL:  3
        # XPASS: 0
        # ERROR: 0
        ```
        - Test를 수행하는 머신에 11211 포트를 사용하는 memcached 노드가 실행 중일 때 발생
        - libtest/unittest와 tests/failure에서 11211 포트를 사용하는 memcached 노드를 kill 시도하는데, kill이 실패하면 테스트 실패
        - tests/testplus에서 11211 포트를 사용하는 memcached 노드가 띄워져 있지 않다는 전제 하에 연산 수행 실패 결과를 받아와야 하는데, 해당 노드가 띄워져 있으면 연산 수행 성공 결과를 받아와 테스트 실패
