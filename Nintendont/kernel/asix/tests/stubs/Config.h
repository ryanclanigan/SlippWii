#ifndef ASIX_TEST_CONFIG_H
#define ASIX_TEST_CONFIG_H
#include "global.h"
#define NIN_CFG_SW_DIAG_LOG (1u << 20)
extern int test_diagnostic_log;
static inline int ConfigGetConfig(u32 flag) {
    assert(flag==NIN_CFG_SW_DIAG_LOG);
    return test_diagnostic_log;
}
#endif
