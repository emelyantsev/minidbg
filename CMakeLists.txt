cmake_minimum_required (VERSION 3.0)
project (MiniDbg)

add_compile_options(-std=c++14)

include_directories(ext/libelfin ext/linenoise include)
add_executable(minidbg src/minidbg.cpp ext/linenoise/linenoise.c)

add_executable(hello examples/hello.cpp)
set_target_properties(hello
                      PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(variable examples/variable.cpp)
set_target_properties(variable
                      PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(unwinding examples/stack_unwinding.cpp)
set_target_properties(unwinding
                      PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(hello2 examples/hello2.cpp)
set_target_properties(hello2
                      PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(test_hex examples/test_hex.cpp)
set_target_properties(test_hex
                     PROPERTIES COMPILE_FLAGS "-gdwarf-2")

add_executable(test_types examples/test_types.cpp)
set_target_properties(test_types
                     PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")
                     
add_custom_target(
   libelfin
   COMMAND make
   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/ext/libelfin
)
target_link_libraries(minidbg
                      ${PROJECT_SOURCE_DIR}/ext/libelfin/dwarf/libdwarf++.so
                      ${PROJECT_SOURCE_DIR}/ext/libelfin/elf/libelf++.so)

add_dependencies(minidbg libelfin)
