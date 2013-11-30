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

#include <assert.h>
#include <string.h>
#include <stdio.h>

// TODO: Refactor this out.
inline double fixedToFloating(int32_t value) { return value / 65536.0; }
inline double fixedToFloating(int16_t value) { return value / 4096.0; }

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
  printf("Version: %d\n", myHeader->version);
  printf("Mine data offset: %d\n", myHeader->mineDataOffset);
  printf("Object offset: %d\n", myHeader->objectsOffset);
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
  const uint16_t vertexCount = (myData[index + 1] << 8) +  myData[index + 0];
  std::vector<Vertex> vertices(vertexCount);

  index += 2; // For reading the vertex count.

  int32_t buffer[3];
  for (int i = 0; i < vertexCount; index += sizeof(buffer), ++i)
  {
    memcpy(&buffer, &myData[index], sizeof(buffer));

    // 32-bit fixed point number, in 16:16 format
    vertices[i].x = fixedToFloating(buffer[0]);
    vertices[i].y = fixedToFloating(buffer[1]);
    vertices[i].z = fixedToFloating(buffer[2]);
  }

  return vertices;
}

void RdlReader::DoStuff()
{
  size_t index = myHeader->mineDataOffset + 1 /* version */;
  const uint16_t vertexCount = (myData[index + 1] << 8) + myData[index + 0];
  const uint16_t cubeCount = (myData[index + 3] << 8) + myData[index + 2];
  index += 4; // Four bytes were just read for the two count above.

  printf("Vertex count: %d\n", vertexCount);
  printf("Cube count: %d\n", cubeCount);

  // Vertices are read by the Vertices() method, so we seek to the start
  // of the cubes.
  index = myHeader->mineDataOffset + 1 + 4 + 12 * vertexCount;

  // Count the number of walls across all cubes.
  uint16_t totalWallCount = 0;

  for (int i = 0; i < cubeCount; ++i)
  {
    const size_t cubeStartIndex = index;

    // First byte of a cube is the neighbour bitmask.
    const uint8_t neighborBitmask = myData[index];
    ++index;

    // Determine number of neighbouring cubes (i.e number of 1's in bits 0-5)
    uint8_t neighbourCount = 0;
    for (uint8_t j = 0, bitmask = neighborBitmask;
         j<6; ++j, bitmask = bitmask>> 1)
    {
      if (bitmask & 1) ++neighbourCount;
    }
    const bool isEnergyCenter = (neighborBitmask & (1 << 6)) != 0;

    // printf("Neighbor bitmask: ");
    // printBitmask(neighborBitmask);

    // Lookup the IDs of the neighbours.
    //
    // This isn't stricly needed as its not really useful information if you
    // are going to load all cubes anyway.
    //
    // TODO: Not yet implemented (actually reading the neighbor list).
    std::vector<int16_t> neighbors(neighbourCount);
    index += sizeof(int16_t) * neighbourCount;

    // Read the indcies of the eight vertices that make up this cube.
    //
    // Not sure this is right...
    uint16_t vertices[8];
    for (uint8_t j = 0; j < 8; ++j)
    {
      // Read the indices.
      vertices[j] = (myData[index + 1] << 8) + myData[index + 0];
      // vertices[j] = (myData[index + 1] << 0) + (myData[index + 0] << 8);
      index += 2;

      if (vertices[j] >= vertexCount)
      {
        // printf("%d\n", vertices[j]);
      }
      assert(vertices[j] < vertexCount);
    }

    if (isEnergyCenter)
    {
      printf("is energy centre\n");
      struct EnergyCenter
      {
        uint8_t special;
        int8_t energyCenterNumber;
        int16_t value;
      };

      // index += sizeof(EnergyCenter); // hope there is no padding.
      index += 4;
    }

    const int16_t rawLighting =
        static_cast<int16_t>((myData[index + 1] << 8) + myData[index + 0]);
    const double lighting = rawLighting / (24 * 327.68);
    index += 2; // Take into account 16-bit number for the lighting.

    // Wall bit masks where a 1 means it is a war or a door.
    const uint8_t wallMask = myData[index];
    ++index;
    // Value of 1 means it is a wall or a door,
    uint8_t wallCount = 0;
    for (uint8_t j = 0, bitmask = wallMask; j<6; ++j, bitmask = bitmask>> 1)
    {
      if (bitmask & 1) ++wallCount;
    }

    printf("Wall count: %d\n", wallCount);

    // printf("Wall bitmask: ");
    // printBitmask(wallMask);

    // For each bit in the wallMask there is an unsigned byte after which is the
    // ID of the wall. Where 255 means -1 and no wall.
    for (uint8_t j = 0; j < wallCount; ++j)
    {
      // Read the id of the wall..
      // printf("Wall: %d\n", myData[index + j]);
    }
    index += wallCount;

    totalWallCount += wallCount;
    // For each side that doesn't have a neigbour
    //     6 - neighbourCount

    // Read texturing information for the sides.

    // 0 to 6 detemrine based on this...

    // Determine if a side is textured....

    size_t sidesWithTextures = 0;
    RdlTexture textures[6];
    for (size_t j = 0, count = 6; j < count; ++j)
    {
      // Determine if this side has a texture
      const bool hasNoTexture =
          (neighborBitmask & (1 << j)) || ((wallMask & (1 << j)) != 0);
      if (hasNoTexture) continue;

      ++sidesWithTextures;

      textures[j].primaryTextureNumber =
        (myData[index + 1] << 8) + myData[index + 0];

      index += 2; // 2 for the primary number.

      if ((textures[j].primaryTextureNumber >> 15) & 1)
      {
        textures[j].secondaryTextureNumber =
            (myData[index + 1] << 8) + myData[index + 0];

        textures[j].primaryTextureNumber <<= 1;
        textures[j].primaryTextureNumber >>= 1;
        printf("Side: %d: Texture: %d\n", j, textures[j].primaryTextureNumber);
        printf("Side: %d: Second texture: %d.\n", j,
               textures[j].secondaryTextureNumber);
        index += 2; // 2 bytes for the secondary number.
      }
      else
      {
        printf("Side: %d: Texture: %d\n", j, textures[j].primaryTextureNumber);
      }

      for (int k = 0; k < 4; k++)
      {
        // UVLs (3 numbers), each is 16-bits (so 2*3 bytes).
        // u = int16_t
        // v = int16_t
        // l = uint16_t
        index += 2 * 3;
      }
    }

    printf("sidesWithTextures: %d\n", sidesWithTextures);
    printf("Wall count: %d\n", wallCount);
    printf("Wall count derived: %d\n", 6 - neighbourCount);
    printf("Neighbour count: %d\n", neighbourCount);
    printf("Is energy center: %c\n", isEnergyCenter ? 'y' : 'n');
    printf("Lighting: %f\n", lighting);
    printf("Difference: %d\n", index - cubeStartIndex);
    printf("    NEXT\n");
  }

  printf("Total wall count: %d\n", totalWallCount);
}
