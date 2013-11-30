#ifndef HOG_READER_HPP_GUARD
#define HOG_READER_HPP_GUARD
//===----------------------------------------------------------------------===//
//
//                     The Descent map loader
//
// NAME         : HogReader
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

#include <memory>
#include <vector>

#include <stdint.h>

class HogReaderIterator;

struct HogFileHeader
{
  char name[13]; // Padded to 13 bytes with \0.
  uint32_t size; // The filesize as N bytes.
};
// Warning: The above structure is padded on x86 so you can not just read in the
// whole thing.

class HogReader
{
public:
  typedef HogReaderIterator iterator;

  HogReader(const char* filename);
  ~HogReader();

  bool IsValid() const;
  // Returns true if the file was succesfully opened and the magic header is
  // correct.

  bool NextFile();

  std::vector<uint8_t> CurrentFile();
  // Returns a copy of the data for the current file after reading it.

  const char* CurrentFileName() const;
  unsigned int CurrentFileSize() const;

  iterator begin();
  iterator end();

private:
  FILE* myFile;
  uint8_t myHeader[3];
  HogFileHeader myChildFile;
  bool hasReadFile;
};

#endif