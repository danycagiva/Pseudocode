cmake_minimum_required(VERSION 3.16)
project(PseudoCode VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Quellen
add_executable(pseudocode
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/interpreter.cpp
)

target_include_directories(pseudocode PRIVATE src)

# Compiler-Warnungen und UTF-8
if(MSVC)
    target_compile_options(pseudocode PRIVATE /W4 /utf-8)
else()
    target_compile_options(pseudocode PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Windows: UTF-8 Konsolenausgabe
if(WIN32)
    target_compile_definitions(pseudocode PRIVATE _WIN32_WINNT=0x0601)
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS pseudocode DESTINATION ${CMAKE_INSTALL_BINDIR})
