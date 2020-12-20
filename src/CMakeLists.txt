add_executable(spelcpp main.cpp)
target_include_directories(spelcpp PUBLIC ${LLVM_INCLUDE_DIRS}
                                          ${CLANG_INCLUDE_DIRS})
target_compile_definitions(spelcpp PUBLIC ${LLVM_DEFINITIONS}
                                          ${CLANG_DEFINITIONS})

target_link_libraries(spelcpp PUBLIC project_options project_warnings)
target_link_libraries(spelcpp PUBLIC clang-${LLVM_VERSION_MAJOR})
target_link_libraries(spelcpp PUBLIC fmt)
