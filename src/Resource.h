#pragma once

#define STRINGIZE_(s) #s
#define STRINGIZE(s)  STRINGIZE_(s)

#define VER_MAJOR                1
#define VER_MINOR                0
#define VER_REVISION             0

#define VER_STRING \
    STRINGIZE(VER_MAJOR) "." STRINGIZE(VER_MINOR) "." STRINGIZE(VER_REVISION)

#define IDI_NTMU               100
