/*
** $Id: lnoparser.c $
** Front-end stubs for the bytecode-only Lua library
** See Copyright Notice in lua.h
*/

/*
** Added for Oidua. Not part of upstream Lua.
**
** The game stub only ever loads precompiled bytecode, so the Lua front-end
** is dead weight in it. This file is what makes dropping the front-end cost
** nothing in the Lua sources themselves: every .c and .h file next to it is
** byte-identical to the official lua-5.5.1 release, and the entire delta
** lives here and in the liblua_bco target in CMakeLists.txt. Re-basing on a
** future 5.5.x is then a plain drop-in of src/.
**
** Only three symbols tie the rest of the library to the front-end:
**
**   luaX_init    called once from f_luaopen in lstate.c to intern the
**                reserved words; the only reference into llex.c from
**                outside the parser
**   luaY_parser  called from f_parser in ldo.c to compile source text; the
**                only reference into lparser.c and lcode.c
**   luaU_dump    called from lua_dump in lapi.c to write bytecode; the only
**                reference into ldump.c
**
** Defining them here lets lparser.c, lcode.c, llex.c and ldump.c stay out
** of the archive. This file also stands in for linit.c, with a
** luaL_openselectedlibs that opens only the sandboxed subset of the
** standard library, which is what lets loadlib.c, liolib.c and ldblib.c go
** too. (luaL_openlibs is a macro over luaL_openselectedlibs in 5.5.)
**
** Because these are real definitions and not #ifdef'd-away call sites,
** linking any of those files back into liblua_bco is a duplicate symbol
** error at build time rather than a parser that quietly comes back to life.
*/

#define lnoparser_c
#define LUA_CORE
#define LUA_LIB

#include "lprefix.h"


#include <stddef.h>

#include "lua.h"

#include "lauxlib.h"
#include "ldo.h"
#include "llex.h"
#include "llimits.h"
#include "lobject.h"
#include "lparser.h"
#include "lualib.h"
#include "lundump.h"


/*
** Interning the reserved words only ever serves the lexer, which is not in
** this build, so there is nothing to intern.
*/
void luaX_init (lua_State *L) {
  UNUSED(L);
}


/*
** Compiling source text is precisely what this build exists to prevent.
** Fail the way the lexer would have: LUA_ERRSYNTAX with a message left on
** the stack, so lua_load and luaL_loadbuffer report it through their normal
** path and callers never have to know they are talking to a stub.
*/
LClosure *luaY_parser (lua_State *L, ZIO *z, Table *anchor, Mbuffer *buff,
                       Dyndata *dyd, const char *name, int firstchar) {
  UNUSED(z); UNUSED(anchor); UNUSED(buff); UNUSED(dyd); UNUSED(firstchar);
  luaO_pushfstring(L, "%s: source loading is disabled (bytecode only)", name);
  luaD_throw(L, LUA_ERRSYNTAX);
  return NULL;  /* to avoid warnings; luaD_throw does not return */
}


/*
** Writing bytecode needs ldump.c. lua_dump reads any non-zero return as
** "could not dump", the same answer it gives for a C function, so there is
** no new failure mode to document here.
*/
int luaU_dump (lua_State *L, const Proto *f, lua_Writer w, void *data,
               int strip) {
  UNUSED(L); UNUSED(f); UNUSED(w); UNUSED(data); UNUSED(strip);
  return 1;
}


/*
** Stands in for linit.c. package (and with it require), io and debug are
** left out: none of the three is part of the documented Oidua scripting
** API, and leaving them out is what keeps loadlib.c, liolib.c and ldblib.c
** out of the archive.
**
** The three keep their places in the table, without a function, so every
** other library keeps the bit lualib.h gives it (LUA_GLIBK, LUA_COLIBK and
** so on). Asking for one of the three opens or preloads nothing.
*/
static const luaL_Reg stdlibs[] = {
  {LUA_GNAME, luaopen_base},
  {LUA_LOADLIBNAME, NULL},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_DBLIBNAME, NULL},
  {LUA_IOLIBNAME, NULL},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {NULL, NULL}
};


/*
** require and preload selected standard libraries
*/
LUALIB_API void luaL_openselectedlibs (lua_State *L, int load, int preload) {
  int mask;
  const luaL_Reg *lib;
  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
  for (lib = stdlibs, mask = 1; lib->name != NULL; lib++, mask <<= 1) {
    if (lib->func == NULL)  /* not in this build? */
      continue;
    if (load & mask) {  /* selected? */
      luaL_requiref(L, lib->name, lib->func, 1);  /* require library */
      lua_pop(L, 1);  /* remove result from the stack */
    }
    else if (preload & mask) {  /* selected? */
      lua_pushcfunction(L, lib->func);
      lua_setfield(L, -2, lib->name);  /* add library to PRELOAD table */
    }
  }
  lua_assert((mask >> 1) == LUA_UTF8LIBK);
  lua_pop(L, 1);  /* remove PRELOAD table */
}
