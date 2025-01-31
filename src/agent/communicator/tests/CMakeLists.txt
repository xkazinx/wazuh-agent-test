find_package(GTest CONFIG REQUIRED)

add_executable(communicator_test communicator_test.cpp)
configure_target(communicator_test)
target_include_directories(communicator_test SYSTEM PRIVATE ${JWT_CPP_INCLUDE_DIRS})
target_compile_definitions(communicator_test PRIVATE -DJWT_DISABLE_PICOJSON=ON)
target_link_libraries(communicator_test PUBLIC Communicator GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
add_test(NAME CommunicatorTest COMMAND communicator_test)

add_executable(http_resolver_test http_resolver_test.cpp)
configure_target(http_resolver_test)
target_link_libraries(http_resolver_test PUBLIC Communicator GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
add_test(NAME HttpResolverTest COMMAND http_resolver_test)

add_executable(http_socket_test http_socket_test.cpp)
configure_target(http_socket_test)
target_link_libraries(http_socket_test PUBLIC Communicator GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main Logger)
add_test(NAME HttpSocketTest COMMAND http_socket_test)

