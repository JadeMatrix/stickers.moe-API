cmake_minimum_required( VERSION 3.9 )

set( CMAKE_CXX_STANDARD          11 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )


project(
    "stickers.moe API"
    CXX
)


find_library( LIBPQXX_LIBRARY    pqxx       )
find_library( REDOX_LIBRARY      redox      )
find_library( FASTFORMAT_LIBRARY FastFormat )

add_executable(
    server
    src/server/main.cpp
    src/common/config.cpp
    src/common/user.cpp
    src/common/bigid.cpp
)
target_link_libraries(
    server
    ${LIBPQXX_LIBRARY}
    ${REDOX_LIBRARY}
    ${FASTFORMAT_LIBRARY}
)
