descent level reader
===

A tool/library for reading the level file formats of the video game by
Parallax Software called "Descent".

Descent is a 3D six degress of freedom shooter where the player is in a ship
and has to fly through mine tunnels of a labyrinthine nature and destroy
virus infected robots.

Licenses
---------------------
The source code of this project is hereby licenses under the terms of
"The MIT License". See LICENSE.txt for details.

Building
---------------------
The source code is compiled by a build system called cake. Its home page is
http://cake-build.sourceforge.net/


Cake is a build system written in Python which reads build scripts that are also
written in Python.

Unfortunately, at the time of writing, the project isn't available via pypi or as
wheel so needs to be installed manually by cloning the repository and running
setup.py.

The steps I've used:
# $ git clone https://github.com/lewissbaker/cake.git
# $ cd cake
# $ python setup.py bdist_wheel
# $ python -m pip install dist/Cake-0.9.7-py2-none-any.whl

Once cake is installed, you can then run "python -m cake.main build.cake" to
compile the project, or simply "python -m cake.main" as the build.cake
argument is implict.

File formats
---------------------

HOG - An archive file-format which contains multiple files (levels, sounds and
      sprites). Very similar idea to a tar or a zip with no compression.

RDL - Registered Descent Level file which describes the structure of the level.
      There are two key parts of a level, the mine structure (i.e the walls and
      cubes) and objects (i.e where hostages are, robots, cards etc).

References
---------------------

* HOG file specification: http://www.descent2.com/ddn/specs/hog/
* RDL file specification: http://www.descent2.com/ddn/specs/rdl/
* Cake build system: http://cake-build.sourceforge.net/

Coding standard
---------------------

80 columns - Each line of text should be at most 80 characters long.
Indent with two spaces - Use only spaces, and indent 2 spaces at a time.
Pointer binds to type (int* a) instead of (int *a).
Allman style breaking before braces so they start on the next line.

Authors
---------------------

* Sean Donnellan (darkdonno@gmail.com) - https://github.com/donno
