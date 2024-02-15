cmake_minimum_required(VERSION 2.4.4)
project(zlib-examples C)

add_executable(enough enough.c)
target_link_libraries(enough z)

add_executable(fitblk fitblk.c)
target_link_libraries(fitblk z)

add_executable(gun gun.c)
target_link_libraries(gun z)

add_executable(gzappend gzappend.c)
target_link_libraries(gzappend z)

add_executable(gzjoin gzjoin.c)
target_link_libraries(gzjoin z)

add_library(gzlog gzlog.h gzlog.c)
target_link_libraries(gzlog z)

add_executable(zpipe zpipe.c)
target_link_libraries(zpipe z)

add_executable(zran zran.c)
target_link_libraries(zran z)
