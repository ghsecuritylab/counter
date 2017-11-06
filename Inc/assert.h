#ifndef ASSERT_H__
#define ASSERT_H__    1

#include "trace.h"

#define ASSERT(x)   {   if (!(x)) {TRACE_printf("%s[%d] : ASSERTED(%s)\n", __func__, __LINE__, #x); while(1);}}

#endif
