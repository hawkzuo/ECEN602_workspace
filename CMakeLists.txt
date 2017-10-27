cmake_minimum_required(VERSION 3.8)
project(ECEN602_workspace)
set(CMAKE_CXX_STANDARD 11)

file( GLOB LIB_SOURCES HW3/TFTP.c  )
file( GLOB LIB_HEADERS HW3/TFTP.h )
add_library( Lib ${LIB_SOURCES} ${LIB_HEADERS} )

set(server_source
        HW3/Server.c)
set(client_source
        HW3/Client.c)

add_executable(Client ${client_source})
target_link_libraries( Client Lib )
add_executable(Server ${server_source})
target_link_libraries( Server Lib )
