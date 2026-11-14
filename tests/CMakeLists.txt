# Die Unittests holen GoogleTest per FetchContent ins Build-Verzeichnis.
# Version und Pruefsumme sind pinnt, damit der Build reproduzierbar bleibt.
# Ein bereits vorhandener Checkout kann ueber -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST
# eingebunden werden.

include(FetchContent)

set(FM_GOOGLETEST_VERSION "1.16.0")

FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v${FM_GOOGLETEST_VERSION}.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    URL_HASH SHA256=78c676fc63881529bf97bf9d45948d905a66833fbfa5318ea2cd7478cb98f399)

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(test_all
    src/main.cpp
    src/testhelper.cpp
    src/fontmaker_test.cpp)

target_include_directories(test_all PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_definitions(test_all PRIVATE
    FM_TESTDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/testdata"
    FM_TMP_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tmp"
    FM_TOOL="$<TARGET_FILE:pplfontmaker>")

target_link_libraries(test_all PRIVATE
    pplfontmaker_core
    GTest::gtest)

# Die Tests rufen das Programm selbst auf, also muss es gebaut sein.
add_dependencies(test_all pplfontmaker)

add_test(NAME unit COMMAND test_all
         WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
