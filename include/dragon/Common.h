#ifndef __DRAGON_COMMON__
#define __DRAGON_COMMON__

#include <assert.h>
#include <iostream>

#ifndef NDEBUG
 #define DRAGON_DEBUG(x) x;
#else
 #define DRAGON_DEBUG(x) do { } while(0);
#endif

inline std::ostream &dbgs() {
  return std::cerr;
}

#define GLOBAL_FUNC "@global"

#endif