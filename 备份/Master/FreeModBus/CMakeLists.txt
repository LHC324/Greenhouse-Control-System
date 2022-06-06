cmake_minimum_required(VERSION 3.0)

aux_source_directory(. libsrc)

add_library(freemodbus ${libsrc})