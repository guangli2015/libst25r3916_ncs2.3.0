/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "st25r3916_spi.h"
#include "st25r3916_dt.h"

LOG_MODULE_REGISTER(st25r3916, CONFIG_ST25R3916_LIB_LOG_LEVEL);

static const struct device *spi_dev = DEVICE_DT_GET(DT_BUS(ST25R3911B_NODE));

#define ST25R3911B_READ_REG(_reg) (0x40 | (_reg))
#define ST25R3911B_WRITE_REG(_reg) (~0xC0 & (_reg))
#define ST25R3911B_DIRECT_CMD(_cmd) (0xC0 | (_cmd))
#define ST25R3911B_FIFO_READ 0xBF
#define ST25R3911B_FIFO_WRITE 0x80

#define REG_CNT 0x3F

/* Timing defined by spec. */
#define T_NCS_SCLK 1

#define SPI_BUF_LEN   512
static uint8_t   spi_txBuf[SPI_BUF_LEN];

/* SPI hardware configuration. */
static const struct spi_config spi_cfg =  {
	.frequency = DT_PROP(ST25R3911B_NODE, spi_max_frequency),
	.operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |
		      SPI_TRANSFER_MSB | SPI_LINES_SINGLE |
		      SPI_MODE_CPHA),
	.slave = DT_REG_ADDR(ST25R3911B_NODE),
	.cs = {
		.gpio = SPI_CS_GPIOS_DT_SPEC_GET(ST25R3911B_NODE),
		.delay = T_NCS_SCLK
	}
};



int st25r3916_spi_init(void)
{
	LOG_DBG("Initializing. SPI device: %s, CS GPIO: %s pin %d",
		spi_dev->name, spi_cfg.cs.gpio.port->name, spi_cfg.cs.gpio.pin);

	if (!device_is_ready(spi_cfg.cs.gpio.port)) {
		LOG_ERR("GPIO device %s is not ready!", spi_cfg.cs.gpio.port->name);

		return -ENXIO;
	}

	if (!device_is_ready(spi_dev)) {
		LOG_ERR("SPI device %s is not ready!", spi_dev->name);
		return -ENXIO;
	}

	return 0;
}


int st25r3916_spiTxRx(const uint8_t *txData, uint8_t *rxData, uint16_t length)
{  

  	int err;
  	if(length > SPI_BUF_LEN)
  	{
    	return -EINVAL;
  	}
  
  	/* Initialize Tx data*/
  	if(txData != NULL)
  	{
    	memcpy(spi_txBuf, txData, length );
  	}
  	else
  	{
    	memset(spi_txBuf, 0x00, length );
  	}
	
	const struct spi_buf tx_bufs[] = {
		{.buf = spi_txBuf, .len = length}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};

	if(rxData != NULL)
		{
			const struct spi_buf rx_bufs[] = {
				{.buf = rxData, .len = length}
			};
			const struct spi_buf_set rx = {
				.buffers = rx_bufs,
				.count = ARRAY_SIZE(rx_bufs)
			};

			err = spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
			if (err) {
				LOG_ERR("SPI reg read failed, err: %d.", err);
				return err;
			}
		}
	else
		{
			err = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
			if (err) {
				LOG_ERR("SPI direct command failed, err: %d.", err);
				return err;
			}
		}
  	return 0;
}


