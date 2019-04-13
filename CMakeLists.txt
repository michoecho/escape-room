cmake_minimum_required (VERSION 3.1)
project (ESCAPE_ROOM C)

set(CMAKE_C_FLAGS "-O3 -Wall -Wextra")

add_executable(manager manager.c context.c bitset.c multiset.c ioutils.c)
target_link_libraries(manager rt pthread)

add_executable(player player.c context.c bitset.c multiset.c ioutils.c)
target_link_libraries(player rt pthread)

install(TARGETS DESTINATION .)
