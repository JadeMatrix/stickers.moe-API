CMAKE_MINIMUM_REQUIRED( VERSION 3.6 )

SET( CMAKE_C_COMPILER   /usr/local/Cellar/llvm/6.0.0/bin/clang   )
SET( CMAKE_CXX_COMPILER /usr/local/Cellar/llvm/6.0.0/bin/clang++ )

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
FIND_LIBRARY( SCRYPT_LIBRARY     scrypt     )

ADD_EXECUTABLE(
    server
    src/api/design.cpp
    src/api/list.cpp
    src/api/media.cpp
    src/api/person.cpp
    src/api/shop.cpp
    src/api/user.cpp
    src/common/auth.cpp
    src/common/bigid.cpp
    src/common/config.cpp
    src/common/document.cpp
    src/common/hashing.cpp
    src/common/jwt.cpp
    src/common/postgres.cpp
    src/common/sorting.cpp
    src/common/timestamp.cpp
    src/common/uuid.cpp
    src/handlers/auth.cpp
    src/handlers/design.cpp
    src/handlers/list.cpp
    src/handlers/media.cpp
    src/handlers/person.cpp
    src/handlers/product.cpp
    src/handlers/shop.cpp
    src/handlers/user.cpp
    src/server/main.cpp
    src/server/parse.cpp
    src/server/routing.cpp
    src/server/server.cpp
)
TARGET_LINK_LIBRARIES(
    server
    "-L/usr/local/Cellar/llvm/6.0.0/lib"
    "-lc++experimental"
    ${PQXX_LIBRARY}
    ${REDOX_LIBRARY}
    ${FASTFORMAT_LIBRARY}
    ${CRYPTOPP_LIBRARY}
    ${TZ_LIBRARY}
    ${CURL_LIBRARY}
    ${SCRYPT_LIBRARY}
)

ADD_EXECUTABLE(
    password_gen
    src/api/media.cpp
    src/api/user.cpp
    src/common/bigid.cpp
    src/common/config.cpp
    src/common/document.cpp
    src/common/hashing.cpp
    src/common/postgres.cpp
    src/common/timestamp.cpp
    src/common/uuid.cpp
    src/server/parse.cpp
    src/utilities/password_gen.cpp
)
TARGET_LINK_LIBRARIES(
    password_gen
    "-L/usr/local/Cellar/llvm/5.0.1/lib"
    "-lc++experimental"
    ${PQXX_LIBRARY}
    ${FASTFORMAT_LIBRARY}
    ${CRYPTOPP_LIBRARY}
    ${TZ_LIBRARY}
    ${CURL_LIBRARY}
    ${SCRYPT_LIBRARY}
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
    ${SCRYPT_LIBRARY}
)
