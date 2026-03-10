# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/soft_dawnload/esp/Espressif/frameworks/esp-idf-v5.1.2/components/bootloader/subproject"
  "D:/soft_dawnload/esp/Espressif/frameworks/esp-idf-v5.1.2/examples/ai_chat/build/bootloader"
  "D:/soft_dawnload/esp/Espressif/frameworks/esp-idf-v5.1.2/examples/ai_chat/build/bootloader-prefix"
  "D:/soft_dawnload/esp/Espressif/frameworks/esp-idf-v5.1.2/examples/ai_chat/build/bootloader-prefix/tmp"
  "D:/soft_dawnload/esp/Espressif/frameworks/esp-idf-v5.1.2/examples/ai_chat/build/bootloader-prefix/src/bootloader-stamp"
  "D:/soft_dawnload/esp/Espressif/frameworks/esp-idf-v5.1.2/examples/ai_chat/build/bootloader-prefix/src"
  "D:/soft_dawnload/esp/Espressif/frameworks/esp-idf-v5.1.2/examples/ai_chat/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/soft_dawnload/esp/Espressif/frameworks/esp-idf-v5.1.2/examples/ai_chat/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/soft_dawnload/esp/Espressif/frameworks/esp-idf-v5.1.2/examples/ai_chat/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
