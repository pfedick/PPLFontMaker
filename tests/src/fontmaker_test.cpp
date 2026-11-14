// PPLFontMaker
// Copyright by Patrick Fedick, in 2026
//
// Tests auf der Ebene des fertigen Programms: Es wird ein Font aus dem
// Testfont erzeugt und die resulting Datei formatseitig geprueft.

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "testhelper.h"

namespace {

// Ein Face eindeutig ueber Groesse, Pixelformat und Stil beschreiben
struct FaceKey {
    int size;
    int pixelformat;
    int style;

    bool operator<(const FaceKey& other) const
    {
        if (size != other.size) {
            return size < other.size;
        }
        if (pixelformat != other.pixelformat) {
            return pixelformat < other.pixelformat;
        }
        return style < other.style;
    }

    bool operator==(const FaceKey& other) const
    {
        return size == other.size && pixelformat == other.pixelformat && style == other.style;
    }
};

std::set<FaceKey> toKeys(const std::vector<fmtest::FaceInfo>& faces)
{
    std::set<FaceKey> keys;
    for (const fmtest::FaceInfo& face : faces) {
        keys.insert(FaceKey{face.size, face.pixelformat, face.style});
    }
    return keys;
}

std::vector<std::string> sizesOf(const std::vector<fmtest::FaceInfo>& faces)
{
    std::vector<std::string> sizes;
    for (const fmtest::FaceInfo& face : faces) {
        sizes.push_back(std::to_string(face.size));
    }
    std::sort(sizes.begin(), sizes.end());
    return sizes;
}

class FontMakerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        fmtest::deleteFile(derFont);
    }

    void TearDown() override
    {
        fmtest::deleteFile(derFont);
    }

    // Führt das Programm aus und prueft, dass es erfolgreich war.
    int ausgefuehrt(const std::vector<std::string>& argumente, std::string* ausgabe = nullptr)
    {
        std::vector<std::string> komplette = argumente;
        komplette.push_back("-q");
        komplette.push_back(quellfont);
        komplette.push_back("-t");
        komplette.push_back(derFont);
        int code = fmtest::runTool(komplette, ausgabe);
        return code;
    }

    // Liest die fertige Datei ein und liefert die Chunks.
    void eingelesen(std::vector<fmtest::PfpChunk>& chunks, fmtest::PfpHeader& header, int erwarteteVersion)
    {
        std::string grund;
        ASSERT_TRUE(fmtest::parsePfpFile(fmtest::readFile(derFont), header, chunks, grund)) << grund;
        EXPECT_EQ(header.fileID, "FONT");
        EXPECT_EQ(header.majorVersion, erwarteteVersion);
    }

    // Liest die Datei, prueft alle Faces und liefert Probleme zurueck.
    std::vector<fmtest::FaceInfo> facesPruefen(int erwarteteVersion, std::vector<std::string>& probleme)
    {
        std::vector<fmtest::PfpChunk> chunks;
        fmtest::PfpHeader header;
        eingelesen(chunks, header, erwarteteVersion);
        std::vector<fmtest::FaceInfo> faces;
        checkFaceChunkNamen(chunks);
        fmtest::checkFaces(chunks, erwarteteVersion, faces, probleme);
        for (const std::string& problem : probleme) {
            ADD_FAILURE() << problem;
        }
        return faces;
    }

    static void checkFaceChunkNamen(const std::vector<fmtest::PfpChunk>& chunks)
    {
        size_t anzahl = 0;
        for (const fmtest::PfpChunk& chunk : chunks) {
            if (chunk.name == "FACE") {
                anzahl++;
            } else if (chunk.name != "NAME" && chunk.name != "AUTH" && chunk.name != "COPY" && chunk.name != "DESC") {
                ADD_FAILURE() << "unerwarteter Chunk '" << chunk.name << "'";
            }
        }
        EXPECT_GT(anzahl, 0u);
    }

    std::string quellfont = fmtest::testDataPath("TestSans.ttf");
    std::string derFont = fmtest::tmpFilePath("erzeugt.pfp");
};

// Mehrere Groessen und Pixelformate ergeben ein Face je Kombination, und die
// Datei enthaelt die erwarteten Text-Chunks.
TEST_F(FontMakerTest, MehrereGroessenUndFormateErgebenEinFaceJeKombination)
{
    std::string ausgabe;
    ASSERT_EQ(ausgefuehrt({"-6", "-s", "8,12", "--mono1", "--name", "TestSans", "--author", "PF"}, &ausgabe), 0) << ausgabe;
    ASSERT_EQ(ausgefuehrt({"-6", "-s", "8,12", "--aa4"}, &ausgabe), 0) << ausgabe;

    std::vector<std::string> probleme;
    std::vector<fmtest::FaceInfo> faces = facesPruefen(6, probleme);
    ASSERT_EQ(faces.size(), 4u);
    EXPECT_EQ(toKeys(faces), (std::set<FaceKey>{{8, 2, 0}, {8, 5, 1}, {12, 2, 0}, {12, 5, 1}}));

    std::vector<fmtest::PfpChunk> chunks;
    fmtest::PfpHeader header;
    eingelesen(chunks, header, 6);

    const fmtest::PfpChunk* name = fmtest::findChunk(chunks, "NAME");
    ASSERT_TRUE(name != nullptr);
    // Ein String-Chunk endet immer mit einem 0-Byte, auch wenn der String
    // per --name gesetzt wurde.
    ASSERT_FALSE(name->data.empty());
    EXPECT_EQ(name->data.back(), 0);
    EXPECT_EQ(std::string(name->data.begin(), name->data.end() - 1), "TestSans");

    const fmtest::PfpChunk* author = fmtest::findChunk(chunks, "AUTH");
    ASSERT_TRUE(author != nullptr);
    EXPECT_EQ(std::string(author->data.begin(), author->data.end() - 1), "PF");
}

// Ein Groessendurchlauf mit gemischter Liste aus Einzelwerten und Bereichen.
TEST_F(FontMakerTest, GemischteGroessenlisteWirdAUFgeloest)
{
    ASSERT_EQ(ausgefuehrt({"-6", "-s", "8,10-13", "--mono1"}), 0);
    std::vector<std::string> probleme;
    std::vector<fmtest::FaceInfo> faces = facesPruefen(6, probleme);
    EXPECT_EQ(sizesOf(faces), (std::vector<std::string>{"10", "11", "12", "13", "8"}));
}

// Wer eine bereits vorhandene Groesse erneut erzeigt, bekommt kein zweites Face.
TEST_F(FontMakerTest, ErneutesErzeugenErsetztDasVorhandeneFace)
{
    ASSERT_EQ(ausgefuehrt({"-6", "-s", "8", "--mono1"}), 0);
    ASSERT_EQ(ausgefuehrt({"-6", "-s", "8", "--mono1"}), 0);

    std::vector<std::string> probleme;
    std::vector<fmtest::FaceInfo> faces = facesPruefen(6, probleme);
    ASSERT_EQ(faces.size(), 1u);
}

// --genbold erzeugt ein eigenes fettes Face, das normale bleibt erhalten.
TEST_F(FontMakerTest, GeneriertesFettesFaceKommtHinzu)
{
    ASSERT_EQ(ausgefuehrt({"-6", "-s", "8", "--aa8"}), 0);
    ASSERT_EQ(ausgefuehrt({"-6", "-s", "8", "--aa8", "--genbold"}), 0);
    std::vector<std::string> probleme;
    std::vector<fmtest::FaceInfo> faces = facesPruefen(6, probleme);
    EXPECT_EQ(toKeys(faces), (std::set<FaceKey>{{8, 3, 1}, {8, 3, 3}}));
}

// -r loescht nur die Faces, die Groesse, Pixelformat und Stil treffen.
TEST_F(FontMakerTest, LoeschtNurPassendeFaces)
{
    ASSERT_EQ(ausgefuehrt({"-6", "-s", "8,12", "--mono1"}), 0);
    ASSERT_EQ(ausgefuehrt({"-6", "-s", "8,12", "--aa4"}), 0);
    std::string ausgabe;
    ASSERT_EQ(fmtest::runTool({"-e", derFont, "-r", "-6", "-s", "8", "--mono1"}, &ausgabe), 0) << ausgabe;

    std::vector<std::string> probleme;
    EXPECT_EQ(toKeys(facesPruefen(6, probleme)), (std::set<FaceKey>{{8, 5, 1}, {12, 2, 0}, {12, 5, 1}}));

    ASSERT_EQ(fmtest::runTool({"-e", derFont, "-r", "-6", "-s", "8", "--aa4"}, &ausgabe), 0) << ausgabe;
    probleme.clear();
    EXPECT_EQ(toKeys(facesPruefen(6, probleme)), (std::set<FaceKey>{{12, 2, 0}, {12, 5, 1}}));
}

// Font5 nutzt Sprungtabellen und ein Zeigerfeld statt Glyph-Chunks.
TEST_F(FontMakerTest, Font5HatSprungtabellenUndKorrekteGlyphZeiger)
{
    std::string ausgabe;
    ASSERT_EQ(ausgefuehrt({"-5", "-s", "12", "--aa4"}, &ausgabe), 0) << ausgabe;

    std::vector<fmtest::PfpChunk> chunks;
    fmtest::PfpHeader header;
    eingelesen(chunks, header, 5);

    std::vector<fmtest::FaceInfo> faces;
    std::vector<std::string> probleme;
    fmtest::checkFaces(chunks, 5, faces, probleme);
    for (const std::string& problem : probleme) {
        ADD_FAILURE() << problem;
    }
    ASSERT_EQ(faces.size(), 1u);
    EXPECT_EQ(faces[0].size, 12);
    EXPECT_EQ(faces[0].pixelformat, 5);
    EXPECT_GT(faces[0].jumptables, 0);
}

// Unicode-Werte oberhalb der Unicode-Ebene 0 koennen nicht dargestellt werden.
TEST_F(FontMakerTest, CodepointsOberhalbDerBasisebeneWerdenAbgelehnt)
{
    std::string ausgabe;
    int code = ausgefuehrt({"-6", "-s", "12", "--mono1", "-c", "70000-70010"}, &ausgabe);
    EXPECT_NE(code, 0);
    EXPECT_NE(ausgabe.find("65535"), std::string::npos) << ausgabe;
}

// Ein Bereich ohne Zeichen im Font erzeugt kein Face mit Muellwerten.
TEST_F(FontMakerTest, BereichOhneZeichenErzeugtKeinFace)
{
    std::string ausgabe;
    ausgefuehrt({"-6", "-s", "12", "--mono1", "-c", "57344-57599"}, &ausgabe);
    EXPECT_NE(ausgabe.find("keine Glyphen"), std::string::npos) << ausgabe;

    std::vector<fmtest::PfpChunk> chunks;
    fmtest::PfpHeader header;
    eingelesen(chunks, header, 6);
    EXPECT_EQ(fmtest::findChunk(chunks, "FACE"), nullptr);
}

// Sich gegenseitige ausschliessende Pixelformate sind ein Fehler vor der Arbeit.
TEST_F(FontMakerTest, WiderspruechlichePixelformateWerdenAbgelehnt)
{
    std::string ausgabe;
    int code = ausgefuehrt({"-6", "-s", "12", "--aa2", "--aa8"}, &ausgabe);
    EXPECT_NE(code, 0);
    EXPECT_NE(ausgabe.find("nicht gemeinsam"), std::string::npos) << ausgabe;
}

}  // namespace
