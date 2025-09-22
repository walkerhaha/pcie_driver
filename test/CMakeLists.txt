cmake_minimum_required (VERSION 2.6)

project (shell)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

#
# cmake -DCMAKE_BUILD_TYPE=Debug ..
# cmake -DCMAKE_BUILD_TYPE=Release ..
#
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
IF(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Debug build.")
  set(CMAKE_CXX_FLAGS "-g -O0 -Wall")
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
  message("Release build.")
  set(CMAKE_CXX_FLAGS "-O2 -Wall")
ELSE()
  message("Some other build type.")
  message("Set to Debug build.")
  set(CMAKE_CXX_FLAGS "-g -O0 -Wall")
ENDIF()

SET(CMAKE_CXX_COMPILER "g++")
SET(CMAKE_C_COMPILER   "gcc")

SET(CMAKE_CXX_COMPILER_ARG1 "-std=c++11 -DSPDLOG_COMPILED_LIB")
SET(CMAKE_C_COMPILER_ARG1 "-std=c++11 -O2 -Wall")

find_package(Threads)

#set(INCLUDE_LIB ../driver lib util lib/pcie ../driver/pcie/common lib/pcie/include  lib/pcie/src )
#include_directories(../driver lib util lib/pcie ../driver/pcie/common lib/pcie/include  lib/pcie/src shell)
include_directories(../driver lib util ../driver/pcie ../driver/pcie/common ../driver/pcie/include  ../driver/pcie/src)



file(GLOB LIB_SOURCES lib/pcie/*.c lib/pcie/src/*.c lib/*.cc lib/*.c util/*.c)
add_library(mlib STATIC ${LIB_SOURCES})
file(GLOB LIB_HEADERS lib/*.h util/*.h)


file(GLOB shell_SOURCES
    shell/*.c
    shell/letter-shell/src/*.c
    shell/letter-shell/extensions/log/*.c
    )

add_executable(shell ${shell_SOURCES})
target_include_directories(shell PUBLIC
    shell
    shell/letter-shell/src
    shell/letter-shell/extensions/fs_support
    shell/letter-shell/extensions/log
    shell/letter-shell/extensions/shell_enhance
    )
target_link_libraries(shell mlib ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(shell PROPERTIES LINK_FLAGS "-T ../shell/shell.lds")

file(GLOB TEST_SOURCES test/*.cc)
add_executable(test ${TEST_SOURCES})
target_link_libraries(test mlib ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS mlib
        EXPORT mlibTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
install(FILES ${LIB_HEADERS} DESTINATION  ${CMAKE_INSTALL_PREFIX}/include/mt)
