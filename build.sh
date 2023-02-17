#!/bin/sh
set -xe

file=main
libs="-lssl -lcrypto -ggdb"

gcc $file.c $libs -o copde

