cmake_minimum_required(VERSION 3.22)

project(Restart)

include(../../cmake/CommonSettings.cmake)
set_common_settings()

find_package(Boost REQUIRED COMPONENTS asio)
find_package(nlohmann_json REQUIRED)


if(WIN32)
    set(SOURCES src/restart_win.cpp)
else()
    set(SOURCES src/restart_unix.cpp)
endif()

add_library(Restart ${SOURCES})
target_include_directories(Restart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(Restart PUBLIC Boost::asio nlohmann_json::nlohmann_json CommandEntry PRIVATE Logger)

include(../../cmake/ConfigureTarget.cmake)
configure_target(Restart)

# if(BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()
