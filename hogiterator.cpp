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

HogReaderIterator::HogReaderIterator(HogReader& Reader)
  : myReader(&Reader), myProgress(Reader.IsValid())
{
  myData.first = Reader.CurrentFileName();
  myData.second = myReader;
}

HogReaderIterator& HogReaderIterator::operator++()
{
  // You can't increment the null so error.
  myProgress = myReader->NextFile();
  myData.first = myReader->CurrentFileName();
  // myData.second = myReader->CurrentFileSize();
  return *this;
}

HogReaderIterator::value_type& HogReaderIterator::operator*()
{ return myData; }

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
{ return !(*this == o); }
