IoT-LAB Git repository
======================

This repository contains the IoT-LAB source code to run experiments on the IoT-LAB sensor network platform.

Documentation is available at https://github.com/iot-lab/iot-lab/wiki

Download toolchain
------------------ 

mspgcc3
http://sourceforge.net/projects/zolertia/files/Toolchain/msp430-z1.tar.gz

Setup your dev env
-------------------

- make sure 'PATH' is set properly e.g. in '~/.bashrc'
  so that cmake finds the cross-compiler : msp430-gcc -v

Compile all applications
------------------------

make -f iotlab.makefile

Description
-----------

The code base contains:

* the wsn430 nodes drivers with examples using them
* a port of different RTOS for the wsn430 nodes:
    * ContikiOS
    * FreeRTOS
    * TinyOS
* Radio communication libraries


License
-------

This is the source repository for the WSN430 drivers, and driver application examples.
All source code is provided under the CeCILL license [www.cecill.info] and can be copied, modified and redistributed under its terms.
If you got this archive, it means you have read and agreed with the licence.

All modification to the original source code written by:

- Guillaume Chelius <guillaume.chelius@inria.fr>
- Eric Fleury       <eric.fleury@inria.fr>
- Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
- Colin Chaballier  <colin.chaballier@inria.fr>
- Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
- Gaëtan Harter     <gaetan.harter@inria.fr>

