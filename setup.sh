#!/bin/bash

read -e -p "Path: " VARNAME
echo 'PATH_EXTLIB :=' $VARNAME > settings.mk