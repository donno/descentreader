//===----------------------------------------------------------------------===//
//
//                     The Descent map loader
//
// NAME         : HogIterator
// PURPOSE      : Providing an iterator over items in the Descent .HOG format.
// COPYRIGHT    : (c) 2011 Sean Donnellan. All Rights Reserved.
// AUTHORS      : Sean Donnellan (darkdonno@gmail.com)
// DESCRIPTION  : Provides a forward iterator over the files in the HOG file
//                format that is used by Parallax Software in the computer game
//                Descent.
//
//===----------------------------------------------------------------------===//

#include "hogiterator.hpp"

#include "hogreader.hpp"

#include <assert.h>

#include <string.h>
#include <stdio.h>

HogReaderIterator::HogReaderIterator(HogReader& Reader)
: myReader(&Reader), myProgress(Reader.IsValid())
{
  strncpy(myData.name, myReader->CurrentFileName(), 13);
  myData.size = myReader->CurrentFileSize();
}

HogReaderIterator& HogReaderIterator::operator++()
{
  // You can't increment the null so error.
  myProgress = myReader->NextFile();
  strncpy(myData.name, myReader->CurrentFileName(), 13);
  myData.size = myReader->CurrentFileSize();
  return *this;
}

const HogReaderIterator::value_type& HogReaderIterator::operator*() const
{
  return myData;
}

const HogReaderIterator::value_type* HogReaderIterator::operator->() const
{
  return &myData;
}

bool HogReaderIterator::operator==(const HogReaderIterator& o) const
{
  if (myReader == nullptr || o.myReader == nullptr)
  {
    return myProgress == o.myProgress;
  }

  // You can't compare iterators from two HOG readers..
  assert(myReader == o.myReader);

  return (strcmp(myData.name, o.myData.name) == 0 &&
          myData.size == o.myData.size);
}

bool HogReaderIterator::operator!=(const HogReaderIterator& o) const
{
  return !(*this == o);
}

std::vector<uint8_t> HogReaderIterator::FileContents()
{
  return myReader->CurrentFile();
}
