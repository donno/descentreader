#ifndef ARRAY_READER_HPP_GUARD
#define ARRAY_READER_HPP_GUARD
//===----------------------------------------------------------------------===//
//
//                     The Descent map loader
//
// NAME         : ArrayReader
// PURPOSE      : Providing an interface for reading data out of an array.
// COPYRIGHT    : (c) 2011 Sean Donnellan. All Rights Reserved.
// AUTHORS      : Sean Donnellan (darkdonno@gmail.com)
// DESCRIPTION  : Provides a helper for extracting data out of an array.
//
//===----------------------------------------------------------------------===//

#include <stdint.h>

class ArrayReader
{
public:
  ArrayReader(const uint8_t* const array, size_t size);

  size_t Index() const
  {
    return myIndex;
  };

  void Seek(size_t index);

  uint8_t ReadByte();
  uint16_t ReadUInt16();
  int16_t ReadInt16();

private:
  size_t myIndex;
  const size_t mySize;
  const uint8_t* const myArray;
};

ArrayReader::ArrayReader(const uint8_t* const array, size_t size)
: myIndex(0), myArray(array), mySize(size)
{
}

void ArrayReader::Seek(size_t index)
{
  myIndex = index;
}

uint8_t ArrayReader::ReadByte()
{
  return myArray[myIndex++];
}

uint16_t ArrayReader::ReadUInt16()
{
  myIndex += 2;
  return (myArray[myIndex - 1] << 8) + myArray[myIndex - 2];
}

int16_t ArrayReader::ReadInt16()
{
  myIndex += 2;
  return (myArray[myIndex - 1] << 8) + myArray[myIndex - 2];
}

#endif
