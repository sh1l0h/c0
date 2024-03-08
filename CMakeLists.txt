cmake_minimum_required(VERSION 3.12)
project(c0 LANGUAGES C)

add_executable(${PROJECT_NAME} 
    src/main.c
    src/lexer.c
    src/token.c
    src/ast.c
    src/parser.c
    src/io/log.c    
    src/data_structures/cyclic_queue.c
    src/data_structures/array_list.c
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -g)

target_link_libraries(${PROJECT_NAME} PRIVATE m)
