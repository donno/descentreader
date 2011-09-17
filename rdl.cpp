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

// The 4-byte MAGIC number at the start of the file format used to identifiy the
// file as being a Descent HOG file.
static char magicRdl[4] = {'D', 'L', 'V', 'P'};

struct RdlHeader
{
  char signature[4];
  long version;
  long mineDataOffset;
  long objectsOffset;
  long fileSize;
};

static_assert(sizeof(RdlHeader) == 20, 
	      "The RdlHeader structure is incorrectly packed");

class RdlReader
{
  // TODO: Write one that takes a file as well.
  RdlReader(unsigned char* data, int size);
  ~RdlReader();
};
