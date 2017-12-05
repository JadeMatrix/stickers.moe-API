cmake_minimum_required( VERSION 3.9 )

set( CMAKE_CXX_STANDARD          11 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )


project(
    "stickers.moe API"
    CXX
)


find_library( PQXX_LIBRARY       pqxx       )
find_library( REDOX_LIBRARY      redox      )
find_library( FASTFORMAT_LIBRARY FastFormat )
find_library( CRYPTOPP_LIBRARY   cryptopp   )
find_library( TZ_LIBRARY         tz         )

add_executable(
    server
    src/server/main.cpp
    src/server/server.cpp
    src/server/routing.cpp
    src/server/handlers.cpp
    src/common/config.cpp
    src/common/timestamp.cpp
    src/common/user.cpp
    src/common/bigid.cpp
    src/common/postgres.cpp
    src/audit/util.cpp
)
target_link_libraries(
    server
    ${PQXX_LIBRARY}
    ${REDOX_LIBRARY}
    ${FASTFORMAT_LIBRARY}
    ${CRYPTOPP_LIBRARY}
    ${TZ_LIBRARY}
)

add_executable(
    dependency_test
    src/dependency_test.cpp
)
target_link_libraries(
    dependency_test
    ${PQXX_LIBRARY}
    ${REDOX_LIBRARY}
    ${FASTFORMAT_LIBRARY}
    ${CRYPTOPP_LIBRARY}
    ${TZ_LIBRARY}
)
