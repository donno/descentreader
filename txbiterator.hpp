#ifndef TXB_ITERATOR_HPP_GUARD
#define TXB_ITERATOR_HPP_GUARD
//===----------------------------------------------------------------------===//
//
//                     The Descent map loader
//
// NAME         : TxbIterator
// PURPOSE      : Providing an iterator over text in aDescent .TXB format.
// COPYRIGHT    : (c) 2022 Sean Donnellan. All Rights Reserved.
// AUTHORS      : Sean Donnellan (darkdonno@gmail.com)
// DESCRIPTION  : Provides a forward iterator over text in a TXB (encrypted)
//                text file. This is one of the files found in HOG file format
//                that is used by Parallax Software in the computer game
//                Descent.
//
//===----------------------------------------------------------------------===//

#include <iterator>
#include <stdint.h>

class TxbReaderIterator
{
  const uint8_t* mySource;
  char myValue;

  char CurrentValue() const;

public:
  // Ideally this would be a std::random_access_iterator_tag, unless it
  // was going to support expanding LR to CR LF.
  using iterator_category = std::forward_iterator_tag;
  using value_type = char;
  using difference_type = std::ptrdiff_t;
  using pointer = const char*;
  using reference = const char&;

  TxbReaderIterator(const uint8_t* Value);

  reference operator*() const
  {
    return myValue;
  }
  pointer operator->()
  {
    return &myValue;
  }

  TxbReaderIterator& operator++();
  TxbReaderIterator operator++(int);

  friend bool operator==(const TxbReaderIterator& Lhs,
                         const TxbReaderIterator& Rhs)
  {
    return Lhs.mySource == Rhs.mySource;
  };
  friend bool operator!=(const TxbReaderIterator& Lhs,
                         const TxbReaderIterator& Rhs)
  {
    return Lhs.mySource != Rhs.mySource;
  };
};

#endif
