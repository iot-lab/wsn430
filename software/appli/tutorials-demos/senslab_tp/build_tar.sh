#!/bin/sh

rm -rf wsn430
mkdir wsn430

svn export ../../drivers/wsn430 wsn430/drivers
svn export ../../lib/mac wsn430/mac
svn export ex1 wsn430/ex1
svn export ex2 wsn430/ex2
svn export ex3 wsn430/ex3

tar czf wsn430.tar.gz wsn430

rm -rf wsn430

# rsync -av wsn430.tar.gz burindes@scm.gforge.inria.fr:/home/groups/senstools/htdocs/packages/tp/


