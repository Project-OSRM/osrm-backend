#ifndef KEPLER_PROJECT_COMPAT53_H_
#define KEPLER_PROJECT_COMPAT53_H_

#include <stddef.h>
#include <limits.h>
#include <string.h>
#if defined(__cplusplus) && !defined(COMPAT53_LUA_CPP)
extern "C" {
#endif
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#if defined(__cplusplus) && !defined(COMPAT53_LUA_CPP)
}
#endif

#ifndef COMPAT53_PREFIX
/* we chose this name because many other lua bindings / libs have
* their own compatibility layer, and that use the compat53 declaration
* frequently, causing all kinds of linker / compiler issues
*/
#  define COMPAT53_PREFIX kp_compat53
#endif // COMPAT53_PREFIX

#ifndef COMPAT53_API
#  if defined(COMPAT53_INCLUDE_SOURCE) && COMPAT53_INCLUDE_SOURCE
#    if defined(__GNUC__) || defined(__clang__)
#      define COMPAT53_API __attribute__((__unused__)) static
#    else
#      define COMPAT53_API static
#    endif /* Clang/GCC */
#  else /* COMPAT53_INCLUDE_SOURCE */
/* we are not including source, so everything is extern */
#      define COMPAT53_API extern
#  endif /* COMPAT53_INCLUDE_SOURCE */
#endif /* COMPAT53_PREFIX */


#define COMPAT53_CONCAT_HELPER(a, b) a##b
#define COMPAT53_CONCAT(a, b) COMPAT53_CONCAT_HELPER(a, b)



/* declarations for Lua 5.1 */
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM == 501

/* XXX not implemented:
* lua_arith (new operators)
* lua_upvalueid
* lua_upvaluejoin
* lua_version
* lua_yieldk
*/

#ifndef LUA_OK
#  define LUA_OK 0
#endif
#ifndef LUA_OPADD
#  define LUA_OPADD 0
#endif
#ifndef LUA_OPSUB
#  define LUA_OPSUB 1
#endif
#ifndef LUA_OPMUL
#  define LUA_OPMUL 2
#endif
#ifndef LUA_OPDIV
#  define LUA_OPDIV 3
#endif
#ifndef LUA_OPMOD
#  define LUA_OPMOD 4
#endif
#ifndef LUA_OPPOW
#  define LUA_OPPOW 5
#endif
#ifndef LUA_OPUNM
#  define LUA_OPUNM 6
#endif
#ifndef LUA_OPEQ
#  define LUA_OPEQ 0
#endif
#ifndef LUA_OPLT
#  define LUA_OPLT 1
#endif
#ifndef LUA_OPLE
#  define LUA_OPLE 2
#endif

/* LuaJIT/Lua 5.1 does not have the updated
* error codes for thread status/function returns (but some patched versions do)
* define it only if it's not found
*/
#if !defined(LUA_ERRGCMM)
/* Use + 2 because in some versions of Lua (Lua 5.1)
* LUA_ERRFILE is defined as (LUA_ERRERR+1)
* so we need to avoid it (LuaJIT might have something at this
* integer value too)
*/
#  define LUA_ERRGCMM (LUA_ERRERR + 2)
#endif /* LUA_ERRGCMM define */

typedef size_t lua_Unsigned;

typedef struct luaL_Buffer_53 {
	luaL_Buffer b; /* make incorrect code crash! */
	char *ptr;
	size_t nelems;
	size_t capacity;
	lua_State *L2;
} luaL_Buffer_53;
#define luaL_Buffer luaL_Buffer_53

/* In PUC-Rio 5.1, userdata is a simple FILE*
* In LuaJIT, it's a struct where the first member is a FILE*
* We can't support the `closef` member
*/
typedef struct luaL_Stream {
	FILE *f;
} luaL_Stream;

#define lua_absindex COMPAT53_CONCAT(COMPAT53_PREFIX, _absindex)
COMPAT53_API int lua_absindex(lua_State *L, int i);

#define lua_arith COMPAT53_CONCAT(COMPAT53_PREFIX, _arith)
COMPAT53_API void lua_arith(lua_State *L, int op);

#define lua_compare COMPAT53_CONCAT(COMPAT53_PREFIX, _compare)
COMPAT53_API int lua_compare(lua_State *L, int idx1, int idx2, int op);

#define lua_copy COMPAT53_CONCAT(COMPAT53_PREFIX, _copy)
COMPAT53_API void lua_copy(lua_State *L, int from, int to);

#define lua_getuservalue(L, i) \
  (lua_getfenv((L), (i)), lua_type((L), -1))
#define lua_setuservalue(L, i) \
  (luaL_checktype((L), -1, LUA_TTABLE), lua_setfenv((L), (i)))

#define lua_len COMPAT53_CONCAT(COMPAT53_PREFIX, _len)
COMPAT53_API void lua_len(lua_State *L, int i);

#define lua_pushstring(L, s) \
  (lua_pushstring((L), (s)), lua_tostring((L), -1))

#define lua_pushlstring(L, s, len) \
  ((((len) == 0) ? lua_pushlstring((L), "", 0) : lua_pushlstring((L), (s), (len))), lua_tostring((L), -1))

#ifndef luaL_newlibtable
#  define luaL_newlibtable(L, l) \
  (lua_createtable((L), 0, sizeof((l))/sizeof(*(l))-1))
#endif
#ifndef luaL_newlib
#  define luaL_newlib(L, l) \
  (luaL_newlibtable((L), (l)), luaL_register((L), NULL, (l)))
#endif

#define lua_pushglobaltable(L) \
  lua_pushvalue((L), LUA_GLOBALSINDEX)

#define lua_rawgetp COMPAT53_CONCAT(COMPAT53_PREFIX, _rawgetp)
COMPAT53_API int lua_rawgetp(lua_State *L, int i, const void *p);

#define lua_rawsetp COMPAT53_CONCAT(COMPAT53_PREFIX, _rawsetp)
COMPAT53_API void lua_rawsetp(lua_State *L, int i, const void *p);

#define lua_rawlen(L, i) lua_objlen((L), (i))

#define lua_tointeger(L, i) lua_tointegerx((L), (i), NULL)

#define lua_tonumberx COMPAT53_CONCAT(COMPAT53_PREFIX, _tonumberx)
COMPAT53_API lua_Number lua_tonumberx(lua_State *L, int i, int *isnum);

#define luaL_checkversion COMPAT53_CONCAT(COMPAT53_PREFIX, L_checkversion)
COMPAT53_API void luaL_checkversion(lua_State *L);

#define lua_load COMPAT53_CONCAT(COMPAT53_PREFIX, _load_53)
COMPAT53_API int lua_load(lua_State *L, lua_Reader reader, void *data, const char* source, const char* mode);

#define luaL_loadfilex COMPAT53_CONCAT(COMPAT53_PREFIX, L_loadfilex)
COMPAT53_API int luaL_loadfilex(lua_State *L, const char *filename, const char *mode);

#define luaL_loadbufferx COMPAT53_CONCAT(COMPAT53_PREFIX, L_loadbufferx)
COMPAT53_API int luaL_loadbufferx(lua_State *L, const char *buff, size_t sz, const char *name, const char *mode);

#define luaL_checkstack COMPAT53_CONCAT(COMPAT53_PREFIX, L_checkstack_53)
COMPAT53_API void luaL_checkstack(lua_State *L, int sp, const char *msg);

#define luaL_getsubtable COMPAT53_CONCAT(COMPAT53_PREFIX, L_getsubtable)
COMPAT53_API int luaL_getsubtable(lua_State* L, int i, const char *name);

#define luaL_len COMPAT53_CONCAT(COMPAT53_PREFIX, L_len)
COMPAT53_API lua_Integer luaL_len(lua_State *L, int i);

#define luaL_setfuncs COMPAT53_CONCAT(COMPAT53_PREFIX, L_setfuncs)
COMPAT53_API void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup);

#define luaL_setmetatable COMPAT53_CONCAT(COMPAT53_PREFIX, L_setmetatable)
COMPAT53_API void luaL_setmetatable(lua_State *L, const char *tname);

#define luaL_testudata COMPAT53_CONCAT(COMPAT53_PREFIX, L_testudata)
COMPAT53_API void *luaL_testudata(lua_State *L, int i, const char *tname);

#define luaL_traceback COMPAT53_CONCAT(COMPAT53_PREFIX, L_traceback)
COMPAT53_API void luaL_traceback(lua_State *L, lua_State *L1, const char *msg, int level);

#define luaL_fileresult COMPAT53_CONCAT(COMPAT53_PREFIX, L_fileresult)
COMPAT53_API int luaL_fileresult(lua_State *L, int stat, const char *fname);

#define luaL_execresult COMPAT53_CONCAT(COMPAT53_PREFIX, L_execresult)
COMPAT53_API int luaL_execresult(lua_State *L, int stat);

#define lua_callk(L, na, nr, ctx, cont) \
  ((void)(ctx), (void)(cont), lua_call((L), (na), (nr)))
#define lua_pcallk(L, na, nr, err, ctx, cont) \
  ((void)(ctx), (void)(cont), lua_pcall((L), (na), (nr), (err)))

#define lua_resume(L, from, nargs) \
  ((void)(from), lua_resume((L), (nargs)))

#define luaL_buffinit COMPAT53_CONCAT(COMPAT53_PREFIX, _buffinit_53)
COMPAT53_API void luaL_buffinit(lua_State *L, luaL_Buffer_53 *B);

#define luaL_prepbuffsize COMPAT53_CONCAT(COMPAT53_PREFIX, _prepbufsize_53)
COMPAT53_API char *luaL_prepbuffsize(luaL_Buffer_53 *B, size_t s);

#define luaL_addlstring COMPAT53_CONCAT(COMPAT53_PREFIX, _addlstring_53)
COMPAT53_API void luaL_addlstring(luaL_Buffer_53 *B, const char *s, size_t l);

#define luaL_addvalue COMPAT53_CONCAT(COMPAT53_PREFIX, _addvalue_53)
COMPAT53_API void luaL_addvalue(luaL_Buffer_53 *B);

#define luaL_pushresult COMPAT53_CONCAT(COMPAT53_PREFIX, _pushresult_53)
COMPAT53_API void luaL_pushresult(luaL_Buffer_53 *B);

#undef luaL_buffinitsize
#define luaL_buffinitsize(L, B, s) \
  (luaL_buffinit((L), (B)), luaL_prepbuffsize((B), (s)))

#undef luaL_prepbuffer
#define luaL_prepbuffer(B) \
  luaL_prepbuffsize((B), LUAL_BUFFERSIZE)

#undef luaL_addchar
#define luaL_addchar(B, c) \
  ((void)((B)->nelems < (B)->capacity || luaL_prepbuffsize((B), 1)), \
   ((B)->ptr[(B)->nelems++] = (c)))

#undef luaL_addsize
#define luaL_addsize(B, s) \
  ((B)->nelems += (s))

#undef luaL_addstring
#define luaL_addstring(B, s) \
  luaL_addlstring((B), (s), strlen((s)))

#undef luaL_pushresultsize
#define luaL_pushresultsize(B, s) \
  (luaL_addsize((B), (s)), luaL_pushresult((B)))

#if defined(LUA_COMPAT_APIINTCASTS)
#define lua_pushunsigned(L, n) \
  lua_pushinteger((L), (lua_Integer)(n))
#define lua_tounsignedx(L, i, is) \
  ((lua_Unsigned)lua_tointegerx((L), (i), (is)))
#define lua_tounsigned(L, i) \
  lua_tounsignedx((L), (i), NULL)
#define luaL_checkunsigned(L, a) \
  ((lua_Unsigned)luaL_checkinteger((L), (a)))
#define luaL_optunsigned(L, a, d) \
  ((lua_Unsigned)luaL_optinteger((L), (a), (lua_Integer)(d)))
#endif

#endif /* Lua 5.1 only */



/* declarations for Lua 5.1 and 5.2 */
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM <= 502

typedef int lua_KContext;

typedef int(*lua_KFunction)(lua_State *L, int status, lua_KContext ctx);

#define lua_dump(L, w, d, s) \
  ((void)(s), lua_dump((L), (w), (d)))

#define lua_getfield(L, i, k) \
  (lua_getfield((L), (i), (k)), lua_type((L), -1))

#define lua_gettable(L, i) \
  (lua_gettable((L), (i)), lua_type((L), -1))

#define lua_geti COMPAT53_CONCAT(COMPAT53_PREFIX, _geti)
COMPAT53_API int lua_geti(lua_State *L, int index, lua_Integer i);

#define lua_isinteger COMPAT53_CONCAT(COMPAT53_PREFIX, _isinteger)
COMPAT53_API int lua_isinteger(lua_State *L, int index);

#define lua_tointegerx COMPAT53_CONCAT(COMPAT53_PREFIX, _tointegerx_53)
COMPAT53_API lua_Integer lua_tointegerx(lua_State *L, int i, int *isnum);

#define lua_numbertointeger(n, p) \
  ((*(p) = (lua_Integer)(n)), 1)

#define lua_rawget(L, i) \
  (lua_rawget((L), (i)), lua_type((L), -1))

#define lua_rawgeti(L, i, n) \
  (lua_rawgeti((L), (i), (n)), lua_type((L), -1))

#define lua_rotate COMPAT53_CONCAT(COMPAT53_PREFIX, _rotate)
COMPAT53_API void lua_rotate(lua_State *L, int idx, int n);

#define lua_seti COMPAT53_CONCAT(COMPAT53_PREFIX, _seti)
COMPAT53_API void lua_seti(lua_State *L, int index, lua_Integer i);

#define lua_stringtonumber COMPAT53_CONCAT(COMPAT53_PREFIX, _stringtonumber)
COMPAT53_API size_t lua_stringtonumber(lua_State *L, const char *s);

#define luaL_tolstring COMPAT53_CONCAT(COMPAT53_PREFIX, L_tolstring)
COMPAT53_API const char *luaL_tolstring(lua_State *L, int idx, size_t *len);

#define luaL_getmetafield(L, o, e) \
  (luaL_getmetafield((L), (o), (e)) ? lua_type((L), -1) : LUA_TNIL)

#define luaL_newmetatable(L, tn) \
  (luaL_newmetatable((L), (tn)) ? (lua_pushstring((L), (tn)), lua_setfield((L), -2, "__name"), 1) : 0)

#define luaL_requiref COMPAT53_CONCAT(COMPAT53_PREFIX, L_requiref_53)
COMPAT53_API void luaL_requiref(lua_State *L, const char *modname,
	lua_CFunction openf, int glb);

#endif /* Lua 5.1 and Lua 5.2 */



/* declarations for Lua 5.2 */
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM == 502

/* XXX not implemented:
* lua_isyieldable
* lua_getextraspace
* lua_arith (new operators)
* lua_pushfstring (new formats)
*/

#define lua_getglobal(L, n) \
  (lua_getglobal((L), (n)), lua_type((L), -1))

#define lua_getuservalue(L, i) \
  (lua_getuservalue((L), (i)), lua_type((L), -1))

#define lua_pushlstring(L, s, len) \
  (((len) == 0) ? lua_pushlstring((L), "", 0) : lua_pushlstring((L), (s), (len)))

#define lua_rawgetp(L, i, p) \
  (lua_rawgetp((L), (i), (p)), lua_type((L), -1))

#define LUA_KFUNCTION(_name) \
  static int (_name)(lua_State *L, int status, lua_KContext ctx); \
  static int (_name ## _52)(lua_State *L) { \
    lua_KContext ctx; \
    int status = lua_getctx(L, &ctx); \
    return (_name)(L, status, ctx); \
  } \
  static int (_name)(lua_State *L, int status, lua_KContext ctx)

#define lua_pcallk(L, na, nr, err, ctx, cont) \
  lua_pcallk((L), (na), (nr), (err), (ctx), cont ## _52)

#define lua_callk(L, na, nr, ctx, cont) \
  lua_callk((L), (na), (nr), (ctx), cont ## _52)

#define lua_yieldk(L, nr, ctx, cont) \
  lua_yieldk((L), (nr), (ctx), cont ## _52)

#ifdef lua_call
#  undef lua_call
#  define lua_call(L, na, nr) \
  (lua_callk)((L), (na), (nr), 0, NULL)
#endif

#ifdef lua_pcall
#  undef lua_pcall
#  define lua_pcall(L, na, nr, err) \
  (lua_pcallk)((L), (na), (nr), (err), 0, NULL)
#endif

#ifdef lua_yield
#  undef lua_yield
#  define lua_yield(L, nr) \
  (lua_yieldk)((L), (nr), 0, NULL)
#endif

#endif /* Lua 5.2 only */



/* other Lua versions */
#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 501 || LUA_VERSION_NUM > 504

#  error "unsupported Lua version (i.e. not Lua 5.1, 5.2, or 5.3)"

#endif /* other Lua versions except 5.1, 5.2, and 5.3 */



/* helper macro for defining continuation functions (for every version
* *except* Lua 5.2) */
#ifndef LUA_KFUNCTION
#define LUA_KFUNCTION(_name) \
  static int (_name)(lua_State *L, int status, lua_KContext ctx)
#endif


#if defined(COMPAT53_INCLUDE_SOURCE) && COMPAT53_INCLUDE_SOURCE == 1
#  include "compat-5.3.c"
#endif


#endif /* KEPLER_PROJECT_COMPAT53_H_ */

