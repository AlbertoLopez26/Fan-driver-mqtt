# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/ESP_framework/v5.2.2/esp-idf/components/bootloader/subproject"
  "C:/Users/Alberto L/OneDrive/Documentos/ESP32/FanDriveMQTT/build/bootloader"
  "C:/Users/Alberto L/OneDrive/Documentos/ESP32/FanDriveMQTT/build/bootloader-prefix"
  "C:/Users/Alberto L/OneDrive/Documentos/ESP32/FanDriveMQTT/build/bootloader-prefix/tmp"
  "C:/Users/Alberto L/OneDrive/Documentos/ESP32/FanDriveMQTT/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/Alberto L/OneDrive/Documentos/ESP32/FanDriveMQTT/build/bootloader-prefix/src"
  "C:/Users/Alberto L/OneDrive/Documentos/ESP32/FanDriveMQTT/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Alberto L/OneDrive/Documentos/ESP32/FanDriveMQTT/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Alberto L/OneDrive/Documentos/ESP32/FanDriveMQTT/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
