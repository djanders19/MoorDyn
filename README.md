MoorDyn v2
==========

MoorDyn v2 is a lumped-mass mooring dynamics model intended for coupling with
floating structure codes. As of 2022 it is available under the BSD 3-Clause
license.

More information about MoorDyn is now available at moordyn-v2.readthedocs.io

## Roadmap

The version 2 is currently under development:

 - [X] BSD-3 license
 - [X] Rigid bodies
 - [X] Rigid cylindrical Rod objects, with surface-piercing capabilities
 - [ ] Wave kinematics
 - [X] Bending stiffness for power cable simulation
 - [X] pinned (3 DOF) and rigid (6 DOF) coupling options
 - [X] Replace the custom algebra code by [Eigen](https://eigen.tuxfamily.org/index.php?title=Main_Page)
 - [X] Different time integrators
 - [X] Save/load
 - [X] VTK exporter
 - [X] New C API
 - [ ] New C++ API
 - [X] Standarize code styling with [clang-format](https://clang.llvm.org/docs/ClangFormat.html)
 - [X] Replace the custom compilation scripts by [CMake](https://cmake.org/) autotools
 - [X] FORTRAN wrappers
 - [X] Python wrappers
 - [X] MATLAB mex files
 - [X] Rust wrappers
 - [ ] Documentation for users
 - [X] Documentation for developers
 - [ ] Tests

There are also some other additional tasks to be carried out prior to the final
release:

 - [X] Test in Linux with GCC
 - [X] Test in Linux with Clang
 - [X] Test in Windows with MinGW
 - [X] Test in Windows with CLang (No Fortran)
 - [X] Test in Windows with Visual Studio CE (No C++, no Fortran)
 - [X] Make a windows installer (CMake + CPack + NSIS)
 - [X] Upload the doxygen documentation somewhere (ideally in readthedocs)
 - [X] Fix the SCUDS missiles bug
 - [ ] Ask DualSphysics devs to join back the mainstream
 - [X] pip package (with wheels?)

## Aknowledgments

Many thanks to all the team of the
[National Renewable Energy Laboratory (NREL)](https://www.nrel.gov/):

  - [Matt Hall](http://matt-hall.ca/moordyn.html)
  - Jason Jonkman

Thanks also to [CoreMarine](https://www.core-marine.com/) for the help with the
version 2 development:

  - Jose Luis Cercos-Pita
  - Aymeric Devulder
  - Elena Gridasova

We must also express our gratitude to some other developers that have
contributed

  - [David Joseph Anderson](https://davidjosephanderson.com/)
