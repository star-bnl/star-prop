#!/usr/bin/env bash

mkdir -p /tmp/star-test-stg
cd /tmp/star-test-stg

ln -s /tmp/star-install/.rootrc $HOME/
ln -s /tmp/star-sw/StRoot/StgMaker/config.xml

starsim -w 1 -b /tmp/star-sw/StRoot/StgMaker/macros/testg.kumac
root4star -b -l -q /tmp/star-sw/StRoot/StgMaker/macros/testg.C
