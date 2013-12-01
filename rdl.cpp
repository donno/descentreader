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

#include <string.h>

// The 4-byte MAGIC number at the start of the file format used to identifiy the
// file as being a Descent HOG file.
static uint8_t magicRdl[4] = {'L', 'V', 'L', 'P'};

struct RdlHeader
{
  uint8_t signature[4];
  uint32_t version;
  uint32_t mineDataOffset;
  uint32_t objectsOffset;
  uint32_t fileSize;
};

//uint16_t vertexCount;
//uint16_t cubeCount;
//Vertex verticies[vertexCount];
//Cube cubes[cubeCount];

static_assert(sizeof(RdlHeader) == 20,
              "The RdlHeader structure is incorrectly packed");

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

RdlReader::~RdlReader()
{
}

// TODO: Refactor this out.
inline double fixedToFloating(int32_t value) { return value / 65536.0; }

void RdlReader::DoStuff()
{
  //32-bit fixed point number, in 16:16 format
  size_t index = myHeader->mineDataOffset;
  const uint8_t version = myData[index];
  index = index + 1;

  const uint16_t vertexCount = (myData[index + 1] << 8) +  myData[index + 0];
  const uint16_t cubeCount   = (myData[index + 3] << 8) + myData[index + 2];

  index += 4; // We just read 4 bytes for the two counts.

  printf("\nVertex count: %d\n", vertexCount);
  printf("Cube count: %d\n", cubeCount);

  int32_t buffer[3];
  for (int i = 0; i < vertexCount; index += 4 * 2, ++i )
  {
    memcpy( &buffer, &myData[index], sizeof(buffer) );
    const double x = fixedToFloating(buffer[0]);
    const double y = fixedToFloating(buffer[1]);
    const double z = fixedToFloating(buffer[2]);

    printf("%f %f %f\n", x, y, z);
  }

  exit(0);
}
