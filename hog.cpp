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
#include "hogreader.hpp"
#include "hogiterator.hpp"
#include "rdl.hpp"

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

    if (fseek(myFile, myChildFile.size, SEEK_CUR) != 0) return false;
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
    Left,
    Top,
    Right,
    Bottom,
    Back,
    Front
  };

  if (cube.neighbors[Left] == -1)
  {
    const Quad quad = { vertices[0], vertices[1], vertices[5], vertices[4] };
    quads->push_back(quad);
  }

  if (cube.neighbors[Top] == -1)
  {
    const Quad quad = { vertices[0], vertices[3], vertices[7], vertices[4] };
    quads->push_back(quad);
  }

  if (cube.neighbors[Right] == -1)
  {
    const Quad quad = { vertices[2], vertices[3], vertices[7], vertices[6] };
    quads->push_back(quad);
  }

  if (cube.neighbors[Bottom] == -1)
  {
    const Quad quad = { vertices[1], vertices[2], vertices[6], vertices[4] };
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

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    printf("usage: %s filename\n", argv[0]);
    return 1;
  }

  HogReader reader(argv[1]);
  if (!reader.IsValid())
  {
    fprintf(stderr, "error to open the hog file");
    return 1;
  }

  enum Mode
  {
    ListAllFiles,
    ExportToPly,
    Debug // Performs some other task during development.
  };

  const Mode mode = ExportToPly;
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

    // TODO: Provide a way to get to the file data from the iterator.
    const auto data = reader.CurrentFile();
    RdlReader rdlReader(data);
    const auto vertices = rdlReader.Vertices();
    const auto cubes = rdlReader.Cubes();
    const auto quads = Quads(cubes);

    const bool verticesOnly = false;

    std::cout << "ply" << std::endl;
    std::cout << "format ascii 1.0" << std::endl;
    std::cout << "comment An exported Descent 1 level (" << (*file).name << ")"
              << std::endl;

    std::cout << "element vertex " << vertices.size() << std::endl;
    std::cout << "property float x" << std::endl;
    std::cout << "property float y" << std::endl;
    std::cout << "property float z" << std::endl;
    if (!verticesOnly)
    {
      std::cout << "element face " << quads.size() << std::endl;
      std::cout << "property list uchar int vertex_index" << std::endl;
    }
    std::cout << "end_header" << std::endl;

    std::for_each(vertices.begin(), vertices.end(), [](Vertex v)
    { printf("%f %f %f\n", v.x, v.y, v.z); });

    if (!verticesOnly)
    {
      std::for_each(quads.begin(), quads.end(), [](const Quad& quad)
      { printf("4 %d %d %d %d\n", quad.a, quad.b, quad.c, quad.d); });
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
      printf("Vertex count: %d\n", vertices.size());
      std::for_each(vertices.begin(), vertices.end(),
                    [](Vertex v)
                    { printf("%16f %16f %16f\n", v.x, v.y, v.z); });
    });
  }
  return 0;
}
