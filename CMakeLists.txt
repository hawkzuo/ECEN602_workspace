cmake_minimum_required(VERSION 3.8)
project(ECEN602_workspace)
set(CMAKE_CXX_STANDARD 11)

file( GLOB LIB_SOURCES HW4/HTTP.c  )
file( GLOB LIB_HEADERS HW4/HTTP.h )
add_library( Lib ${LIB_SOURCES} ${LIB_HEADERS} )

set(proxy_bonus_source
        HW4/ProxyBonus.c)
set(client_source
        HW4/Client.c)
set(proxy_basic_source
        HW4/ProxyBasic.c)

add_executable(client ${client_source})
target_link_libraries( client Lib )
add_executable(proxy ${proxy_bonus_source})
target_link_libraries( proxy Lib )
add_executable(proxyBasic ${proxy_basic_source})
target_link_libraries( proxyBasic Lib )
