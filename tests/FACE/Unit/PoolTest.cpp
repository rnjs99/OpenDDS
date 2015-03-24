#include "dds/DCPS/SafetyProfilePool.h"
#include "ace/Log_Msg.h"

#include <string.h>
#include <iostream>

unsigned int assertions = 0;
unsigned int failed = 0;

#define TEST_CHECK(COND) \
  ++assertions; \
  if (!( COND )) {\
    ++failed; \
    ACE_ERROR((LM_ERROR,"(%P|%t) TEST_CHECK(%C) FAILED at %N:%l %a\n",\
        #COND , -1)); \
  }

using namespace OpenDDS::DCPS;

class PoolTest {
public:
  void test_pool_alloc() {
    Pool pool(1024, 64);
    char* ptr = pool.pool_alloc(128);

    TEST_CHECK(ptr);
    validate_pool(pool, 128);
  }

  void test_pool_allocs() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(128);
    validate_pool(pool, 128);
    char* ptr1 = pool.pool_alloc(256);
    validate_pool(pool, 384);
    char* ptr2 = pool.pool_alloc(128);
    validate_pool(pool, 512);

    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    TEST_CHECK(ptr0 > ptr1);
    TEST_CHECK(ptr1 > ptr2);
  }

  void test_pool_alloc_last() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(128);
    validate_pool(pool, 128);
    char* ptr1 = pool.pool_alloc(256);
    validate_pool(pool, 384);
    char* ptr2 = pool.pool_alloc(128);
    validate_pool(pool, 512);
    char* ptr3 = pool.pool_alloc(512);
    validate_pool(pool, 1024);

    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    TEST_CHECK(ptr3);
    TEST_CHECK(ptr0 > ptr1);
    TEST_CHECK(ptr1 > ptr2);
    TEST_CHECK(ptr2 > ptr3);
  }
private:
  void validate_pool(Pool& pool, size_t expected_allocated_bytes) {
    char* prev = 0;
    char* prev_end = 0;
    size_t allocated_bytes = 0;
    size_t free_bytes = 0;
    bool prev_free;
    // Check all allocs in positional order and not overlapping
    for (unsigned int i = 0; i < pool.allocs_in_use_; ++i) {
      PoolAllocation* alloc = pool.allocs_ + i;
//printf("At index %d, ptr is %x, size %d free %d\n", i, alloc->ptr(), alloc->size(), alloc->free_);
      TEST_CHECK(prev < alloc->ptr());
      if (prev_end) {
        TEST_CHECK(prev_end == alloc->ptr());
        // Validate not consecutive free blocks
        TEST_CHECK(!(prev_free && alloc->free_));
      }
      prev_end = alloc->ptr() + alloc->size();
      prev_free = alloc->free_;

      if (!alloc->free_) {
        allocated_bytes += alloc->size();
      } else {
        free_bytes += alloc->size();
      }
    }

    TEST_CHECK(allocated_bytes == expected_allocated_bytes);
    TEST_CHECK(allocated_bytes + free_bytes == pool.pool_size_);

    size_t prev_size = 0;

    // Check all free blocks in size order
    for (PoolAllocation* free_alloc = pool.first_free_;
         free_alloc;
         free_alloc = free_alloc->next_free_) {
      // If not the first alloc
      if (free_alloc != pool.first_free_) {
        TEST_CHECK(free_alloc->size() <= prev_size);
      }
      prev_size = free_alloc->size();
    }
  }
};

int main(int, const char** )
{
  PoolTest test;

  test.test_pool_alloc();
  test.test_pool_allocs();
  test.test_pool_alloc_last();

  printf("%d assertions failed, %d passed\n", failed, assertions - failed);

  if (failed) {
    printf("test FAILED\n");
  } else {
    printf("test PASSED\n");
  }
  return failed;
}

