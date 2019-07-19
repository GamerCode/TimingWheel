macro(add_example_target EXAMPLE_TARGET)

    # 添加头文件路径
    include_directories("${PROJECT_SOURCE_DIR}/src")
    LINK_DIRECTORIES("${PROJECT_BINARY_DIR}/lib")

    # 添加 math 子目录
    add_executable("example_${EXAMPLE_TARGET}" "example_${EXAMPLE_TARGET}.cpp" )
    target_link_libraries("example_${EXAMPLE_TARGET}" tw pthread dl)
    add_definitions("-Wall -g")

endmacro(add_example_target)


# examples
add_example_target(dlink)
add_example_target(tw)