/*
 * e8call.hpp
 *
 *  Created on: Dec 7, 2017
 *      Author: nullifiedcat
 */

#ifndef _E8_INC_
#define _E8_INC_

#include <stdint.h>

inline void * e8call(void * pAddress) { return (void *)uintptr_t(*(uintptr_t *)pAddress + uintptr_t(pAddress) + 4); }
inline uintptr_t e8call(uintptr_t iAddress) { return (uintptr_t)(e8call((void*)iAddress)); }
inline uintptr_t e8call_direct(uintptr_t iAddress) { return e8call(iAddress + 1); }

#endif