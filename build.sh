#!/bin/sh
set -xe

file=main
libs="-lssl -lcrypto -O3"

gcc $file.c $libs -o copde

