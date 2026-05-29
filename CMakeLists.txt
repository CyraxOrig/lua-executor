cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64")

project(LuaExecutor VERSION 1.0.0)

# ─── Fetch Lua 5.4 ────────────────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    lua_src
    GIT_REPOSITORY https://github.com/walterschell/Lua.git
    GIT_TAG        v5.4.6
)
FetchContent_MakeAvailable(lua_src)

# ─── Collect sources ──────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# ─── Geode SDK ────────────────────────────────────────────────────────────────
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "GEODE_SDK environment variable is not set!\n"
        "Set it to the path of your Geode SDK installation."
    )
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# ─── Include paths ────────────────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${lua_src_SOURCE_DIR}/lua-5.4.6/include
)

# ─── Link libraries ───────────────────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME}
    geode-sdk
    lua_static
)

# ─── Geode setup ──────────────────────────────────────────────────────────────
setup_geode_mod(${PROJECT_NAME})
