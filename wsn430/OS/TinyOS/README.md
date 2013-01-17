TinyOS for wsn430
=================

Introduction
------------

This is the Senslab port of the TinyOS release version 2.1.1


The officially supported version is the following:

* `deb http://hinrg.cs.jhu.edu/tinyos lenny main`
    * tinyos-2.1.1_2.1.1-20100323_all
    * tinyos-required-msp430_2.1-20100228_all


### wsn430 versions ###

It supports the two Senslab nodes:

* `wsn430v13` (with CC1100 radio chip)
* `wsn430v14` (with CC2420 radio chip).

### Folder content ###

* `Apps_official_2.1.1` contains the applications folder given in the official TinyOS 2.1.1 version
* `wsn430_tinyos-2.1.1_port` contains the wsn430 specific files


Usage on SSH servers
--------------------

TinyOS has already been installed on the ssh frontends and is ready to use for wsn430.

You can go to `Apps_official_2.1.1` and try an example:

        cd Apps_official_2/Blink
        make wsn430v13


Installing on your machine
--------------------------

If you want to compile TinyOS programs from your machine, you can follow the installation instructions

1. Add the tinyos repository
        sudo  echo "deb http://hinrg.cs.jhu.edu/tinyos lenny main"  >> /etc/apt/sources.list
2. Update and install the tinyos packages, for msp430 and version 2.1.1
        sudo apt-get update
        sudo apt-get install tinyos-required-msp430 tinyos-2.1.1
3. Install wsn430 specific files
        sudo cp wsn430_tinyos-2.1.1_port/* /opt/tinyos-2.1.1
4. Update your .bashrc, add the tinyos.sh file in it
        echo "source /opt/tinyos-2.1.1/tinyos.sh" >> ~/.bashrc
5. Install the java required files
        sudo bash $(which tos-install-jni)
        # corrects a missing 'generic_printf.h' dependency
        # http://www.geoffreylo.com/resources/tinyos-primer/
        sudo cp /opt/tinyos-2.1.1/tos/lib/printf.h /opt/tinyos-2.1.1/tos/lib/generic_printf.h
        cd /opt/tinyos-2.1.1/support/sdk/java
        make tinyos.jar


### Installing without 'deb' files ###

If you don't want to rely on a 'deb' method, find your own method on http://docs.tinyos.net/tinywiki/index.php/Getting_started.
However, please note the version notice on top of the document



