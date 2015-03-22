#ifndef SD_SPI_MY_H
#define SD_SPI_MY_H

#include <stdint.h>
#include <spi_my.h>
#include <sd_mmc_protocol.h>
#include <init_my.h>
#include <compiler.h>
#include <uart.h>

// Define to enable the SPI mode instead of Multimedia Card interface mode
#define SD_MMC_SPI_MODE
// Define to enable the SDIO support
//#define SDIO_SUPPORT_ENABLE
#ifdef SDIO_SUPPORT_ENABLE
#  define IS_SDIO()  (sd_mmc_card->type & CARD_TYPE_SDIO)
#else
#  define IS_SDIO()  false
#endif


#    define SD_MMC_SPI_MEM_CNT          1
 //Optional card detect pin and write protection pin
#    define SD_MMC_0_CD_GPIO            (PIO_PA17_IDX)
#    define SD_MMC_0_CD_PIO_ID          ID_PIOA
#    define SD_MMC_0_CD_FLAGS           (PIO_INPUT | PIO_PULLUP)
#    define SD_MMC_0_CD_DETECT_VALUE    0
#    define SD_MMC_SPI_0_CS             0
#    define SD_MMC_SPI                  SPI0
#    define SPI_NPCS0_GPIO              (PIO_PA28_IDX)
#    define SPI_NPCS0_FLAGS             (PIO_PERIPH_A | PIO_DEFAULT)
#    define SPI_MISO_GPIO               (PIO_PA25_IDX)
#    define SPI_MISO_FLAGS              (PIO_PERIPH_A | PIO_DEFAULT)
#    define SPI_MOSI_GPIO               (PIO_PA26_IDX)
#    define SPI_MOSI_FLAGS              (PIO_PERIPH_A | PIO_DEFAULT)
#    define SPI_SPCK_GPIO               (PIO_PA27_IDX)
#    define SPI_SPCK_FLAGS              (PIO_PERIPH_A | PIO_DEFAULT)


#    define driver  sd_mmc_spi
#    define SD_MMC_MEM_CNT        SD_MMC_SPI_MEM_CNT
#    define sd_mmc_is_spi()       true

typedef uint8_t sd_mmc_err_t; //!< Type of return error code
typedef uint8_t sd_mmc_spi_errno_t;


//! \name Return error codes
//! @{
#define SD_MMC_OK               0    //! No error
#define SD_MMC_INIT_ONGOING     1    //! Card not initialized
#define SD_MMC_ERR_NO_CARD      2    //! No SD/MMC card inserted
#define SD_MMC_ERR_UNUSABLE     3    //! Unusable card
#define SD_MMC_ERR_SLOT         4    //! Slot unknow
#define SD_MMC_ERR_COMM         5    //! General communication error
#define SD_MMC_ERR_PARAM        6    //! Illeage input parameter
#define SD_MMC_ERR_WP           7    //! Card write protected
//! @}

//! \name Return error codes
//! @{
#define SD_MMC_SPI_NO_ERR                 0 //! No error
#define SD_MMC_SPI_ERR                    1 //! General or an unknown error
#define SD_MMC_SPI_ERR_RESP_TIMEOUT       2 //! Timeout during command
#define SD_MMC_SPI_ERR_RESP_BUSY_TIMEOUT  3 //! Timeout on busy signal of R1B response
#define SD_MMC_SPI_ERR_READ_TIMEOUT       4 //! Timeout during read operation
#define SD_MMC_SPI_ERR_WRITE_TIMEOUT      5 //! Timeout during write operation
#define SD_MMC_SPI_ERR_RESP_CRC           6 //! Command CRC error
#define SD_MMC_SPI_ERR_READ_CRC           7 //! CRC error during read operation
#define SD_MMC_SPI_ERR_WRITE_CRC          8 //! CRC error during write operation
#define SD_MMC_SPI_ERR_ILLEGAL_COMMAND    9 //! Command not supported
#define SD_MMC_SPI_ERR_WRITE             10 //! Error during write operation
#define SD_MMC_SPI_ERR_OUT_OF_RANGE      11 //! Data access out of range
//! @}

typedef uint8_t card_type_t; //!< Type of card type

//! \name Card Types
//! @{
#define CARD_TYPE_UNKNOWN   (0)      //!< Unknown type card
#define CARD_TYPE_SD        (1 << 0) //!< SD card
#define CARD_TYPE_MMC       (1 << 1) //!< MMC card
#define CARD_TYPE_SDIO      (1 << 2) //!< SDIO card
#define CARD_TYPE_HC        (1 << 3) //!< High capacity card
//! SD combo card (io + memory)
#define CARD_TYPE_SD_COMBO  (CARD_TYPE_SD | CARD_TYPE_SDIO)
//! @}

typedef uint8_t card_version_t; //!< Type of card version

//! \name Card Versions
//! @{
#define CARD_VER_UNKNOWN   (0)       //! Unknown card version
#define CARD_VER_SD_1_0    (0x10)    //! SD version 1.0 and 1.01
#define CARD_VER_SD_1_10   (0x1A)    //! SD version 1.10
#define CARD_VER_SD_2_0    (0X20)    //! SD version 2.00
#define CARD_VER_SD_3_0    (0X30)    //! SD version 3.0X
#define CARD_VER_MMC_1_2   (0x12)    //! MMC version 1.2
#define CARD_VER_MMC_1_4   (0x14)    //! MMC version 1.4
#define CARD_VER_MMC_2_2   (0x22)    //! MMC version 2.2
#define CARD_VER_MMC_3     (0x30)    //! MMC version 3
#define CARD_VER_MMC_4     (0x40)    //! MMC version 4
//! @}

//! This SD MMC stack uses the maximum block size autorized (512 bytes)
#define SD_MMC_BLOCK_SIZE          512

#define CSD_REG_BIT_SIZE            128 //!< 128 bits
#define CSD_REG_BSIZE               (CSD_REG_BIT_SIZE / 8) //!< 16 bytes

#define SD_MMC_VOLTAGE_SUPPORT \
		(OCR_VDD_27_28 | OCR_VDD_28_29 | \
		OCR_VDD_29_30 | OCR_VDD_30_31 | \
		OCR_VDD_31_32 | OCR_VDD_32_33)


void send_clock(void);
static uint8_t sd_mmc_spi_crc7(uint8_t * buf, uint8_t size);
static bool sd_mmc_spi_wait_busy(void);
bool send_command(sdmmc_cmd_def_t cmd, uint32_t arg);
bool adtc_start(sdmmc_cmd_def_t cmd, uint32_t arg,
		uint16_t block_size, uint16_t nb_block, bool access_block);
static bool sd_cmd8(uint8_t * v2);
static bool sdio_op_cond(void);
static bool sd_spi_op_cond(uint8_t v2);
static bool sd_mmc_spi_start_read_block(void);
static void sd_mmc_spi_stop_read_block(void);
bool sd_mmc_spi_start_read_blocks(void *dest, uint16_t nb_block);
static bool sd_mmc_cmd9_spi(void);
static void sd_decode_csd(void);
static bool sd_acmd51(void);
static bool sd_mmc_cmd13(void);
bool sd_mmc_spi_card_init(void);

#endif