cmake_minimum_required(VERSION 3.12...3.21)

project(
  reproc
  VERSION 14.2.4
  DESCRIPTION "Cross-platform C99/C++11 process library"
  HOMEPAGE_URL "https://github.com/DaanDeMeyer/reproc"
  LANGUAGES C CXX
)

# Common options and functions separated for easier reuse in other projects.
include(cmake/reproc.cmake)

option(
  REPROC_MULTITHREADED
  "Use `pthread_sigmask` and link against the system's thread library"
  ON
)

if(REPROC_MULTITHREADED)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  set(REPROC_THREAD_LIBRARY ${CMAKE_THREAD_LIBS_INIT})
endif()

add_subdirectory(reproc)
add_subdirectory(reproc++)