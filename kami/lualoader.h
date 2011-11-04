// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

extern "C" {
#include "../kami-lua/lua.h"
#include "../kami-lua/lualib.h"
#include "../kami-lua/lauxlib.h"
}
