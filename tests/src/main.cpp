// PPLFontMaker
// Copyright by Patrick Fedick, in 2026
//
// Einstiegspunkt der Unittests

#include <filesystem>

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // tests/tmp wird zur Laufzeit mit temporären Daten gefüllt
    std::filesystem::create_directories(std::filesystem::path(FM_TMP_DIR));

    return RUN_ALL_TESTS();
}
