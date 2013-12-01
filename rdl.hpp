#ifndef RDL_HPP_GUARD
#define RDL_HPP_GUARD
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

#include <vector>

#ifdef _MSC_VER
typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
#else
#include <stdint.h>
#include <stddef.h>
#endif

struct Cube;
struct RdlHeader;

struct Vertex
{
  double x;
  double y;
  double z;
};

class RdlReader
{
  // TODO: Write one that takes a file as well.
public:
  RdlReader(const std::vector<uint8_t>& Data);

  bool IsValid() const;
  // Returns true if magic header is correct.

  std::vector<Vertex> Vertices() const;
  std::vector<Cube> Cubes() const;

private:
  size_t CubeOffset() const;
  // The index of the first cube in the file.

  const std::vector<uint8_t>& myData;
  const RdlHeader* const myHeader;
};

#endif
