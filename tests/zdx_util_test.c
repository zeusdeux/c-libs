#include <stdint.h>

#include "../zdx_util.h"
#include "../zdx_test_utils.h"

int main(void)
{
  {
    testlog(L_INFO, "Testing CLOSESTPOWEROF2 macro for uint32_t values");

    uint32_t p0 = CLOSESTPOWEROF2((uint32_t)0);
    assertm(p0 == 0, "Expected: %u, Received: %u", (uint32_t)0, p0);

    uint32_t p1 = CLOSESTPOWEROF2((uint32_t)1);
    assertm(p1 == 1, "Expected: %u, Received: %u", (uint32_t)1, p1);

    for (uint8_t i = 1; i < 31; i++) {
      // make sure to cast 1 to uint32_t to ensure it's a 32bit value
      uint32_t n = ((uint32_t)1 << i);
      uint32_t pn = CLOSESTPOWEROF2(n);

      uint32_t m = n + 1;
      uint32_t pm = CLOSESTPOWEROF2(m);
      uint32_t epm = (uint32_t)1 << (i + 1);

      assertm(pn == n, "Expected: %u, Received: %u (i = %u)", n, pn, i);
      assertm(pm == epm, "Expected: %u, Received: %u (i = %u)", epm, pm, i);
    }
  }

  {
    testlog(L_INFO, "Testing CLOSESTPOWEROF2 macro for uint64_t values");

    uint64_t p0 = CLOSESTPOWEROF2((uint64_t)0);
    assertm(p0 == 0, "Expected: %llu, Received: %llu", (uint64_t)0, p0);

    uint64_t p1 = CLOSESTPOWEROF2((uint64_t)1);
    assertm(p1 == 1, "Expected: %llu, Received: %llu", (uint64_t)1, p1);

    for (uint64_t i = 31; i < 63; i++) {
      // make sure to cast 1 to uint32_t to ensure it's a 64bit value
      // as 1 by default is int which is typically 32bits
      uint64_t n = ((uint64_t)1 << i);
      uint64_t pn = CLOSESTPOWEROF2(n);

      uint64_t m = n + 1;
      uint64_t pm = CLOSESTPOWEROF2(m);
      uint64_t epm = (uint64_t)1 << (i + 1);

      assertm(pn == n, "Expected: %llu, Received: %llu (i = %llu)", n, pn, i);
      assertm(pm == epm, "Expected: %llu, Received: %llu (i = %llu)", epm, pm, i);
    }
  }

  testlog(L_INFO, "<zdx_util_test> All ok!\n");
  return 0;
}
