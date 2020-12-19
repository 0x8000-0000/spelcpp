llvm_map_components_to_libnames(llvm_libs support core irreader)

add_executable(spelcpp main.cpp)
target_include_directories(spelcpp PUBLIC ${LLVM_INCLUDE_DIRS})

target_link_libraries(spelcpp PUBLIC project_options project_warnings)
target_link_libraries(spelcpp PUBLIC ${llvm_libs})
target_link_libraries(spelcpp PUBLIC fmt)
