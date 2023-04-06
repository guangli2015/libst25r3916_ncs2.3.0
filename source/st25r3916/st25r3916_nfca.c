/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <stdbool.h>
#include <assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <st25r3916_nfca.h>



#include "st25r3916_spi.h"
#include "st25r3916_com.h"
#include "st25r3916_irq.h"
#include "example_poller.h"

LOG_MODULE_DECLARE(st25r3916);




int st25r3916_nfca_init()
{
    int err;
	unsigned char val;
	LOG_INF("st25r3916_nfca_init");
	err = st25r3916_spi_init();
	if (err) {
		return err;
	}
/*	LOG_INF("preval 0x%x \n",val);
	err=st25r3916ReadRegister(ST25R3916_REG_OP_CONTROL,&val);
	if (err) {
		LOG_INF("st25r3916ReadRegister fail\n");
		return err;
	}
	LOG_INF("aftval 0x%x \n",val);
	st25r3916WriteRegister(ST25R3916_REG_OP_CONTROL,0xc0);
		LOG_INF("preval 0x%x \n",val);
	err=st25r3916ReadRegister(ST25R3916_REG_OP_CONTROL,&val);
	if (err) {
		LOG_INF("st25r3916ReadRegister fail\n");
		return err;
	}
	LOG_INF("aftval 0x%x \n",val);*/
	//st25r3916InitInterrupts();
		/* Initialize rfal and run example code for NFCA */
	exampleRfalPollerRun();

	
	return 0;
}


