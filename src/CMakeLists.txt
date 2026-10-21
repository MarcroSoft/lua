# ------------------------------------------------------------------------------
# Build Lua as static library only
# ------------------------------------------------------------------------------

set(LUA_CORE_SOURCES
  lapi.c lctype.c lfunc.c lmathlib.c loslib.c ltable.c lundump.c
  lauxlib.c ldblib.c lgc.c lmem.c lparser.c ltablib.c lutf8lib.c
  lbaselib.c ldebug.c linit.c loadlib.c lstate.c ltm.c lvm.c
  lcode.c ldo.c liolib.c lobject.c lstring.c lzio.c lcorolib.c
  ldump.c llex.c lopcodes.c lstrlib.c
)

set(LUA_HEADERS
  lua.h luaconf.h lualib.h lauxlib.h lua.hpp
)

# ------------------------------------------------------------------------------
# Static Lua library
# ------------------------------------------------------------------------------

add_library(lib${PROJECT_NAME} STATIC ${LUA_CORE_SOURCES})

target_sources(lib${PROJECT_NAME}
  PUBLIC
    FILE_SET HEADERS
    FILES ${LUA_HEADERS}
)

target_include_directories(lib${PROJECT_NAME}
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>"
)

set_target_properties(lib${PROJECT_NAME} PROPERTIES
  OUTPUT_NAME "lua"
  PREFIX "lib"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# IMPORTANT:
# Do not define LUA_BUILD_AS_DLL.
# Static builds must not use __declspec(dllimport/dllexport).

# ------------------------------------------------------------------------------
# Bytecode-only Lua library (for the Oidua game stub)
# ------------------------------------------------------------------------------
# The stub only ever loads precompiled bytecode, so the Lua front-end is dead
# weight there. lnoparser.c supplies the three symbols that tie the rest of the
# library to it - luaX_init (lstate.c), luaY_parser (ldo.c) and luaU_dump
# (lapi.c) - so lparser.c, lcode.c, llex.c and ldump.c can stay out of the
# archive. It also stands in for linit.c with a luaL_openlibs limited to the
# sandboxed stdlib (no package/require, io or debug), which is what lets
# loadlib.c, liolib.c and ldblib.c go too.
#
# Keeping all of that in lnoparser.c rather than in #ifdefs is what leaves
# every Lua source file byte-identical to the official 5.4.8 release, so the
# next 5.4.x is a plain drop-in of src/. It also turns an accidental
# re-inclusion of any excluded file into a duplicate symbol error.

set(LUA_BCO_SOURCES ${LUA_CORE_SOURCES})
list(REMOVE_ITEM LUA_BCO_SOURCES
  lparser.c lcode.c llex.c ldump.c loadlib.c liolib.c ldblib.c linit.c)
list(APPEND LUA_BCO_SOURCES lnoparser.c)

add_library(liblua_bco STATIC ${LUA_BCO_SOURCES})
target_include_directories(liblua_bco
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
set_target_properties(liblua_bco PROPERTIES
  OUTPUT_NAME "lua_bco"
  PREFIX "lib"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ------------------------------------------------------------------------------
# Executables
# ------------------------------------------------------------------------------

add_executable(lua
  lua.c
  ${PROJECT_SOURCE_DIR}/logo/lua-logo-olp-dist.rc
  ${PROJECT_SOURCE_DIR}/logo/lua-olp.manifest
)

add_executable(luac
  luac.c
  ${PROJECT_SOURCE_DIR}/logo/lua-logo-olp-dist.rc
  ${PROJECT_SOURCE_DIR}/logo/lua-olp.manifest
)

add_executable(wlua
  lua.c
  ${PROJECT_SOURCE_DIR}/logo/lua-logo-olp-dist.rc
  ${PROJECT_SOURCE_DIR}/logo/lua-olp.manifest
)

target_link_libraries(lua PRIVATE lib${PROJECT_NAME})
target_link_libraries(luac PRIVATE lib${PROJECT_NAME})
target_link_libraries(wlua PRIVATE lib${PROJECT_NAME})

if(WIN32)
  set_target_properties(wlua PROPERTIES WIN32_EXECUTABLE TRUE)

  if(CMAKE_C_COMPILER_ID STREQUAL "IntelLLVM")
    target_link_options(wlua PRIVATE "/link" "/ENTRY:mainCRTStartup")
  elseif(MSVC)
    target_link_options(wlua PRIVATE "/ENTRY:mainCRTStartup")
  endif()
endif()

# ------------------------------------------------------------------------------
# Install
# ------------------------------------------------------------------------------

install(
  TARGETS lib${PROJECT_NAME}
  EXPORT lib${PROJECT_NAME}Targets
  ARCHIVE DESTINATION ${INSTALL_LIBDIR} COMPONENT lib
  FILE_SET HEADERS DESTINATION ${INSTALL_INCLUDEDIR} COMPONENT dev
)

install(
  TARGETS lua wlua luac
  RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT bin
)

install(
  EXPORT lib${PROJECT_NAME}Targets
  FILE lib${PROJECT_NAME}Targets.cmake
  DESTINATION ${INSTALL_CMAKEDIR}
)

# ------------------------------------------------------------------------------
# CMake package config
# ------------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/lib${PROJECT_NAME}Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${INSTALL_CMAKEDIR}
  PATH_VARS INSTALL_DIR INSTALL_INCLUDEDIR INSTALL_LIBDIR
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMinorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${INSTALL_CMAKEDIR}
)