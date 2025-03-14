#!/bin/sh

set -xe

gcc -o server server.c
gcc -o client client.c
