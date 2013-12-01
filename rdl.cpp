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

//                - A series of files which are preceded by a short header,
//                  which describes the name of the file and the numer of bytes.
//
//                 | "DLVLP - 3 bytes
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

#include "rdl.hpp"

#include "arrayreader.hpp"

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

void RdlReader::DoStuff()
{
  ArrayReader reader(myData.data(), myData.size());
  reader.Seek(myHeader->mineDataOffset + 1 /* version */);

  const uint16_t vertexCount = reader.ReadUInt16();
  const uint16_t cubeCount = reader.ReadUInt16();

  printf("Vertex count: %d\n", vertexCount);
  printf("Cube count: %d\n", cubeCount);

  size_t index = CubeOffset();
  reader.Seek(index);

  for (int i = 0; i < cubeCount; ++i)
  {
    const size_t cubeStartIndex = index;

    // First byte of a cube is the neighbour bitmask.
    const uint8_t neighborBitmask = reader.ReadByte();
    ++index;

    const bool isEnergyCenter = (neighborBitmask & (1 << 6)) != 0;

    // Lookup the IDs of the neighbours.
    int16_t neighbors[6];
    size_t neighbourCount = 0;
    for (uint8_t j = 0; j < 6; ++j)
    {
      if (neighborBitmask & (1 << j))
      {
        neighbors[j] = reader.ReadInt16();
        ++neighbourCount;
      }
      else
      {
        neighbors[j] = -1;
      }
    }

    index += sizeof(int16_t) * neighbourCount;
    assert(index == reader.Index());

    // Read the indices of the eight vertices that make up this cube.
    uint16_t vertices[8];
    for (uint8_t j = 0; j < 8; ++j)
    {
      vertices[j] = reader.ReadUInt16();
      index += 2;
      assert(vertices[j] < vertexCount);
    }

    if (isEnergyCenter)
    {
      struct EnergyCenter
      {
        uint8_t special;
        int8_t energyCenterNumber;
        int16_t value;
      };

      EnergyCenter energyCentre;
      energyCentre.special = reader.ReadByte();
      energyCentre.energyCenterNumber = reader.ReadByte();
      energyCentre.value = reader.ReadInt16();

      index += 4;
    }

    const int16_t rawLighting = reader.ReadInt16();
    const double lighting = rawLighting / (24 * 327.68);
    index += 2; // Take into account 16-bit number for the lighting.

    // Wall bit masks where a 1 means it is a war or a door.
    const uint8_t wallMask = reader.ReadByte();
    ++index;
    // Value of 1 means it is a wall or a door,

    uint8_t walls[6];
    for (uint8_t wallIndex = 0; wallIndex < 6; ++wallIndex)
    {
      if (wallMask & (1 << wallIndex))
      {
        walls[wallIndex] = reader.ReadByte();
        ++index;
      }
      else
      {
        walls[wallIndex] = 255;
      }
    }
    assert(index == reader.Index());

    size_t sidesWithTextures = 0;

    // Read texturing information for the sides.
    RdlTexture textures[6];
    for (size_t j = 0, count = 6; j < count; ++j)
    {
      const bool hasTexture = (neighbors[j] == -1) || (walls[j] != 255);
      if (!hasTexture) continue;

      ++sidesWithTextures;

      assert(index == reader.Index());

      textures[j].primaryTextureNumber = reader.ReadUInt16();
      index += 2; // 2 for the primary number.

      if ((textures[j].primaryTextureNumber >> 15) & 1)
      {
        textures[j].secondaryTextureNumber = reader.ReadUInt16();

        textures[j].primaryTextureNumber <<= 1;
        textures[j].primaryTextureNumber >>= 1;
        // printf("Side: %d: Texture: %d and %d\n", j + 1,
        //        textures[j].primaryTextureNumber,
        //        textures[j].secondaryTextureNumber & 0xFFF);
        index += 2; // 2 bytes for the secondary number.
      }
      else
      {
        // printf("Side: %d: Texture: %d\n", j + 1,
        //        textures[j].primaryTextureNumber);
      }

      assert(index == reader.Index());

      for (int k = 0; k < 4; k++)
      {
        // UVLs (3 numbers), each is 16-bits (so 2*3 bytes).
        const int16_t u = reader.ReadInt16();
        const int16_t v = reader.ReadInt16();
        const uint16_t l = reader.ReadUInt16();
        index += 2 * 3;
      }
    }

    printf("Cube %d: Lighting: %.2f\n", i, lighting);
  }
}

size_t RdlReader::CubeOffset() const
{
  const size_t index = myHeader->mineDataOffset + 1 /* version */;
  const uint16_t vertexCount = (myData[index + 1] << 8) + myData[index + 0];

  // The 1 is the size of the version number, the 4 is for the four bytes that
  // are the vertex and cube counts then lastly we skip over all the vertices.
  return myHeader->mineDataOffset + 1 + 4 + 12 * vertexCount;
}
