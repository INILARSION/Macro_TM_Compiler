cmake_minimum_required(VERSION 3.19)
project(Makro_Compiler C)

set(CMAKE_C_STANDARD 11)

add_executable(Makro_Compiler main.c parser.c parser.h program_helper.h)