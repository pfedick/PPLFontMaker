cmake_minimum_required(VERSION 3.20)

project(pplfontmaker VERSION 1.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# pplib8 liegt im Homeverzeichnis des Benutzers (~/lib oder ~/lib64)
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/lib" "$ENV{HOME}/lib64")

find_package(pplib8 REQUIRED)
find_package(Freetype REQUIRED)

# Sanitizer (z.B. -DPPLFONTMAKER_SANITIZER=address,undefined), vom Makefile-Target
# "asan" verwendet.
set(PPLFONTMAKER_SANITIZER "" CACHE STRING "Kommagetrennte Liste der Sanitizer")
if(PPLFONTMAKER_SANITIZER AND NOT WIN32)
    add_compile_options(-fsanitize=${PPLFONTMAKER_SANITIZER} -fno-omit-frame-pointer -fno-sanitize-recover=all)
    add_link_options(-fsanitize=${PPLFONTMAKER_SANITIZER})
endif()

# Coverage-Auswertung mit gcovr (Target "coverage" im Makefile)
option(PPLFONTMAKER_ENABLE_COVERAGE "Code-Coverage messen" OFF)
if(PPLFONTMAKER_ENABLE_COVERAGE AND NOT WIN32)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# Unittests (GoogleTest, siehe tests/)
option(PPLFONTMAKER_BUILD_TESTS "Unittests bauen" OFF)

# Versions-Header in den Buildverzeichnis erzeugen
string(TIMESTAMP FM_BUILDDATE_INT "%Y%m%d" UTC)
string(TIMESTAMP FM_BUILDDATE_STRING "%d.%m.%Y" UTC)

set(FM_GIT_REVISION "0")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-list --count HEAD
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    OUTPUT_VARIABLE FM_GIT_REVISION
                    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/version.h" @ONLY)

# Kern ohne main(), damit die Unittests dieselben Klassen verwenden
add_library(pplfontmaker_core STATIC
    src/common.cpp
    src/font5maker.cpp
    src/font6maker.cpp)

target_include_directories(pplfontmaker_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_include_directories(pplfontmaker_core PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(pplfontmaker_core PUBLIC
    pplib::core
    Freetype::Freetype)

target_compile_definitions(pplfontmaker_core PUBLIC
    PPL_PROJECT_TYPE=1
    PPL_EXE=1
    PPL_PUBLISHER_ID=1)

add_executable(pplfontmaker src/main.cpp)

target_include_directories(pplfontmaker PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(pplfontmaker PRIVATE pplfontmaker_core)

install(TARGETS pplfontmaker RUNTIME DESTINATION bin)

if(PPLFONTMAKER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
