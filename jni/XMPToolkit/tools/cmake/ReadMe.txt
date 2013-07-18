The XMP Toolkit uses an open-source system to manage the build process. XMP Toolkit places cmake configuration files (CMakeLists.txt) in the source directories which is used to generate the standard build files.These generated build files can then be used to build the Toolkit.

To use CMake:

1. Obtain a copy of the CMake for the current platform (Windows, Mac, or Linux) from the location :
http://www.cmake.org/cmake/resources/software.html
The minimum version of CMake required for this release is 2.8.6

Download the latest CMake ditribution zipped package from the above link corresponding to the current platform. For example, for the CMake version 2.8.11.1 the distribution names for different platforms are:
Windows	----	cmake-2.8.11.1-win32-x86.zip 
Mac OSX ----	cmake-2.8.11.1-Darwin64-universal.tar.gz
Linux   ----    cmake-2.8.11.1-Linux-i386.tar.gz

2. For Windows and Linux copy the folders /bin and /share into <xmpsdk>/tools/cmake/
   For Mac:
     a) Create the folder  <xmpsdk>/tools/cmake/bin
     b) Rename and copy the app to this location <xmpsdk>/tools/cmake/bin/cmake.app
