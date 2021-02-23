cmake_minimum_required(VERSION 2.8.12)
project(dr_buster)

add_subdirectory(pybind11)
pybind11_add_module(_core src/main.cpp)
