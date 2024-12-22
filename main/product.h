#ifndef __PRODUCT_H__
#define __PRODUCT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "cc_err.h"


cc_err_t product_init(void);
void __reboot(uint32_t interval, void *arg);


#ifdef __cplusplus
}
#endif

#endif