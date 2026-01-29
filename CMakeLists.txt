cmake_minimum_required(VERSION 3.10)
project(wisielec-online VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

set(SHARED_SOURCES
    src/net.c
)

set(SERVER_SOURCES
    src/server.c
    src/auth.c
    src/server_utils.c
    ${SHARED_SOURCES}
)

set(CLIENT_SOURCES
    src/client_utils.c
    src/client.c
    ${SHARED_SOURCES}
)

add_executable(serv ${SERVER_SOURCES})
add_executable(cli ${CLIENT_SOURCES})

