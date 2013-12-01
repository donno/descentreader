//===----------------------------------------------------------------------===//
//
//                     The Descent map loader
//
// NAME         : Rdl
// PURPOSE      : Providing a decoder and wrapper for the Descent .RDL format.
// COPYRIGHT    : (c) 2011 Sean Donnellan. All Rights Reserved.
// AUTHORS      : Sean Donnellan (darkdonno@gmail.com)
// DESCRIPTION  : A decoder for the RDL file format that is used by Parallax
//                Software in the computer game, Descent.
//
//                The file format is as follows:
//
//                - Magic number which is 4 bytes and corresponding to the
//                  string "LVLP"
//
//                - TODO: Document the rest of the format.
//
//===----------------------------------------------------------------------===//

#include "rdl.hpp"

#include "arrayreader.hpp"
#include "cube.hpp"

#include <assert.h>
#include <string.h>
#include <stdio.h>

// TODO: Refactor this out.
inline double fixedToFloating(int32_t value)
{
  return value / 65536.0;
}
inline double fixedToFloating(int16_t value)
{
  return value / 4096.0;
}

inline void printBitmask(uint8_t bitmask)
{
  for (uint8_t j = 0; j<8; ++j, bitmask = bitmask>> 1)
  {
    putchar((bitmask & 1) ? '1' : '0');
  }
  putchar('\n');
}

// The 4-byte MAGIC number at the start of the file format used to identifiy the
// file as being a Descent HOG file.
static uint8_t magicRdl[4] = { 'L', 'V', 'L', 'P' };

struct RdlHeader
{
  uint8_t signature[4];
  uint32_t version;
  uint32_t mineDataOffset;
  uint32_t objectsOffset;
  uint32_t fileSize;
};

struct RdlTexture
{
  uint16_t primaryTextureNumber;

  // This isn't present if the high bit of primaryTextureNumber is a 0.
  uint16_t secondaryTextureNumber;
};

// struct RdlMineData
//{
//  uint16_t vertexCount;
//  uint16_t cubeCount;
//  Vertex verticies[vertexCount]; // Vertex = { int32_t, int32_t, int32_t};
//  Cube cubes[cubeCount];
//};

#ifdef _MSC_VER
static_assert(sizeof(RdlHeader) == 20,
              "The RdlHeader structure is incorrectly packed");
#endif

RdlReader::RdlReader(const std::vector<uint8_t>& Data)
: myData(Data),
  myHeader(reinterpret_cast<const struct RdlHeader* const>(&Data.front()))
{
  // printf("Version: %d\n", myHeader->version);
  // printf("Mine data offset: %d\n", myHeader->mineDataOffset);
  // printf("Object offset: %d\n", myHeader->objectsOffset);
}

bool RdlReader::IsValid() const
{
  if (myData.size() < sizeof(magicRdl)) return false;
  if (memcmp(magicRdl, &myData.front(), sizeof(magicRdl)) != 0) return false;

  return myHeader->fileSize == myData.size();
}

std::vector<Vertex> RdlReader::Vertices() const
{
  // First step, determine how many vertices there are.
  size_t index = myHeader->mineDataOffset + 1 /* version byte */;
  const uint16_t vertexCount = (myData[index + 1] << 8) + myData[index + 0];
  std::vector<Vertex> vertices(vertexCount);

  index += 2; // For reading the vertex count.

  int32_t buffer[3];
  for (int i = 0; i < vertexCount; index += 3 * 2, ++i)
  {
    // 32-bit fixed point number, in 16:16 format
    memcpy(&buffer, &myData[index], sizeof(buffer));
    vertices[i].x = fixedToFloating(buffer[0]);
    vertices[i].y = fixedToFloating(buffer[1]);
    vertices[i].z = fixedToFloating(buffer[2]);
  }

  return vertices;
}

std::vector<Cube> RdlReader::Cubes() const
{
  ArrayReader reader(myData.data(), myData.size());
  reader.Seek(myHeader->mineDataOffset + 1 /* version */);

  // First step, determine how many vertices and cubes there are.
  const uint16_t vertexCount = reader.ReadUInt16();
  const uint16_t cubeCount = reader.ReadUInt16();

  // Skip over the vertex data and go to the cubes.
  reader.Seek(CubeOffset());
  std::vector<Cube> cubes(cubeCount);
  for (int i = 0; i < cubeCount; ++i)
  {
    Cube& cube = cubes[i];
    const uint8_t neighbourBitmask = reader.ReadByte();
    const bool isEnergyCenter = (neighbourBitmask & (1 << 6)) != 0;

    // Read neighbour information.
    for (uint8_t j = 0; j < 6; ++j)
    {
      if (neighbourBitmask & (1 << j))
      {
        cube.neighbors[j] = reader.ReadInt16();
      }
      else
      {
        cube.neighbors[j] = -1;
      }
    }

    // Read the indices of the eight vertices that make up this cube.
    for (uint8_t j = 0; j < 8; ++j)
    {
      cube.vertices[j] = reader.ReadUInt16();
      assert(cube.vertices[j] < vertexCount);
    }

    // Optionally there are four-bytes that define the energy centre.
    // This probably needs to be given a better name.
    if (isEnergyCenter)
    {
      struct EnergyCenter
      {
        uint8_t special;
        int8_t energyCenterNumber;
        int16_t value;
      };

      // TODO: Store this information in the Cube.
      EnergyCenter energyCentre;
      energyCentre.special = reader.ReadByte();
      energyCentre.energyCenterNumber = reader.ReadByte();
      energyCentre.value = reader.ReadInt16();
    }

    const int16_t rawLighting = reader.ReadInt16();
    cube.lighting = rawLighting / (24 * 327.68);

    // Wall bit masks where a 1 means it is a wall or door.
    const uint8_t wallMask = reader.ReadByte();
    for (uint8_t wallIndex = 0; wallIndex < 6; ++wallIndex)
    {
      if (wallMask & (1 << wallIndex))
      {
        cube.walls[wallIndex] = reader.ReadByte();
      }
      else
      {
        cube.walls[wallIndex] = 255;
      }
    }

    // Read texturing information for the sides.
    for (size_t j = 0, count = 6; j < count; ++j)
    {
      const bool hasTexture =
          (cube.neighbors[j] == -1) || (cube.walls[j] != 255);
      if (!hasTexture)
      {
        cube.textures[j].primaryTextureNumber = 0;
        cube.textures[j].secondaryTextureNumber = 0;
        continue;
      }

      cube.textures[j].primaryTextureNumber = reader.ReadUInt16();

      if ((cube.textures[j].primaryTextureNumber >> 15) & 1)
      {
        cube.textures[j].secondaryTextureNumber = reader.ReadUInt16();
        // printf("Side: %d: Texture: %d and %d\n", j + 1,
        //        cube.textures[j].primaryTextureNumber & ~(1 << 15),
        //        cube.textures[j].secondaryTextureNumber & 0xFFF);
      }
      else
      {
        // printf("Side: %d: Texture: %d\n", j + 1,
        //        cube.textures[j].primaryTextureNumber);
      }

      for (int k = 0; k < 4; k++)
      {
        // UVLs (3 numbers), each is 16-bits (so 2*3 bytes).
        const int16_t u = reader.ReadInt16();
        const int16_t v = reader.ReadInt16();
        const uint16_t l = reader.ReadUInt16();
      }
    }
  }
  return cubes;
}

size_t RdlReader::CubeOffset() const
{
  const size_t index = myHeader->mineDataOffset + 1 /* version */;
  const uint16_t vertexCount = (myData[index + 1] << 8) + myData[index + 0];

  // The 1 is the size of the version number, the 4 is for the four bytes that
  // are the vertex and cube counts then lastly we skip over all the vertices.
  return myHeader->mineDataOffset + 1 + 4 + 12 * vertexCount;
}
