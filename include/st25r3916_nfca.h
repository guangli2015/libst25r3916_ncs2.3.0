/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ST25R3916_NFCA_H_
#define ST25R3916_NFCA_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>

/**
 * @file
 * @defgroup st25r3911b_nfca ST25R3911B NFC Reader NFC-A Technology
 * @{
 *
 * @brief API for the ST25R3911B NFC Reader NFC-A Technology.
 */

#ifdef __cplusplus
extern "C" {
#endif



/** @brief Initialize NFC Reader NFC-A technology.
 *
 *  @details This function initializes everything that is required,
 *           like interrupts and common reader functionality.
 *
 *  @param[out] events NFC-A Events.
 *  @param[in] cnt Event count. This driver needs 2 events.
 *  @param[in] cb Callback structure.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3916_nfca_init();




#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ST25R3911B_NFCA_H_ */
