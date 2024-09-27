****************************************************************************
	Description of SoundMexPro source code files

	Copyright 2023 Daniel Berg, Oldenburg, Germany
Supported by the German Research Council (DFG, EXC 2177/1; project ID 390895286)
****************************************************************************

****************************************************************************
    SoundMexPro is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SoundMexPro is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SoundMexPro.  If not, see <http:www.gnu.org/licenses/>.
****************************************************************************

The source code archive of SoundMexPro contains all source code files and
C++-Builder project files (*.cbproj) to compile all executables and libraries
of SoundMexPro. If not stated differently in separate _README.txt files within 
a projects subdirectory all projects were compiled using Embarcadero C++-Builder
12.2 Athens Professional. They were compiled using "Steema TChart Pro", but special 
features of the "Pro" version are not used within SoundMexPro. You might
need to ignore errors and/or remove unsupported options from the *.DFM files for
a successful build using "Steema TChart Standard" shipped with C++-Builder.
You might need to adjust paths and/or 3rd party source code files and/or libraries 
within the project files for a successful build. The project files each contain 
two target platforms, Win32 and Win64 for 32bit and 64bit respectively

This source code package does NOT include the MEX-related source files. These files
are shipped with SoundMexPro itself in the subdirectory "mex" of the installation.

Please extract the Development-directory conained in the archive to the installation 
directory of SoundMexPro. Files from the "mex" directory of the installation will be
needed for compiling SoundMexPro.


****************************************************************************
A. Prerequisites

The following 3rd party libraries are needed for SoundMexPro (not all projects will 
need all of these libaries). 

1.FFTW 3.x
-----------
The FFTW package was developed at MIT by Matteo Frigo and Steven G. Johnson. It is
licensed under GNU GPL and can be downloaded from the fftw.org website. 
SoundMexPro projects using the libfftw3f-3.dll link to an import library created 
from the original DLL using Embarcaderos implib.exe or mkexp.exe respectively.

2. libsndfile
-------------
SoundMexPro uses libsndfile 1.x for reading sound files, see http://www.mega-nerd.com/libsndfile/ 
for more information.
SoundMexPro projects using the libsndfile link to an import library created from the original DLL
using using Embarcaderos implib.exe or mkexp.exe respectively.

3. Steinberg ASIO-SDK 2.3.3
---------------------------
SoundMexPro uses the ASIO-SDK 2.x by Steinberg. It can be downloaded from the Steinberg website.
SoundMexPro projects using the ASIO-SDK by linking AsioSDK.lib which was compiled and linked 
from the original files. You may replace AsioSDK.lib by adding the corresponding source
files from the original SDK to the projects instead.

4. Steinberg VST-SDK 2.4
------------------------
The VST-Plugins shipped with SoundMexPro uses the VST-SDK 2.4 by Steinberg. It can be downloaded 
from the Steinberg website. SoundMexPro projects using the VST-SDK by linking VSTSDK.lib which 
was compiled and linked from the original files. You may replace VSTSDK.lib by adding the 
corresponding source from the original SDK 
	AudioEffect.cpp and audioeffectx.cpp
to the projects instead.
****************************************************************************


****************************************************************************
B. Directories/Projects

NOTE: the directories/projects are described here in the order they have to be compiled. For detailed
information please refer refer to the source files in the project folders.  

1. HTTools
----------
The project HTTools10.cbproj contains two visual components in a C++-Builder-Package (*.bpl) needed 
by SoundDllPro. Compile and install this package before compiling SoundDllPro.

2. SoundDllPro
--------------
The project SoundDllPro.cbproj contains the main functionality of SoundMexPro. It creates the DLL
SoundDllPro.dll. 
SoundDllPro.dll is used in the open source freeware "AudioSpike" as well, the project contains a 
copy instruction of binaries to (local) path A:\bin and A:\bin64 respectively: djust for your needs!

3. SMPIPC
---------
The project SMPIPC.cbproj contains an executable loading SoundDllPro.dll and implements interprocess
communication to loaders, that want to use SoundDllPro (MEX, OCTAVE mex).
NOTE: this project links to the import library of SoundDllPro.dll (SoundDllPro.cbproj creates it
during the build process).

4. SoundMexProPy
----------------
The project SoundMexProPy.cbproj creates SoundMexProPy32.dll and SoundMexProPy64.dll respectively as 
interfaces to Python. They run SMPIPC.EXE and implement interprocess communication between Python
and SoundMexPro (shipped with SoundMexPro in the subdirectory "bin" and "bin64")

5. SoundDllProLoader
--------------------
The project SoundDllProLoader.cbproj creates SoundDllProLoader.exe, a loader that loads SoundDllPro.dll
directly. Please refer to the SoundDllProLoader documentation shipped with SoundMexPro in the 
subdirectory "Manual"

6. VSTPlugins
-------------
This folder contains multiple subfolders for different VST plugins. Please refer to the HtVst-Plugins 
documentation shipped with SoundMexPro in the subdirectory "Manual".

NOTE: some of the subfolders may contain separate _README.txt files with important information!!!
*************************************************************************************************

