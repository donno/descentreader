//===----------------------------------------------------------------------===//
//
//                     The Descent map loader
//
// NAME         : Hog
// PURPOSE      : Providing a decoder and wrapper for the Descent .HOG format.
// COPYRIGHT    : (c) 2011 Sean Donnellan. All Rights Reserved.
// AUTHORS      : Sean Donnellan (darkdonno@gmail.com)
// DESCRIPTION  : A decoder for the HOG file format that is used by Parallax
//                Software in the computer game, Descent.
//
//                The file format is as follows:
//
//                - Magic number which is 3 bytes and corresponding to the
//                  string "DHF"

//                - A series of files which are preceded by a short header,
//                  which describes the name of the file and the numer of bytes.
//
//                 | "DHF" - 3 bytes
//                 |---------------- Start of the first file
//                 | filename - 13 bytes
//                 | size - 4 bytes
//                 | data - the size of this part is by the size before it.
//                 |---------------- The next header/file comes straight after.
//                 | filename - 13 bytes
//                 | size - 4 bytes
//                 | data - the size of this part is by the size before it.
//
//===----------------------------------------------------------------------===//

/////
// Developer notes
// Different file formats: http://www.descent2.com/ddn/kb/files/
//
/////

#include "cube.hpp"
#include "hogiterator.hpp"
#include "hogreader.hpp"
#include "rdl.hpp"
#include "txbiterator.hpp"
#include "txbreader.hpp"

#include <fstream>
#include <memory>
#include <iterator>
#include <utility>
#include <vector>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
static_assert(sizeof(uint8_t) == 1,
              "The size of uint8_t is incorrect it must be 1-byte");
static_assert(sizeof(uint32_t) == 4,
              "The size of uint32_t is incorrect it must be 4-bytes");
static_assert(sizeof(uint8_t) == sizeof(char),
              "The size of a char must be 1-byte");
#endif

// The 3-byte MAGIC number at the start of the file format used to identifiy the
// file as being a Descent HOG file.
static uint8_t magic[3] = { 'D', 'H', 'F' };

void ExtractTxb(const TxbReader& Reader,
                const std::string& Name,
                std::ostream& Output)
{
  std::copy(Reader.begin(), Reader.end(), std::ostream_iterator<char>(Output));
}

HogReader::iterator HogReader::begin()
{
  // Sync back up to the start just after the magic number.
  if (IsValid())
  {
    fseek(myFile, sizeof(magic), SEEK_SET);
    if (fread(&myChildFile.name, 13, 1, myFile) != 1) HogReaderIterator();
    if (fread(&myChildFile.size, 4, 1, myFile) != 1) HogReaderIterator();
  }

  return HogReaderIterator(*this);
}

HogReader::iterator HogReader::end()
{
  return HogReaderIterator();
}

HogReader::HogReader(const char* filename) : myFile(nullptr), hasReadFile(false)
{
  myFile = fopen(filename, "rb");
  myChildFile.name[0] = '\0';
  myChildFile.size = 0;
  if (!myFile) return;

  const size_t count = 1;
  if (fread(myHeader, sizeof(myHeader), count, myFile) != count)
  {
    myHeader[0] = '\0'; // Failed to load.
    return;
  }

  // Read in the header for the first file.
  if (IsValid())
  {
    if (fread(&myChildFile.name, 13, 1, myFile) != 1) return;
    if (fread(&myChildFile.size, 4, 1, myFile) != 1) return;
  }
}

HogReader::~HogReader()
{
  if (myFile) fclose(myFile);
}

bool HogReader::IsValid() const
{
  if (!myFile) return false;
  return memcmp(myHeader, magic, 3) == 0;
}

bool HogReader::NextFile()
{
  // Skip the current file.
  if (feof(myFile)) return false;

  if (!hasReadFile)
  {
    // The data for the current file has not been read so skip over the data
    // section
    // for the file.

    if (fseek(myFile, static_cast<long>(myChildFile.size), SEEK_CUR) != 0)
    {
      return false;
    }
  }

  // Read in the header for the next file.
  if (fread(&myChildFile.name, 13, 1, myFile) != 1) return false;
  if (fread(&myChildFile.size, 4, 1, myFile) != 1) return false;

  hasReadFile = false; // We haven't read the next file yet.
  return true;
}

const char* HogReader::CurrentFileName() const
{
  return myChildFile.name;
}

unsigned int HogReader::CurrentFileSize() const
{
  return myChildFile.size;
}

std::vector<uint8_t> HogReader::CurrentFile()
{
  std::vector<uint8_t> fileData;

  // TODO: Allow this to seek-back and read it again.
  //
  // For now ensure the current file hasn't been read yet.
  assert(!hasReadFile);

  // Skip the current file.
  if (feof(myFile)) return fileData;

  const unsigned int size = CurrentFileSize(); // Size in bytes
  fileData.resize(size);

  if (fread(fileData.data(), size, 1, myFile) != 1)
  {
    fileData.clear();
  }
  else
  {
    hasReadFile = true;
  }

  return fileData;
}

#include <algorithm>
#include <iostream>
#include <string>

struct Quad
{
  size_t a;
  size_t b;
  size_t c;
  size_t d;
};

void Quads(const Cube& cube, std::vector<Quad>* quads)
{
  const uint16_t* const vertices = cube.vertices;

  // Vertices:
  // 0 - left, front, top
  // 1 - left, front, bottom
  // 2 - right, front, bottom
  // 3 - right, front, top
  // 4 - left, back, top
  // 5 - left, back, bottom
  // 6 - right, back, bottom
  // 7 - right, back, top

  // Neighbours:
  enum Neighbour
  {
    Right,
    Top,
    Left,
    Bottom,
    Back,
    Front
  };

  if (cube.neighbors[Right] == -1)
  {
    const Quad quad = { vertices[2], vertices[3], vertices[7], vertices[6] };
    quads->push_back(quad);
  }

  if (cube.neighbors[Top] == -1)
  {
    const Quad quad = { vertices[0], vertices[3], vertices[7], vertices[4] };
    quads->push_back(quad);
  }

  if (cube.neighbors[Left] == -1)
  {
    const Quad quad = { vertices[0], vertices[1], vertices[5], vertices[4] };
    quads->push_back(quad);
  }

  if (cube.neighbors[Bottom] == -1)
  {
    const Quad quad = { vertices[1], vertices[2], vertices[6], vertices[5] };
    quads->push_back(quad);
  }

  if (cube.neighbors[Front] == -1)
  {
    const Quad quad = { vertices[0], vertices[1], vertices[2], vertices[3] };
    quads->push_back(quad);
  }

  if (cube.neighbors[Back] == -1)
  {
    const Quad quad = { vertices[4], vertices[5], vertices[6], vertices[7] };
    quads->push_back(quad);
  }
}

std::vector<Quad> Quads(const std::vector<Cube>& Cubes)
{
  // Generate quads from the sides of the cubes.
  std::vector<Quad> quads;
  for (auto cube = Cubes.cbegin(), cubeEnd = Cubes.cend(); cube != cubeEnd;
       ++cube)
  {
    Quads(*cube, &quads);
  }
  return quads;
}

void ExportToPly(const RdlReader& Reader, const std::string& Name,
                 std::ostream& Output)
{
  const auto vertices = Reader.Vertices();
  const auto cubes = Reader.Cubes();
  const auto quads = Quads(cubes);

  const bool verticesOnly = false;

  Output << "ply" << std::endl;
  Output << "format ascii 1.0" << std::endl;
  Output << "comment An exported Descent 1 level (" << Name << ")" << std::endl;

  Output << "element vertex " << vertices.size() << std::endl;
  Output << "property float x" << std::endl;
  Output << "property float y" << std::endl;
  Output << "property float z" << std::endl;
  if (!verticesOnly)
  {
    Output << "element face " << quads.size() << std::endl;
    Output << "property list uchar int vertex_index" << std::endl;
  }
  Output << "end_header" << std::endl;

  std::for_each(vertices.begin(), vertices.end(), [&Output](Vertex v)
  { Output << v.x << " " << v.y << " " << v.z << std::endl; });

  if (!verticesOnly)
  {
    std::for_each(quads.begin(), quads.end(), [&Output](const Quad& quad)
    {
      Output << "4 " << quad.a << " " << quad.b << " " << quad.c << " "
             << quad.d << std::endl;
    });
  }
}

int main(int argc, char* argv[])
{
  if (argc != 2 && argc != 3)
  {
    printf("usage: %s [-d -l -p -a] filename\n", argv[0]);
    return 1;
  }

  enum Mode
  {
    ListAllFiles,
    ExportToPly,
    ExportAllToPly,
    ExportAllText,
    ExtractAll, // This extracts it as-is no decoding.
    Debug // Performs some other task during development.
  };

  Mode mode = ExportToPly;
  bool filenameIsFirst = true;

  // Command line option parsing
  if ((argv[1] && argv[1][0] == '-') ||
      (argc == 3 && argv[2] && argv[2][0] == '-'))
  {
    filenameIsFirst = (argv[1][0] != '-');
    const char option = filenameIsFirst ? argv[2][1] : argv[1][1];
    switch (option)
    {
    default:
      fprintf(stderr, "error unsupported option provided (%c)", option);
      return 1;
    case '\0':
      fprintf(stderr, "error option specifier - provided but no option");
      return 1;
    case 'd':
      mode = Debug;
      break;
    case 'l':
      mode = ListAllFiles;
      break;
    case 'p':
      mode = ExportToPly;
      break;
    case 'a':
      mode = ExportAllToPly;
      break;
    case 't':
      mode = ExportAllText;
      break;
    case 'x':
      mode = ExtractAll;
      break;
    }

    if (argc == 2)
    {
      fprintf(stderr, "option provided but no filename");
      return 1;
    }
  }

  HogReader reader(filenameIsFirst ? argv[1] : argv[2]);
  if (!reader.IsValid())
  {
    fprintf(stderr, "error to open the hog file");
    return 1;
  }

  if (mode == ListAllFiles)
  {
    printf("%-13s Size\n", "Name");
    printf("=====================\n");
    std::for_each(reader.begin(), reader.end(),
                  [](HogReader::iterator::value_type item)
    { printf("%-13s %d\n", item.name, item.size); });
  }
  else if (mode == ExportToPly)
  {
    auto file = std::find_if(reader.begin(), reader.end(),
                             [](const HogReader::iterator::value_type & v)->bool
    { return strcmp(v.name, "level02.rdl") == 0; });

    const auto data = file.FileContents();
    RdlReader rdlReader(data);
    ::ExportToPly(rdlReader, std::string(file->name), std::cout);
  }
  else if (mode == ExportAllToPly)
  {
    for (auto file = reader.begin(), end = reader.end(); file != end; ++file)
    {
      const std::string name(file->name);
      if (name.length() < 4) continue;
      if (name.substr(name.length() - 4) != ".rdl") continue;

      const auto data = file.FileContents();
      RdlReader rdlReader(data);

      const std::string ply = name.substr(0, name.length() - 4) + ".ply";
      std::cout << "Writing out " << ply << std::endl;
      std::ofstream output(ply.c_str());
      ::ExportToPly(rdlReader, name, output);
    }
  }
  else if (mode == ExportAllText)
  {
    for (auto file = reader.begin(), end = reader.end(); file != end; ++file)
    {
      const std::string name(file->name);
      if (name.length() < 4) continue;
      if (name.substr(name.length() - 4) != ".txb") continue;

      const auto data = file.FileContents();

      TxbReader txbReader(data);

      const std::string txt = name.substr(0, name.length() - 4) + ".txt";
      std::cout << "Writing out " << txt << std::endl;
      std::ofstream output(txt.c_str());
      ::ExtractTxb(txbReader, name, output);
    }
  }
  else if (mode == ExtractAll)
  {
    for (auto file = reader.begin(), end = reader.end(); file != end; ++file)
    {
      const auto data = file.FileContents();

      std::cout << "Writing out " << file->name << std::endl;
      std::ofstream output(file->name);
      std::copy(data.begin(), data.end(),
                std::ostream_iterator<uint8_t>(output));
    }
  }
  else
  {
    // Iterate over the file list and list all the files which do not
    // end in the 'rdl' file extension.
    const std::string extentionRdl(".rdl");
    std::for_each(reader.begin(), reader.end(),
                  [&reader, &extentionRdl](HogReader::iterator::value_type n)
    {
      const std::string filename(n.name);
      if (!std::equal(extentionRdl.rbegin(), extentionRdl.rend(),
                      filename.rbegin()))
        return;

      printf("File: %s Size: %d\n", n.name, reader.CurrentFileSize());
      const auto data = reader.CurrentFile();
      RdlReader rdlReader(data);

      if (!rdlReader.IsValid()) return;

      // Print out the vertices.
      const auto vertices = rdlReader.Vertices();
      printf("Vertex count: %zd\n", vertices.size());
      std::for_each(vertices.begin(), vertices.end(),
                    [](Vertex v)
                    { printf("%16f %16f %16f\n", v.x, v.y, v.z); });
    });
  }
  return 0;
}
