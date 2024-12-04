Instructions for compiling MEX files for SoundMexPro
----------------------------------------------------

Compiling MATLAB MEX files
--------------------------
To compile GNU Octave MEX files, you need MATLAB and C++-compiler to be installed and configured to run the 
MATLAB command "mex" sucessfully

To compile the main SoundMexPro MEX file and the MEX for running script plugins start MATLAB, change to the "mex"
subdirectory of your SoundMexPro installatio and run the following commands:

mex -outdir ..\bin soundmexpro.cpp soundmexpro_tools.cpp soundmexpro_ipc.cpp
mex -outdir ..\bin mpluginmex.cpp 


Depending on your MATLAB version and compiler, you might need to pass the define "MATLAB65" to mex, i.e.

mex -DMATLAB65 -outdir ..\bin soundmexpro.cpp soundmexpro_tools.cpp soundmexpro_ipc.cpp
mex -DMATLAB65 -outdir ..\bin mpluginmex.cpp 

The MEX files will be compiled and written to the "bin" directory of SoundMexPro. If you are using the 64bit version
of SoundmMexPro replace 'bin' by 'bin64'

Compiling MATLABGNU Octave MEX files
------------------------------------
To compile GNU Octave MEX files, you need GNU Octave and C++-compiler to be installed and configured to run 
the MATLAB command "mkoctfile" sucessfully.

To compile the main SoundMexPro MEX file and the MEX for running script plugins start Octave, change to the "mex"
subdirectory of your SoundMexPro installation and run the following commands:

mkoctfile --mex -o ..\bin\soundmexpro.mex soundmexpro.cpp soundmexpro_tools.cpp soundmexpro_ipc.cpp -lole32
mkoctfile --mex -o ..\bin\mpluginmex.mex mpluginmex.cpp

The MEX files will be compiled and written to the "bin" directory of SoundMexPro. If you are using the 64bit version
of SoundmMexPro replace 'bin' by 'bin64'


