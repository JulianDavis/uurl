#pragma once
#include <stdio.h>

#ifdef DEBUG
#define debug_print(fmt, ...) do { fprintf(stderr, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__); } while (0)
#else
#define debug_print(fmt, ...) do { } while (0)
#endif
