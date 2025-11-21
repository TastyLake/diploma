cmake_minimum_required(VERSION 3.15)

project(CppDiplomProject)

# Настройки для Windows
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A01)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Настройка C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск Boost
find_package(Boost 1.70.0 REQUIRED COMPONENTS system)

if(Boost_FOUND)
    message(STATUS "✅ Boost found: ${Boost_VERSION}")
else()
    message(FATAL_ERROR "❌ Could not find Boost")
endif()

# Поиск OpenSSL
find_package(OpenSSL REQUIRED)

# Поиск PostgreSQL
find_package(PostgreSQL REQUIRED)

# Поиск libpqxx
find_package(libpqxx REQUIRED)

add_subdirectory(spider)
add_subdirectory(http_server)