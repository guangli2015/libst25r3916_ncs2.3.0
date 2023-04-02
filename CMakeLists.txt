#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(
   include
   source/st25r3916
   source
)
zephyr_library()
FILE(GLOB drv_sources1 source/*.c)
FILE(GLOB drv_sources2 source/st25r3916/*.c)
set(drv_sources ${drv_sources1} ${drv_sources2})
zephyr_library_sources(${drv_sources})