# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/EspressifIDE-IDF/Espressif/frameworks/esp-idf-v4.4.3/components/bootloader/subproject"
  "D:/Workspace/ESP_32_LED_BLINKER/build/bootloader"
  "D:/Workspace/ESP_32_LED_BLINKER/build/bootloader-prefix"
  "D:/Workspace/ESP_32_LED_BLINKER/build/bootloader-prefix/tmp"
  "D:/Workspace/ESP_32_LED_BLINKER/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Workspace/ESP_32_LED_BLINKER/build/bootloader-prefix/src"
  "D:/Workspace/ESP_32_LED_BLINKER/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Workspace/ESP_32_LED_BLINKER/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
