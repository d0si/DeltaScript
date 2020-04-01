cmake_minimum_required(VERSION 3.10)
project(DeltaScript
    LANGUAGES CXX
    VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(DELTASCRIPT_SOURCES
    DeltaScript/Context.cpp
    DeltaScript/Engine.cpp

    DeltaScript/Compiler/Parser.cpp
    DeltaScript/Compiler/Lexer.cpp

    DeltaScript/Exception/DeltaScriptException.cpp
)

add_library(${PROJECT_NAME} SHARED
    ${DELTASCRIPT_SOURCES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER DeltaScript.h)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
