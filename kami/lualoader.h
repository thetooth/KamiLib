// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

extern "C" {
#include "../../lua-5.2.0/lua-5.2.0/lua.h"
#include "../../lua-5.2.0/lua-5.2.0/lualib.h"
#include "../../lua-5.2.0/lua-5.2.0/lauxlib.h"
}

namespace klib {
	inline void LuaCheckError(lua_State *L, int status){
		if ( status!=0 ) {
			cl("-- %s\n", lua_tostring(L, -1));
			lua_pop(L, 1); // remove error message
		}
	}
}