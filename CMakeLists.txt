cmake_minimum_required(VERSION 3.10)
project(RAMpage VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wunreachable-code)
endif()

find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        "src/*.cpp" "src/*.h"
        "tests/*.cpp" "tests/*.h"
    )
    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT} -i -style=file ${ALL_CXX_SOURCE_FILES}
        COMMENT "Auto-formatting C++ files with clang-format"
    )
else()
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found. Skipping auto-formatting."
    )
endif()

add_library(RAMpageCore
    src/database/Database.cpp
    src/commands/CommandRegistry.cpp
    src/commands/StringCommands.cpp
    src/commands/ListCommands.cpp
    src/commands/PubSubCommands.cpp
    src/server/Server.cpp
    src/persistence/PersistenceManager.cpp
    src/protocol/RESPParser.cpp
    src/protocol/RESPSerializer.cpp
    src/pubsub/PubSubManager.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(RAMpageCore PUBLIC Threads::Threads)

add_dependencies(RAMpageCore format)

target_include_directories(RAMpageCore PUBLIC src)

add_executable(rampage_cli src/rampage_cli.cpp)
target_link_libraries(rampage_cli PRIVATE RAMpageCore)

add_executable(rampage src/main.cpp)
target_link_libraries(rampage PRIVATE RAMpageCore)

add_executable(db_test tests/db.test.cpp)
target_link_libraries(db_test PRIVATE RAMpageCore)

enable_testing()
add_test(NAME DatabaseTest COMMAND db_test)