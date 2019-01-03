#if ZANY80_ABI == 1 || ZANY80_ABI == CPP

#ifndef __cplusplus
#error Zany80 ABI 1 can only be used from C++. Try using ABI 2.
#endif
#include "ABI/ABI1.h"

#elif ZANY80_ABI == 2 || ZANY80_ABI == LIBLIB

#warning Zany80 ABI 2 is experimental
#include "ABI/ABI2.h"

#else

#error Must specify a plugin ABI

#endif
