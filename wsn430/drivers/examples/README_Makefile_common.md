Compilation with Makefile.common
================================


The compilation is based on a Makefile that:

* defines the minimum things
* includes the `Makefile.common`
* and get simple support for:
    * multiple targets with many common source files,
    * header dependency,
    * mspgcc “uniarch” support
    * and all the .elf and .hex files generation.


Makefile
--------

It checks that some required environment variables exist.
It’s better to see a well written error message
than a gcc error saying than one of the source file is missing.

This is all what a Makefile must contain to work

* `NAMES` variable with one name for each target
* `SRC` variable containing the source files common to all the targets
* `INCLUDES` variable with the gcc include arguments
* Include the Makefile.common
* If required, `SRC_target_name` variable containing the source files specific to `<target_name>`

