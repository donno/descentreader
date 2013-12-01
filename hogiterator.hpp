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

#include <iterator>
#include <string>
#include <utility>

#include <stdint.h>

class HogReader;

struct HogFileItem
{
  char name[13];
  uint32_t size;
};

class HogReaderIterator
    : public std::iterator<std::forward_iterator_tag, HogFileItem>
{
public:
  HogReaderIterator() : myReader(nullptr), myProgress(false)
  {
    myData.name[0] = '\0';
    myData.size = 0;
  }

  HogReaderIterator(HogReader& Reader);

  HogReaderIterator& operator++();
  const value_type& operator*() const;
  const value_type* operator->() const;
  bool operator==(const HogReaderIterator& o) const;
  bool operator!=(const HogReaderIterator& o) const;

private:
  value_type myData;
  bool myProgress;
  HogReader* myReader;
};

#endif
