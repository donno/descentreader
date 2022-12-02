//===----------------------------------------------------------------------===//
//
//                     The Descent map loader
//
// NAME         : TxbReaderIterator
// PURPOSE      : Providing an iterator over text in aDescent .TXB format.
// COPYRIGHT    : (c) 2011 Sean Donnellan. All Rights Reserved.
// AUTHORS      : Sean Donnellan (darkdonno@gmail.com)
// DESCRIPTION  : A decoder for the TXB file format that is used by Parallax
//                Software in the computer game, Descent.
//
// The file format is as follows:
//
// A byte value of 0xA is a LF and for the game would be converted to CR LF.
// Other bytes are rotated by 2 bits to the left (so the most significant bits
// then it is XORed with 0xA7.
//
//===----------------------------------------------------------------------===//

#include "txbiterator.hpp"

char TxbReaderIterator::CurrentValue() const
{
  if (*mySource == 0x0A)
  {
    return 0x0A;
  }

  return (((*mySource & 0x3F) << 2) + ((*mySource & 0xC0) >> 6)) ^ 0xA7;
}

TxbReaderIterator::TxbReaderIterator(const uint8_t* Source)
: mySource(Source), myValue(CurrentValue())
{
}

TxbReaderIterator& TxbReaderIterator::operator++()
{
  myValue = CurrentValue();
  mySource++;
  return *this;
}

TxbReaderIterator TxbReaderIterator::operator++(int)
{
  TxbReaderIterator previous = *this;
  ++(*this);
  return previous;
}
