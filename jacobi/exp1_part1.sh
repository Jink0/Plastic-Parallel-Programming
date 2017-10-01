#!/bin/bash
# Repeats, Grid size, Num Stages, (Num workers, Num iterations, Set-pin bool, Num cores)
bin/jacobi 1 256 1   1 10000 1 4

bin/jacobi 1 256 2   1 5000  0     4 5000 0
bin/jacobi 1 256 2   1 5000  1 4   4 5000 1 4