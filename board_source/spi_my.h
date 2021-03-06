#ifndef SPI_MY_H
#define SPI_MY_H

#include <stdint.h>
#include <stddef.h>
#include <init_my.h>
#include <sam3x8e.h>

#define div_ceil(a, b)      (((a) + (b) - 1) / (b))
#//define SPI0_asf       ((Spi_asf    *)0x40008000U) /**< \brief (SPI0      ) Base Address */
/** Spi Hw ID . */
#define SPI_ID          ID_SPI0

/** SPI base address for SPI master mode*/
#define SPI_MASTER_BASE      SPI0

/* Chip select. */
#define SPI_CHIP_SEL 0
#define SPI_CHIP_PCS spi_get_pcs(SPI_CHIP_SEL)

/* Clock polarity. */
#define SPI_CLK_POLARITY 0

/* Clock phase. */
#define SPI_CLK_PHASE 1

/* Delay before SPCK. */
#define SPI_DLYBS 0x00

/* Delay between consecutive transfers. */
#define SPI_DLYBCT 0x00

#define spi_get_pcs(chip_sel_id) ((~(1u<<(chip_sel_id)))&0xF)

/** Time-out value (number of attempts). */
#define SPI_TIMEOUT       15000

enum status_code {
	STATUS_OK               =  0, //!< Success
	STATUS_ERR_BUSY         =  0x19,
	STATUS_ERR_DENIED       =  0x1C,
	STATUS_ERR_TIMEOUT      =  0x12,
	ERR_IO_ERROR            =  -1, //!< I/O error
	ERR_FLUSHED             =  -2, //!< Request flushed from queue
	ERR_TIMEOUT             =  -3, //!< Operation timed out
	ERR_BAD_DATA            =  -4, //!< Data integrity check failed
	ERR_PROTOCOL            =  -5, //!< Protocol error
	ERR_UNSUPPORTED_DEV     =  -6, //!< Unsupported device
	ERR_NO_MEMORY           =  -7, //!< Insufficient memory
	ERR_INVALID_ARG         =  -8, //!< Invalid argument
	ERR_BAD_ADDRESS         =  -9, //!< Bad address
	ERR_BUSY                =  -10, //!< Resource is busy
	ERR_BAD_FORMAT          =  -11, //!< Data format not recognized
	ERR_NO_TIMER            =  -12, //!< No timer available
	ERR_TIMER_ALREADY_RUNNING   =  -13, //!< Timer already running
	ERR_TIMER_NOT_RUNNING   =  -14, //!< Timer not running
	ERR_ABORTED             =  -15, //!< Operation aborted by user
	/**
	 * \brief Operation in progress
	 *
	 * This status code is for driver-internal use when an operation
	 * is currently being performed.
	 *
	 * \note Drivers should never return this status code to any
	 * callers. It is strictly for internal use.
	 */
	OPERATION_IN_PROGRESS	= -128,
};

typedef enum status_code status_code_t;

/* SPI clock setting (Hz). */
static uint32_t gs_ul_spi_clock = 500000;

typedef enum
{
	SPI_ERROR = -1,
	SPI_OK = 0,
	SPI_ERROR_TIMEOUT = 1,
	SPI_ERROR_ARGUMENT,
	SPI_ERROR_OVERRUN,
	SPI_ERROR_MODE_FAULT,
	SPI_ERROR_OVERRUN_AND_MODE_FAULT
} spi_status_t;

void spi_enable(Spi *p_spi);

void spi_disable(Spi *p_spi);

void spi_set_lastxfer(Spi *p_spi);

void spi_set_master_mode(Spi *p_spi);

uint32_t spi_get_peripheral_select_mode(Spi *p_spi);

void spi_disable_mode_fault_detect(Spi *p_spi);

void spi_reset(Spi *p_spi);

void spi_enable_clock(Spi *p_spi);

void spi_set_peripheral_chip_select_value(Spi *p_spi, uint32_t ul_value);

void spi_set_clock_polarity(Spi *p_spi, uint32_t ul_pcs_ch,
		uint32_t ul_polarity);

void spi_set_clock_phase(Spi *p_spi, uint32_t ul_pcs_ch, uint32_t ul_phase);

void spi_set_cs_hold(Spi *p_spi, uint32_t ul_pcs_ch, uint32_t ul_hold);

void spi_set_baudrate_div(Spi *p_spi, uint32_t ul_pcs_ch,
		uint8_t uc_baudrate_divider);

void spi_set_bits_per_transfer(Spi *p_spi, uint32_t ul_pcs_ch,
		uint32_t ul_bits);

void spi_set_transfer_delay(Spi *p_spi, uint32_t ul_pcs_ch,
		uint8_t uc_dlybs, uint8_t uc_dlybct);

void spi_master_transfer(void *p_buf, uint32_t size);

void spi_put(Spi *p_spi, uint16_t data);

uint32_t spi_is_tx_ready(Spi *p_spi);

uint32_t spi_is_rx_ready(Spi *p_spi);

status_code_t spi_read_packet(Spi *p_spi, uint8_t *data, size_t len);

void spi_master_initialize(void);

void spi_master_test(void);

int16_t spi_calc_baudrate_div(const uint32_t baudrate, uint32_t mck);
//extern uint32_t sysclk_get_cpu_hz(void);

#endif