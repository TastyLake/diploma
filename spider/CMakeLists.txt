cmake_minimum_required(VERSION 3.15)

project(SpiderApp)

add_executable(SpiderApp
    main.cpp
    http_utils.cpp
    html_parser.cpp
    database.cpp
    config.cpp
)

target_compile_features(SpiderApp PRIVATE cxx_std_20)

target_link_libraries(SpiderApp 
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    PostgreSQL::PostgreSQL
    libpqxx::pqxx
)

target_include_directories(SpiderApp PRIVATE 
    ${Boost_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
)

# Для Windows
if(WIN32)
    target_link_libraries(SpiderApp ws2_32 crypt32)
endif()