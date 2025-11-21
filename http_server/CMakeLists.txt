cmake_minimum_required(VERSION 3.15)

project(HttpServerApp)

add_executable(HttpServerApp 
    main.cpp
    http_connection.cpp
    database.cpp
    config.cpp
)

target_compile_features(HttpServerApp PRIVATE cxx_std_20)

target_link_libraries(HttpServerApp 
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    PostgreSQL::PostgreSQL
    libpqxx::pqxx
)

target_include_directories(HttpServerApp PRIVATE 
    ${Boost_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
)

# Для Windows
if(WIN32)
    target_link_libraries(HttpServerApp ws2_32 crypt32)
endif()