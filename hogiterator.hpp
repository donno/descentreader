#ifndef HOG_ITERATOR_HPP_GUARD
#define HOG_ITERATOR_HPP_GUARD
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

#include <utility>

class HogReader;

class HogReaderIterator
{
public:
  typedef std::pair<const char*, HogReader*> value_type;

  HogReaderIterator() : myReader(nullptr), myProgress(false)
  {
    myData.first = nullptr;
    myData.second = nullptr;
  }

  HogReaderIterator(HogReader& Reader);

  HogReaderIterator& operator++();
  value_type& operator*();
  bool operator==(const HogReaderIterator& o) const;
  bool operator!=(const HogReaderIterator& o) const;

private:
  value_type myData;
  bool myProgress;
  HogReader* myReader;
};

#endif
