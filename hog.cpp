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

#include "rdl.hpp"

#include <iterator>
#include <utility>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static_assert( sizeof(uint8_t) == 1 , "The size of uint8_t is incorrect it must be 1-byte" );
static_assert( sizeof(uint32_t) == 4, "The size of uint32_t is incorrect it must be 4-bytes" );
static_assert( sizeof(uint8_t) == sizeof(char), "The size of a char must be 1-byte" );

// The 3-byte MAGIC number at the start of the file format used to identifiy the
// file as being a Descent HOG file.
static uint8_t magic[3] = {'D', 'H', 'F'};

struct HogFileHeader
{
  char name[13]; // Padded to 13 bytes with \0.
  uint32_t size; // The filesize as N bytes.
};

// Warning: The above structure is padded on x86 so you can not just read in the
// whole thing.

class HogReaderIterator;

class HogReader
{
public:
  typedef HogReaderIterator iterator;

  typedef std::shared_ptr<std::vector<uint8_t>> T_FileData;

    HogReader(const char* filename);
    ~HogReader();

    bool IsValid() const;
    // Returns true if the file was succesfully opened and the magic header is
    // correct.

    bool NextFile();


  T_FileData CurrentFile();
  // Returns a shared pointer to the vector containing the data for the current file.

  const char* CurrentFileName() const;
  unsigned int CurrentFileSize() const;

  iterator begin();
  iterator end();
  
private:
  FILE* myFile;
  uint8_t myHeader[sizeof(magic)];
  struct HogFileHeader myChildFile;

  T_FileData myFileDataPtr; // Only valid when we are deferenced until the next read.
};

class HogReaderIterator
{
public:
  typedef std::pair<const char*, HogReader*> value_type;

  HogReaderIterator()
    : myReader(NULL),
      myProgress(false)
    {
      myData.first = NULL;
      myData.second = myReader;
    }

  HogReaderIterator( HogReader& Reader )
    :  myReader(&Reader),
       myProgress(Reader.IsValid())
    {
      myData.first = Reader.CurrentFileName();
      myData.second = myReader;
    }
  // Itr(const Itr& o);                   // Copy constructor
  // Itr& operator=(const Itr& o);        // Assignment operator
  HogReaderIterator& operator++();                   // Next element
  value_type& operator*();                   // Dereference
  bool operator==(const HogReaderIterator& o) const; // Comparison
  bool operator!=(const HogReaderIterator& o) const; // { return !(this == o); 
  
private:
  value_type myData;
  bool myProgress;
  HogReader* myReader;
};

HogReaderIterator& HogReaderIterator::operator++()
{
    // You can't increment the null so error.
    myProgress = myReader->NextFile();
    myData.first = myReader->CurrentFileName();
    
    // myData.second == myReader.
    //myData.second = myReader->CurrentFileSize();
    return *this;
}

HogReaderIterator::value_type& HogReaderIterator::operator*()
{
  return myData;
}  

bool HogReaderIterator::operator==(const HogReaderIterator& o) const
{
    if (myReader == o.myReader) return true;
    if (myReader && o.myReader)
    {
	return myData == o.myData;
    }
    return myProgress == o.myProgress;
}

bool HogReaderIterator::operator!=(const HogReaderIterator& o) const
{
    return !(*this == o);
}

HogReader::iterator HogReader::begin()
{
  // Sync back up to the start just after the magic number.
  if (IsValid())
  {
    fseek ( myFile , sizeof(magic) , SEEK_SET );
    if( fread( &myChildFile.name, 13, 1, myFile ) != 1 ) HogReaderIterator();
    if( fread( &myChildFile.size, 4, 1, myFile ) != 1 )  HogReaderIterator();
  }
  
  return HogReaderIterator(*this);
}

HogReader::iterator HogReader::end()
{
  return HogReaderIterator();
}

HogReader::HogReader(const char* filename)
  : myFile(NULL),
    myFileDataPtr(NULL)
{
    myFile = fopen( filename, "rb" );
    myChildFile.name[0] = '\0';
    myChildFile.size = 0;
    if( !myFile ) return;

    const size_t count = 1;
    if( fread(myHeader, sizeof(myHeader), count, myFile) != count )
    {
      myHeader[0] = '\0'; // Failed to load.
      return;
    }

    // Read in the header for the first file.
    if (IsValid())
    {
      if( fread( &myChildFile.name, 13, 1, myFile ) != 1 ) return;
      if( fread( &myChildFile.size, 4, 1, myFile ) != 1 ) return;
    }
}

HogReader::~HogReader()
{
    if (myFile) fclose(myFile);
}

bool HogReader::IsValid() const
{
  if( !myFile ) return false;
  return memcmp( myHeader, magic, 3 ) == 0;
}

bool HogReader::NextFile()
{
    // Skip the current file.
    if( feof(myFile) ) return false; 

    if( !myFileDataPtr )
    {
      // The data for the current file has not been read so skip over the data section
      // for the file.
      
      if( fseek( myFile, myChildFile.size, SEEK_CUR ) != 0 ) return false;
    }

    // Read in the header for the next file.
    if( fread( &myChildFile.name, 13, 1, myFile ) != 1 ) return false;
    if( fread( &myChildFile.size, 4, 1, myFile ) != 1 ) return false;

    myFileDataPtr = NULL; // Invalid the data as we have moved along.
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

HogReader::T_FileData HogReader::CurrentFile()
{
  if( myFileDataPtr )
  {
    // The current file has already be de-ferenced so just return it.
    return myFileDataPtr;
  }

  // Skip the current file.
  if( feof(myFile) ) return NULL; 

  const unsigned int size = CurrentFileSize(); // Size in bytes

  myFileDataPtr = std::make_shared<std::vector<uint8_t>>(size);

  if( fread( &myFileDataPtr->front(), size, 1, myFile ) != 1 ) return NULL;

  return myFileDataPtr;
}

#include <string>
#include <algorithm>

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

  // File list.

  const bool listingAllFiles = false;

  if (listingAllFiles)
  {
    std::for_each(
      reader.begin(), reader.end(),
      [](HogReader::iterator::value_type n)
      {
	printf("%s %d\n", n.first, n.second);
      });
  }
  else
  {
    // Iterate over the file list and list all the files which do not
    // end in the 'rdl' file extension.
    const std::string extentionRdl(".rdl");

    std::for_each(
      reader.begin(), reader.end(),
      [&extentionRdl](HogReader::iterator::value_type n)
      {
        std::string filename(n.first);
        if (!std::equal(extentionRdl.rbegin(),
                        extentionRdl.rend(),
                        filename.rbegin())) return;

        printf("%s %d\n", n.first, n.second->CurrentFileSize());
        auto dataPtr = n.second->CurrentFile();
        auto data = *dataPtr.get(); // Get the vector.
        RdlReader reader(data);
        printf("Is valid Rdl: %s\n", reader.IsValid() ? "Yes" : "No");

        if( !reader.IsValid() ) return;
        reader.DoStuff();
      });

    // std::for_each(
    //   reader.begin(), reader.end(),
    //   [&extentionRdl](HogReader::iterator::value_type n)
    //   {
    // 	std::string filename(n.first);
    // 	if (!std::equal(extentionRdl.rbegin(), 
    // 			extentionRdl.rend(),
    // 			filename.rbegin())) return;
    // 	printf("%s %d\n", n.first, n.second->CurrentFileSize());
    //   });
  }
  return 0;
}
