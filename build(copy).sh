#!/bin/sh
#add warnings -Wall -Wextra -Wconversion -Wshadow -Wpedantic -Werror -pedantic-errors
gcc -march=native -o0  multithreading_test.c -I.  -g3 -msse4.1  -o executable -L/usr/lib/x86_64-linux-gnu -lX11 -lm -pthread

