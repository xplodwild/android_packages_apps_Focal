Building Template Plugin Projects:

Windows:

1. Double Click "GenerateSamples_win.bat" or run it through command prompt.
2. Enter the type of project to create
3. The project files will be created in vc10\windows or vc10\windows_x64 folder

Mac:

1. Run the shell script GenerateSamples_mac.sh.
2. Enter the type of project to create
3. The project files will be created in xcode\intel or xcode\intel_64 folder

Linux:

1. Run the Makefile. This Makefile will call cmake to generate the makefile for all the template plugin projects. Also all the projects will be built automatically.
2. All the template plugin project makefiles will be created in gcc folder.
3. The template plugins will be built in ../../public folder.
4. Make sure the gcc location is added to $PATH and its libraries location to $LD_LIBRARY_PATH. There is a need to add libuuid.so library path to the $LD_LIBRARY_PATH as well. Refer the ReadMe.txt present in <xmp-sdk>/build folder for more information around setting relevant gcc path on linux.
