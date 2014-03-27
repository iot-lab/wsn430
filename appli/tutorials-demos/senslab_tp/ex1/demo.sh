#! /bin/bash

source wsim.inc

## ============= Config===================

ELF=ex1.elf
PLATFORM=senslabv13b

VERBOSE=2
LOGFILE=wsim.log
TRACEFILE=wsim.trc
GUI=yes

MODE=time
TIME=30s

## ============= Run =====================

run_wsim

## =======================================

