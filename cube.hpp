#ifndef CUBE_HPP_GUARD
#define CUBE_HPP_GUARD
//===----------------------------------------------------------------------===//
//
//                     The Descent map loader
//
// NAME         : Cube
// PURPOSE      : Holds the data for a cube read from a Descent level file.
// COPYRIGHT    : (c) 2013 Sean Donnellan. All Rights Reserved.
// AUTHORS      : Sean Donnellan (darkdonno@gmail.com)
// DESCRIPTION  : A cube is like a defined by 8 points with 6 faces. The faces
//                may be walls, doors or connected to a neighbouring cell..
//
//===----------------------------------------------------------------------===//

#include <stdint.h>

struct Texture
{
  uint16_t primaryTextureNumber;

  // This isn't present if the high bit of primaryTextureNumber is a 0.
  uint16_t secondaryTextureNumber;
};

struct Cube
{
  // The ID of the vertices that make up this cube.
  uint16_t vertices[8];

  // The ID of other the neighbouring cubes. A value of -1 indicates there is no
  // cube on that face.
  int16_t neighbors[6];

  // Defines the ID of the wall that makes up the face. A value of 255 indicates
  // there is no wall.
  uint8_t walls[6];

  // The static lighting value for the cube.
  double lighting;

  // The texture information for the sides given that there is actually a wall
  // to draw there.
  Texture textures[6];
};

#endif
