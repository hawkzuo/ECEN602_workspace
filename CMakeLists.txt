cmake_minimum_required(VERSION 3.8)
project(ECEN602_workspace)
set(CMAKE_CXX_STANDARD 11)

file( GLOB LIB_SOURCES HW3/TFTP.c  )
file( GLOB LIB_HEADERS HW3/TFTP.h )
add_library( Lib ${LIB_SOURCES} ${LIB_HEADERS} )

set(server_source
        HW3/Server.c)
set(parser_source
        HW3/Parser.c)

add_executable(Parser ${parser_source})
target_link_libraries( Parser Lib )
add_executable(Server ${server_source})
target_link_libraries( Server Lib )
