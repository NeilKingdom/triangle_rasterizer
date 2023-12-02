#!/bin/sh
# 
# Dependencies: 
#     - sxiv
#     - gcc

gcc rasterizer.c -o rasterizer -lm
./rasterizer
sxiv "triangle.ppm"
