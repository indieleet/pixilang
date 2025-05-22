#pragma once

#include <stdint.h>
#include <stdlib.h> //malloc
#include <unistd.h> //close

#define HAVE_FCNTL_H 1
#define HAVE_STDARG_H 1
#define UINT32 uint32_t

#ifdef __ANDROID__
    #define S_IWRITE S_IWUSR
    #define S_IREAD S_IRUSR
#endif
