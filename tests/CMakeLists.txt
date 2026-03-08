include(Catch)

function(add_catch2_test_target target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name}
        PRIVATE
        src_lib
        memory_lib
        log_lib
        Catch2::Catch2WithMain
        ${LIBS}
    )
    catch_discover_tests(${target_name})
endfunction()

add_catch2_test_target(test_noncopyable test_noncopyable.test.cpp)
add_catch2_test_target(test_timestamp  test_timestamp.test.cpp)
add_catch2_test_target(test_logger     test_logger.test.cpp)
