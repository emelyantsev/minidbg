cmake_minimum_required (VERSION 3.0)
project (MiniDbg)

add_compile_options(-std=c++20)

include_directories(ext/libelfin ext/linenoise include)
add_executable(minidbg src/minidbg.cpp src/debugger.cpp src/debugger2.cpp src/helpers.cpp src/registers.cpp src/symbols.cpp ext/linenoise/linenoise.c)

add_executable(hello1 examples/hello1.cpp)
set_target_properties(hello1
                      PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(hello2 examples/hello2.cpp)
set_target_properties(hello2
                     PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(hello3 examples/hello3.cpp)
set_target_properties(hello3
                     PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(hello4 examples/hello4.cpp)
set_target_properties(hello4
                     PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(variable examples/variable.cpp)
set_target_properties(variable
                      PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(unwinding examples/stack_unwinding.cpp)
set_target_properties(unwinding
                      PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(test_hex examples/test_hex.cpp)
set_target_properties(test_hex
                     PROPERTIES COMPILE_FLAGS "-gdwarf-2  -O0")

add_executable(test_types examples/test_types.cpp)
set_target_properties(test_types
                     PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(test_attach examples/test_attach.cpp)
set_target_properties(test_attach
                     PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")
                     
add_custom_target(
   libelfin
   COMMAND make
   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/ext/libelfin
)

target_link_libraries(minidbg
                      ${PROJECT_SOURCE_DIR}/ext/libelfin/dwarf/libdwarf++.so
                      ${PROJECT_SOURCE_DIR}/ext/libelfin/elf/libelf++.so)

set_target_properties(minidbg PROPERTIES COMPILE_FLAGS "-g")

add_dependencies(minidbg libelfin)
