add_library(cfga_lib cfga_lib.cpp)

add_executable(cfga CFGA.cpp)
target_link_libraries(cfga PRIVATE
        ${SVF_LIB}
        ${LLVM_LIB}
        cfga_lib
        )
set_target_properties(cfga PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

option(CFGA_BUILD_TESTS "Build the call/return path regression checker" OFF)
if (CFGA_BUILD_TESTS)
    add_executable(cfga_regression tests/check_paths.cpp)
    target_link_libraries(cfga_regression PRIVATE ${SVF_LIB} ${LLVM_LIB} cfga_lib)
endif ()
