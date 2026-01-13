cmake_minimum_required(VERSION 3.10)
project(wisielec-online VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

set(SHARED_SOURCES

)

set(SERVER_SOURCES
    src/server.c
    ${SHARED_SOURCES}
)

set(CLIENT_SOURCES
    src/client.c   
    ${SHARED_SOURCES}
)

add_executable(serv ${SERVER_SOURCES})
add_executable(cli ${CLIENT_SOURCES})

