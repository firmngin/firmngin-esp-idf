# The Firmngin connection over pure ESP-IDF — no Arduino core anywhere.
cmake_minimum_required(VERSION 3.16)

# Discover the library component from the repository's firmngin/ wrapper. It
# compiles ../src, which is the same tree the Arduino port mirrors via git
# subtree; the vendored websocket stays out (registry component instead).
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../..")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firmngin-example-basic)
