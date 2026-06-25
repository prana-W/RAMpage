cmake_minimum_required(VERSION 3.10)
project(RAMpage VERSION 1.0 LANGUAGES CXX)

# We are using std::variant, so C++17 is required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable aggressive warnings to catch bugs early
if(MSVC)
    # Windows/MSVC specific warnings
    add_compile_options(/W4)
else()
    # GCC/Clang specific warnings
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wunreachable-code)
endif()

# Create a core static library containing all the business logic
add_library(RAMpageCore
    src/database/Database.cpp
    src/commands/CommandRegistry.cpp
    src/commands/StringCommands.cpp
    src/commands/ListCommands.cpp
    src/server/Server.cpp
)

# Allow anything linking to RAMpageCore to easily #include from the src directory
target_include_directories(RAMpageCore PUBLIC src)

# -----------------------------------------------------
# 1. Main Executable (The CLI interface)
# -----------------------------------------------------
add_executable(rampage_cli src/rampage_cli.cpp)
target_link_libraries(rampage_cli PRIVATE RAMpageCore)

# -----------------------------------------------------
# 2. TCP Server Executable
# -----------------------------------------------------
add_executable(rampage src/main.cpp)
target_link_libraries(rampage PRIVATE RAMpageCore)

# -----------------------------------------------------
# 2. Test Executable (The Database tests)
# -----------------------------------------------------
add_executable(db_test tests/db.test.cpp)
target_link_libraries(db_test PRIVATE RAMpageCore)

# Register tests with CTest so you can run `ctest` or `make test`
enable_testing()
add_test(NAME DatabaseTest COMMAND db_test)
