cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Community firmware replaces the bootloader when installed over USB. Keep the
# factory Recovery entry gesture in every derivative build so USB development
# does not strand the device outside the mini-program BLE install path.
idf_build_set_property(
    BOOTLOADER_EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/bootloader_components/"
    APPEND)

project(FoloToy-AI-Passport)

# pokedexfs sits at 0x35A000, after protected cardid. FLASH_IN_PROJECT lets
# `idf.py flash` write it as its own segment and skip cardid / Recovery.
set(POKEDEX_FS_DIR "${CMAKE_CURRENT_LIST_DIR}/assets/pokedex/fs")
if(EXISTS "${POKEDEX_FS_DIR}/sprites1.bin" AND EXISTS "${POKEDEX_FS_DIR}/sprites2.bin" AND EXISTS "${POKEDEX_FS_DIR}/cries.bin" AND EXISTS "${POKEDEX_FS_DIR}/trainers.bin")
    spiffs_create_partition_image(pokedexfs "${POKEDEX_FS_DIR}" FLASH_IN_PROJECT)
else()
    message(WARNING "Pokedex media missing; run python3 tools/pokedex/build_media.py and python3 tools/pokedex/build_trainers.py")
endif()
