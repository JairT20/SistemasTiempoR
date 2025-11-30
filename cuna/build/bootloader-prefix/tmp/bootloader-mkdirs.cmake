# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/jair/esp/esp-idf/components/bootloader/subproject"
  "/home/jair/Documents/sistemastiemporeal/cuna/build/bootloader"
  "/home/jair/Documents/sistemastiemporeal/cuna/build/bootloader-prefix"
  "/home/jair/Documents/sistemastiemporeal/cuna/build/bootloader-prefix/tmp"
  "/home/jair/Documents/sistemastiemporeal/cuna/build/bootloader-prefix/src/bootloader-stamp"
  "/home/jair/Documents/sistemastiemporeal/cuna/build/bootloader-prefix/src"
  "/home/jair/Documents/sistemastiemporeal/cuna/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/jair/Documents/sistemastiemporeal/cuna/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/jair/Documents/sistemastiemporeal/cuna/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
