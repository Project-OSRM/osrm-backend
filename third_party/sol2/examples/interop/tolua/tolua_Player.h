/*
** Lua binding: Player
** Generated automatically by tolua++-1.0.93-lua53 on Sat Feb 10 08:48:53 2018.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_Player_open (lua_State* tolua_S);

#include "Player.h"

/* function to release collected object via destructor */
#ifdef __cplusplus

static int tolua_collect_Player (lua_State* tolua_S)
{
 Player* self = (Player*) tolua_tousertype(tolua_S,1,0);
	Mtolua_delete(self);
	return 0;
}
#endif


/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"Player");
}

/* method: new of class  Player */
#ifndef TOLUA_DISABLE_tolua_Player_Player_new00
static int tolua_Player_Player_new00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   Player* tolua_ret = (Player*)  Mtolua_new((Player)());
    tolua_pushusertype(tolua_S,(void*)tolua_ret,"Player");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'new'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: new_local of class  Player */
#ifndef TOLUA_DISABLE_tolua_Player_Player_new00_local
static int tolua_Player_Player_new00_local(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   Player* tolua_ret = (Player*)  Mtolua_new((Player)());
    tolua_pushusertype(tolua_S,(void*)tolua_ret,"Player");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'new'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: delete of class  Player */
#ifndef TOLUA_DISABLE_tolua_Player_Player_delete00
static int tolua_Player_Player_delete00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  Player* self = (Player*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'delete'", NULL);
#endif
  Mtolua_delete(self);
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'delete'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: setHealth of class  Player */
#ifndef TOLUA_DISABLE_tolua_Player_Player_setHealth00
static int tolua_Player_Player_setHealth00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  Player* self = (Player*)  tolua_tousertype(tolua_S,1,0);
  int _health = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'setHealth'", NULL);
#endif
  {
   self->setHealth(_health);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'setHealth'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: getHealth of class  Player */
#ifndef TOLUA_DISABLE_tolua_Player_Player_getHealth00
static int tolua_Player_Player_getHealth00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  Player* self = (Player*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getHealth'", NULL);
#endif
  {
   int tolua_ret = (int)  self->getHealth();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getHealth'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_Player_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  #ifdef __cplusplus
  tolua_cclass(tolua_S,"Player","Player","",tolua_collect_Player);
  #else
  tolua_cclass(tolua_S,"Player","Player","",NULL);
  #endif
  tolua_beginmodule(tolua_S,"Player");
   tolua_function(tolua_S,"new",tolua_Player_Player_new00);
   tolua_function(tolua_S,"new_local",tolua_Player_Player_new00_local);
   tolua_function(tolua_S,".call",tolua_Player_Player_new00_local);
   tolua_function(tolua_S,"delete",tolua_Player_Player_delete00);
   tolua_function(tolua_S,"setHealth",tolua_Player_Player_setHealth00);
   tolua_function(tolua_S,"getHealth",tolua_Player_Player_getHealth00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_Player (lua_State* tolua_S) {
 return tolua_Player_open(tolua_S);
};
#endif

