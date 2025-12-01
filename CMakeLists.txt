cmake_minimum_required(VERSION 3.31)
project(word_count_ranges_views LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-Wall -Wextra)

add_executable(word-count
    word-iterator.hxx
    word-count.cxx
)

