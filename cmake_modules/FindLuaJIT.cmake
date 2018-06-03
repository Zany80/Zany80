set(LUAJIT_PATHS
    ${LUAJIT_ROOT}
    $ENV{LUAJIT_ROOT}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
)

find_path(LUA_INCLUDE_DIR luajit.h
    PATH_SUFFIXES include/luajit-2.0 include/luajit-2.1 include
    PATHS ${LUAJIT_PATHS}
)

find_library(LUA_LIBRARY
    NAMES luajit-5.1
    PATHS ${LUAJIT_PATHS}
)

if(LUA_LIBRARY)
  # include the math library for Unix
  if(UNIX AND NOT APPLE)
    find_library(LUA_MATH_LIBRARY m)
    set( LUA_LIBRARIES "${LUA_LIBRARY};${LUA_MATH_LIBRARY}" CACHE STRING "Lua Libraries")
  # For Windows and Mac, don't need to explicitly include the math library
  else()
    set( LUA_LIBRARIES "${LUA_LIBRARY}" CACHE STRING "Lua Libraries")
  endif()
endif()

if(LUA_INCLUDE_DIR AND EXISTS "${LUA_INCLUDE_DIR}/luajit.h")
  file(STRINGS "${LUA_INCLUDE_DIR}/luajit.h" luajit_version_str REGEX "^#define[ \t]+LUAJIT_VERSION[ \t]+\"LuaJIT .+\"")

  string(REGEX REPLACE "^#define[ \t]+LUAJIT_VERSION[ \t]+\"LuaJIT ([^\"]+)\".*" "\\1" LUAJIT_VERSION_STRING "${luajit_version_str}")
  unset(luajit_version_str)
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LuaJIT
                                  REQUIRED_VARS LUA_LIBRARIES LUA_INCLUDE_DIR
                                  VERSION_VAR LUAJIT_VERSION_STRING)

mark_as_advanced(LUA_INCLUDE_DIR LUA_LIBRARIES LUA_LIBRARY LUA_MATH_LIBRARY)
