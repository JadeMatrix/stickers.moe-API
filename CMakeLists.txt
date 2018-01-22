CMAKE_MINIMUM_REQUIRED( VERSION 3.6 )

SET( CMAKE_C_COMPILER   /usr/local/Cellar/llvm/5.0.1/bin/clang   )
SET( CMAKE_CXX_COMPILER /usr/local/Cellar/llvm/5.0.1/bin/clang++ )

SET( CMAKE_CXX_STANDARD          17 )
SET( CMAKE_CXX_STANDARD_REQUIRED ON )


PROJECT(
    "stickers.moe API"
    CXX
)


FIND_LIBRARY( PQXX_LIBRARY       pqxx       )
FIND_LIBRARY( REDOX_LIBRARY      redox      )
FIND_LIBRARY( FASTFORMAT_LIBRARY FastFormat )
FIND_LIBRARY( CRYPTOPP_LIBRARY   cryptopp   )
FIND_LIBRARY( TZ_LIBRARY         tz         )
FIND_LIBRARY( CURL_LIBRARY       curl       )

ADD_EXECUTABLE(
    server
    src/server/main.cpp
    src/server/server.cpp
    src/server/routing.cpp
    src/handlers/user.cpp
    src/common/config.cpp
    src/common/timestamp.cpp
    src/common/bigid.cpp
    src/common/postgres.cpp
    src/common/hashing.cpp
    src/api/user.cpp
)
TARGET_LINK_LIBRARIES(
    server
    "-L/usr/local/Cellar/llvm/5.0.1/lib"
    ${PQXX_LIBRARY}
    ${REDOX_LIBRARY}
    ${FASTFORMAT_LIBRARY}
    ${CRYPTOPP_LIBRARY}
    ${TZ_LIBRARY}
    ${CURL_LIBRARY}
)

ADD_EXECUTABLE(
    dependency_test
    src/dependency_test.cpp
)
TARGET_LINK_LIBRARIES(
    dependency_test
    ${PQXX_LIBRARY}
    ${REDOX_LIBRARY}
    ${FASTFORMAT_LIBRARY}
    ${CRYPTOPP_LIBRARY}
    ${TZ_LIBRARY}
    ${CURL_LIBRARY}
)
