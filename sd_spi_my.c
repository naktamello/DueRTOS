#include <sd_spi_my.h>
//! SD/MMC card states
enum card_state {
	SD_MMC_CARD_STATE_READY    = 0, //!< Ready to use
	SD_MMC_CARD_STATE_DEBOUNCE = 1, //!< Debounce on going
	SD_MMC_CARD_STATE_INIT     = 2, //!< Initialization on going
	SD_MMC_CARD_STATE_UNUSABLE = 3, //!< Unusable card
	SD_MMC_CARD_STATE_NO_CARD  = 4, //!< No SD/MMC card inserted
};

//! SD/MMC card information structure
struct sd_mmc_card {
	uint32_t clock;            //!< Card access clock
	uint32_t capacity;         //!< Card capacity in KBytes
#if (defined SD_MMC_0_CD_GPIO)
	uint32_t cd_gpio;          //!< Card detect GPIO
#  if (defined SD_MMC_0_WP_GPIO)
	uint32_t wp_gpio;          //!< Card write protection GPIO
#  endif
#endif
	uint16_t rca;              //!< Relative card address
	enum card_state state;     //!< Card state
	card_type_t type;          //!< Card type
	card_version_t version;    //!< Card version
	uint8_t  bus_width;        //!< Number of DATA lin on bus (MCI only)
	uint8_t csd[CSD_REG_BSIZE];//!< CSD register
	uint8_t high_speed;        //!< High speed card (1)
};

//! SD/MMC transfer rate unit codes (10K) list
const uint32_t sd_mmc_trans_units[7] = {
	10, 100, 1000, 10000, 0, 0, 0
};
//! SD transfer multiplier factor codes (1/10) list
const uint32_t sd_trans_multipliers[16] = {
	0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80
};


//! SD/MMC card list
//! Note: Initialize card detect pin fields if present
static struct sd_mmc_card sd_mmc_cards[SD_MMC_MEM_CNT]
#if (defined SD_MMC_0_CD_GPIO) && (defined SD_MMC_0_WP_GPIO)
 = {
# define SD_MMC_CD_WP(slot, unused) \
	{.cd_gpio = SD_MMC_##slot##_CD_GPIO, \
	.wp_gpio = SD_MMC_##slot##_WP_GPIO},
	MREPEAT(SD_MMC_MEM_CNT, SD_MMC_CD_WP, ~)
# undef SD_MMC_CD_WP
}
#elif (defined SD_MMC_0_CD_GPIO)
 = {
# define SD_MMC_CD(slot, unused) \
	{.cd_gpio = SD_MMC_##slot##_CD_GPIO},
	MREPEAT(SD_MMC_MEM_CNT, SD_MMC_CD, ~)
# undef SD_MMC_CD
}
#endif
;

//! Pointer on current slot configurated
static struct sd_mmc_card *sd_mmc_card;
static sd_mmc_spi_errno_t sd_mmc_spi_err;
//! 32 bits response of the last command
static uint32_t sd_mmc_spi_response_32;
//! Current position (byte) of the transfer started by mci_adtc_start()
static uint32_t sd_mmc_spi_transfert_pos;
//! Size block requested by last mci_adtc_start()
static uint16_t sd_mmc_spi_block_size;
//! Total number of block requested by last mci_adtc_start()
static uint16_t sd_mmc_spi_nb_block;


void send_clock(void)
{
  uint8_t cmd;

  cmd = 0xFF;
for (int i = 0; i < 10; i++) {
 spi_master_transfer(&cmd, sizeof(cmd));
 }
}

static uint8_t sd_mmc_spi_crc7(uint8_t * buf, uint8_t size)
{
	uint8_t crc, value, i;

	crc = 0;
	while (size--) {
		value = *buf++;
		for (i = 0; i < 8; i++) {
			crc <<= 1;
			if ((value & 0x80) ^ (crc & 0x80)) {
				crc ^= 0x09;
			}
			value <<= 1;
		}
	}
	crc = (crc << 1) | 1;
	return crc;
}

static bool sd_mmc_spi_wait_busy(void)
{
	uint8_t line = 0xFF;

	/* Delay before check busy
	 * Nbr timing minimum = 8 cylces
	 */
	spi_read_packet(SPI0, &line, 1);

	/* Wait end of busy signal
	 * Nec timing: 0 to unlimited
	 * However a timeout is used.
	 * 200 000 * 8 cycles
	 */
	uint32_t nec_timeout = 200000;
	spi_read_packet(SPI0, &line, 1);
	do {
		spi_read_packet(SPI0, &line, 1);
		if (!(nec_timeout--)) {
			return false;
		}
	} while (line != 0xFF);
	return true;
}

bool send_command(sdmmc_cmd_def_t cmd, uint32_t arg)
{
	return adtc_start(cmd, arg, 0, 0, false);
}

bool adtc_start(sdmmc_cmd_def_t cmd, uint32_t arg,
		uint16_t block_size, uint16_t nb_block, bool access_block)
{
	uint8_t dummy = 0xFF;
	uint8_t cmd_token[6];
	uint8_t ncr_timeout;
	uint8_t r1; //! R1 response

	// Encode SPI command
	cmd_token[0] = SPI_CMD_ENCODE(SDMMC_CMD_GET_INDEX(cmd));
	cmd_token[1] = arg >> 24;
	cmd_token[2] = arg >> 16;
	cmd_token[3] = arg >> 8;
	cmd_token[4] = arg;
	cmd_token[5] = sd_mmc_spi_crc7(cmd_token, 5);

	// 8 cycles to respect Ncs timing
	// Note: This byte does not include start bit "0",
	// thus it is ignored by card.
         spi_master_transfer(&dummy, sizeof(dummy));
        // Send command
         spi_master_transfer(cmd_token, sizeof(cmd_token));

         r1 = 0xFF;

         spi_read_packet(SPI0, &r1, 1);
         ncr_timeout = 7;
         while (1) {
                spi_read_packet(SPI0, &r1, 1);
                if((r1 & R1_SPI_ERROR) == 0) {
                spi_set_lastxfer(SPI_MASTER_BASE);
                break;
                }
                if(--ncr_timeout == 0) {
                sd_mmc_spi_err = SD_MMC_SPI_ERR_RESP_TIMEOUT;
                spi_set_lastxfer(SPI_MASTER_BASE);
                return false;
                }
         }

	// Save R1 (Specific to SPI interface) in 32 bit response
	// The R1_SPI_IDLE bit can be checked by high level
	sd_mmc_spi_response_32 = r1;
	// Manage error in R1
	if (r1 & R1_SPI_COM_CRC) {
		sd_mmc_spi_err = SD_MMC_SPI_ERR_RESP_CRC;
                 spi_set_lastxfer(SPI_MASTER_BASE);
		return false;
	}
	if (r1 & R1_SPI_ILLEGAL_COMMAND) {
		sd_mmc_spi_err = SD_MMC_SPI_ERR_ILLEGAL_COMMAND;
                 spi_set_lastxfer(SPI_MASTER_BASE);
		return false;
	}
	if (r1 & ~R1_SPI_IDLE) {
		// Other error
		sd_mmc_spi_err = SD_MMC_SPI_ERR;
                 spi_set_lastxfer(SPI_MASTER_BASE);
		return false;
	}

	// Manage other responses
	if (cmd & SDMMC_RESP_BUSY) {
		if (!sd_mmc_spi_wait_busy()) {
			sd_mmc_spi_err = SD_MMC_SPI_ERR_RESP_BUSY_TIMEOUT;
                         spi_set_lastxfer(SPI_MASTER_BASE);
			return false;
		}
	}
	if (cmd & SDMMC_RESP_8) {
		sd_mmc_spi_response_32 = 0;
		spi_read_packet(SPI0, (uint8_t*) & sd_mmc_spi_response_32, 1);
		sd_mmc_spi_response_32 = le32_to_cpu(sd_mmc_spi_response_32);
                 spi_set_lastxfer(SPI_MASTER_BASE);
	}
	if (cmd & SDMMC_RESP_32) {
		spi_read_packet(SPI0, (uint8_t*) & sd_mmc_spi_response_32, 4);
		sd_mmc_spi_response_32 = be32_to_cpu(sd_mmc_spi_response_32);
                 spi_set_lastxfer(SPI_MASTER_BASE);
	}

	sd_mmc_spi_block_size = block_size;
	sd_mmc_spi_nb_block = nb_block;
	sd_mmc_spi_transfert_pos = 0;
	return true; // Command complete


}

static bool sd_cmd8(uint8_t * v2)
{
	uint32_t resp;

	*v2 = 0;
	// Test for SD version 2
	if (!send_command(SD_CMD8_SEND_IF_COND,
			SD_CMD8_PATTERN | SD_CMD8_HIGH_VOLTAGE)) {
                       // spi_set_lastxfer(SPI_MASTER_BASE);
		return true; // It is not a V2
	}
	// Check R7 response
	resp = sd_mmc_spi_response_32;
	if (resp == 0xFFFFFFFF) {
        spi_set_lastxfer(SPI_MASTER_BASE);
		// No compliance R7 value
		return true; // It is not a V2
	}
	if ((resp & (SD_CMD8_MASK_PATTERN | SD_CMD8_MASK_VOLTAGE))
				!= (SD_CMD8_PATTERN | SD_CMD8_HIGH_VOLTAGE)) {
                               // spi_set_lastxfer(SPI_MASTER_BASE);
		return false;
	}
	*v2 = 1;
        //spi_set_lastxfer(SPI_MASTER_BASE);
	return true;
}

static bool sdio_op_cond(void)
{
	uint32_t resp;

	// CMD5 - SDIO send operation condition (OCR) command.
	if (!send_command(SDIO_CMD5_SEND_OP_COND, 0)) {
		return true; // No error but card type not updated
	}
	resp = sd_mmc_spi_response_32;
	if ((resp & OCR_SDIO_NF) == 0) {
		return true; // No error but card type not updated
	}

	/*
	 * Wait card ready
	 * Timeout 1s = 400KHz / ((6+4)*8) cylces = 5000 retry
	 * 6 = cmd byte size
	 * 4(SPI) 6(MCI) = response byte size
	 */
	uint32_t cmd5_retry = 5000;
	while (1) {
		// CMD5 - SDIO send operation condition (OCR) command.
		if (!send_command(SDIO_CMD5_SEND_OP_COND,
				resp & SD_MMC_VOLTAGE_SUPPORT)) {
			return false;
		}
		resp = sd_mmc_spi_response_32;
		if ((resp & OCR_POWER_UP_BUSY) == OCR_POWER_UP_BUSY) {
			break;
		}
		if (cmd5_retry-- == 0) {
			return false;
		}
	}
	// Update card type at the end of busy
	if ((resp & OCR_SDIO_MP) > 0) {
		sd_mmc_card->type = CARD_TYPE_SD_COMBO;
	} else {
		sd_mmc_card->type = CARD_TYPE_SDIO;
	}
	return true; // No error and card type updated with SDIO type
}

static bool sd_spi_op_cond(uint8_t v2)
{
	uint32_t arg, retry, resp;

	/*
	 * Timeout 1s = 400KHz / ((6+1)*8) cylces = 7150 retry
	 * 6 = cmd byte size
	 * 1 = response byte size
	 */
	retry = 7150;
	do {
		// CMD55 - Indicate to the card that the next command is an
		// application specific command rather than a standard command.
		if (!send_command(SDMMC_CMD55_APP_CMD, 0)) {
			return false;
		}

		// (ACMD41) Sends host OCR register
		arg = 0;
		if (v2) {
			arg |= SD_ACMD41_HCS;
		}
		// Check response
		if (!send_command(SD_SPI_ACMD41_SD_SEND_OP_COND, arg)) {
			return false;
		}
		resp = sd_mmc_spi_response_32;
		if (!(resp & R1_SPI_IDLE)) {
			// Card is ready
			break;
		}
		if (retry-- == 0) {
			return false;
		}
	} while (1);

	// Read OCR for SPI mode
	if (!send_command(SDMMC_SPI_CMD58_READ_OCR, 0)) {
		return false;
	}
	if ((sd_mmc_spi_response_32 & OCR_CCS) != 0) {
		sd_mmc_card->type |= CARD_TYPE_HC;
	}
	return true;
}

static bool sd_mmc_spi_start_read_block(void)
{
	uint32_t i;
	uint8_t token;

	Assert(!(sd_mmc_spi_transfert_pos % sd_mmc_spi_block_size));

	/* Wait for start data token:
	 * The read timeout is the Nac timing.
	 * Nac must be computed trough CSD values,
	 * or it is 100ms for SDHC / SDXC
	 * Compute the maximum timeout:
	 * Frequency maximum = 25MHz
	 * 1 byte = 8 cycles
	 * 100ms = 312500 x sd_mmc_spi_drv_read_packet() maximum
	 */
	token = 0;
	i = 500000;
	do {
		if (i-- == 0) {
			sd_mmc_spi_err = SD_MMC_SPI_ERR_READ_TIMEOUT;
			return false;
		}
		spi_read_packet(SD_MMC_SPI, &token, 1);
		if (SPI_TOKEN_DATA_ERROR_VALID(token)) {
			Assert(SPI_TOKEN_DATA_ERROR_ERRORS & token);
			if (token & (SPI_TOKEN_DATA_ERROR_ERROR
					| SPI_TOKEN_DATA_ERROR_ECC_ERROR
					| SPI_TOKEN_DATA_ERROR_CC_ERROR)) {
				sd_mmc_spi_err = SD_MMC_SPI_ERR_READ_CRC;
			} else {
				sd_mmc_spi_err = SD_MMC_SPI_ERR_OUT_OF_RANGE;
			}
			return false;
		}
	} while (token != SPI_TOKEN_SINGLE_MULTI_READ);

	return true;
}

static void sd_mmc_spi_stop_read_block(void)
{
	uint8_t crc[2];
	// Read 16-bit CRC (not cheked)
	spi_read_packet(SD_MMC_SPI, crc, 2);
}

bool sd_mmc_spi_start_read_blocks(void *dest, uint16_t nb_block)
{
	uint32_t pos;

	sd_mmc_spi_err = SD_MMC_SPI_NO_ERR;
	pos = 0;
	while (nb_block--) {
		Assert(sd_mmc_spi_nb_block >
				(sd_mmc_spi_transfert_pos / sd_mmc_spi_block_size));
		if (!sd_mmc_spi_start_read_block()) {
			return false;
		}

		// Read block
		spi_read_packet(SD_MMC_SPI, &((uint8_t*)dest)[pos], sd_mmc_spi_block_size);
		pos += sd_mmc_spi_block_size;
		sd_mmc_spi_transfert_pos += sd_mmc_spi_block_size;

		sd_mmc_spi_stop_read_block();
	}
	return true;
}

static bool sd_mmc_cmd9_spi(void)
{
	if (!adtc_start(SDMMC_SPI_CMD9_SEND_CSD, (uint32_t)sd_mmc_card->rca << 16,
			CSD_REG_BSIZE, 1, true)) {
		return false;
	}
	if (!sd_mmc_spi_start_read_blocks(sd_mmc_card->csd, 1)) {
		return false;
	}
	return true;
}

static void sd_decode_csd(void)
{
 	uint32_t unit;
	uint32_t mul;
	uint32_t tran_speed;

	// Get SD memory maximum transfer speed in Hz.
	tran_speed = CSD_TRAN_SPEED(sd_mmc_card->csd);
	unit = sd_mmc_trans_units[tran_speed & 0x7];
	mul = sd_trans_multipliers[(tran_speed >> 3) & 0xF];
	sd_mmc_card->clock = unit * mul * 1000;

	/*
	 * Get card capacity.
	 * ----------------------------------------------------
	 * For normal SD/MMC card:
	 * memory capacity = BLOCKNR * BLOCK_LEN
	 * Where
	 * BLOCKNR = (C_SIZE+1) * MULT
	 * MULT = 2 ^ (C_SIZE_MULT+2)       (C_SIZE_MULT < 8)
	 * BLOCK_LEN = 2 ^ READ_BL_LEN      (READ_BL_LEN < 12)
	 * ----------------------------------------------------
	 * For high capacity SD card:
	 * memory capacity = (C_SIZE+1) * 512K byte
	 */
	if (CSD_STRUCTURE_VERSION(sd_mmc_card->csd) >= SD_CSD_VER_2_0) {
		sd_mmc_card->capacity =
				(SD_CSD_2_0_C_SIZE(sd_mmc_card->csd) + 1)
				* 512;
	} else {
		uint32_t blocknr = ((SD_CSD_1_0_C_SIZE(sd_mmc_card->csd) + 1) *
				(1 << (SD_CSD_1_0_C_SIZE_MULT(sd_mmc_card->csd) + 2)));
		sd_mmc_card->capacity = blocknr *
				(1 << SD_CSD_1_0_READ_BL_LEN(sd_mmc_card->csd))
				/ 1024;
	}
}

static bool sd_acmd51(void)
{
	uint8_t scr[SD_SCR_REG_BSIZE];

	// CMD55 - Indicate to the card that the next command is an
	// application specific command rather than a standard command.
	if (!send_command(SDMMC_CMD55_APP_CMD, (uint32_t)sd_mmc_card->rca << 16)) {
		return false;
	}
	if (!adtc_start(SD_ACMD51_SEND_SCR, 0,
			SD_SCR_REG_BSIZE, 1, true)) {
		return false;
	}
	if (!sd_mmc_spi_start_read_blocks(scr, 1)) {
		return false;
	}


	// Get SD Memory Card - Spec. Version
	switch (SD_SCR_SD_SPEC(scr)) {
	case SD_SCR_SD_SPEC_1_0_01:
		sd_mmc_card->version = CARD_VER_SD_1_0;
		break;

	case SD_SCR_SD_SPEC_1_10:
		sd_mmc_card->version = CARD_VER_SD_1_10;
		break;

	case SD_SCR_SD_SPEC_2_00:
		if (SD_SCR_SD_SPEC3(scr) == SD_SCR_SD_SPEC_3_00) {
			sd_mmc_card->version = CARD_VER_SD_3_0;
		} else {
			sd_mmc_card->version = CARD_VER_SD_2_0;
		}
		break;

	default:
		sd_mmc_card->version = CARD_VER_SD_1_0;
		break;
	}
	return true;
}

static bool sd_mmc_cmd13(void)
{
	uint32_t nec_timeout;

	/* Wait for data ready status.
	 * Nec timing: 0 to unlimited
	 * However a timeout is used.
	 * 200 000 * 8 cycles
	 */
	nec_timeout = 200000;
	do {
		if (sd_mmc_is_spi()) {
			if (!send_command(SDMMC_SPI_CMD13_SEND_STATUS, 0)) {
				return false;
			}
			// Check busy flag
			if (!(sd_mmc_spi_response_32 & 0xFF)) {
				break;
			}
		} else {
			if (!send_command(SDMMC_MCI_CMD13_SEND_STATUS,
					(uint32_t)sd_mmc_card->rca << 16)) {
				return false;
			}
			// Check busy flag
			if (sd_mmc_spi_response_32 & CARD_STATUS_READY_FOR_DATA) {
				break;
			}
		}
		if (nec_timeout-- == 0) {
			return false;
		}
	} while (1);

	return true;
}

bool sd_mmc_spi_card_init(void)
{
	uint8_t v2 = 0;
        sd_mmc_card = &sd_mmc_cards[0];
	// In first, try to install SD/SDIO card
	sd_mmc_card->type = CARD_TYPE_SD;
	sd_mmc_card->version = CARD_VER_UNKNOWN;
	sd_mmc_card->rca = 0;

	// Card need of 74 cycles clock minimum to start
	send_clock();

	// CMD0 - Reset all cards to idle state.
	if (!send_command(SDMMC_SPI_CMD0_GO_IDLE_STATE, 0)) {
                UARTWriteStr("sd not ready\n");
		return false;
	}
	if (!sd_cmd8(&v2)) {
                UARTWriteStr("sd_cmd8 error\n");
		return false;
	}
	// Try to get the SDIO card's operating condition
	if (!sdio_op_cond()) {
                 UARTWriteStr("sdio_cop_cond error\n");
		return false;
	}

	if (sd_mmc_card->type & CARD_TYPE_SD) {
		// Try to get the SD card's operating condition
		if (!sd_spi_op_cond(v2)) {
			// It is not a SD card
			sd_mmc_card->type = CARD_TYPE_MMC;
                         UARTWriteStr("sd_spi_op_cond error\n");
			return false;
		}

		/* The CRC on card is disabled by default.
		 * However, to be sure, the CRC OFF command is send.
		 * Unfortunately, specific SDIO card does not support it
		 * (H&D wireless card - HDG104 WiFi SIP)
		 * and the command is send only on SD card.
		 */
		if (!send_command(SDMMC_SPI_CMD59_CRC_ON_OFF, 0)) {
                         UARTWriteStr("CMD59 error\n");
			return false;
		}
	}
	// SD MEMORY
	if (sd_mmc_card->type & CARD_TYPE_SD) {
		// Get the Card-Specific Data
		if (!sd_mmc_cmd9_spi()) {
                         UARTWriteStr("CMD9 error\n");
			return false;
		}
		sd_decode_csd();
		// Read the SCR to get card version
		if (!sd_acmd51()) {
                        UARTWriteStr("CMD51 error\n");
			return false;
		}
	}
//	if (IS_SDIO()) {
//		if (!sdio_get_max_speed()) {
//			return false;
//		}
//	}
	// SD MEMORY not HC, Set default block size
	if ((sd_mmc_card->type & CARD_TYPE_SD) &&
			(0 == (sd_mmc_card->type & CARD_TYPE_HC))) {
		if (!send_command(SDMMC_CMD16_SET_BLOCKLEN, SD_MMC_BLOCK_SIZE)) {
                        UARTWriteStr("CMD16 error\n");
			return false;
		}
	}
	// Check communication
	if (sd_mmc_card->type & CARD_TYPE_SD) {
		if (!sd_mmc_cmd13()) {
                        UARTWriteStr("CMD13 error\n");
			return false;
		}
	}
	// Reinitialize the slot with the new speed
spi_set_baudrate_div(SPI_MASTER_BASE, SPI_CHIP_SEL,
			(sysclk_get_cpu_hz() / sd_mmc_card->clock));
                        UARTWriteStr("Test Success\n");
	return true;
}
