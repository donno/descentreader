#ifndef TXB_READER_HPP_GUARD
#define TXB_READER_HPP_GUARD
//===----------------------------------------------------------------------===//
//
//                     The Descent map loader
//
// NAME         : TxbReader
// PURPOSE      : Read contents of Descent .TXB format.
// COPYRIGHT    : (c) 2022 Sean Donnellan. All Rights Reserved.
// AUTHORS      : Sean Donnellan (darkdonno@gmail.com)
// DESCRIPTION  : Provides a iterators over series of bytes that represent the
//                text in a TXB (encrypted) text file.
//
// This class could likely be replaced with creating a TxbReaderIterator from
// a std::span in C++20 instead or at least a function which takes the
// std::vector and returns the begin/end iterates.
//
//===----------------------------------------------------------------------===//

class TxbReader
{
public:
  // The reader should not outlive the vector of bytes.
  TxbReader(const std::vector<uint8_t>& Data);

  TxbReaderIterator begin() const;
  TxbReaderIterator end() const;

private:
  const std::vector<uint8_t>& myData;
};

inline TxbReader::TxbReader(const std::vector<uint8_t>& Data) : myData(Data)
{
}

inline TxbReaderIterator TxbReader::begin() const
{
  return TxbReaderIterator(myData.data());
}

inline TxbReaderIterator TxbReader::end() const
{
  return TxbReaderIterator(myData.data() + myData.size());
}

#endif
