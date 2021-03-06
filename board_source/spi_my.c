#include <spi_my.h>

void spi_enable(Spi *p_spi)
{
	p_spi->SPI_CR = SPI_CR_SPIEN;
}
void spi_disable(Spi *p_spi)
{
	p_spi->SPI_CR = SPI_CR_SPIDIS;
}
void spi_set_lastxfer(Spi *p_spi)
{
	p_spi->SPI_CR = SPI_CR_LASTXFER;
}
void spi_set_master_mode(Spi *p_spi)
{
	p_spi->SPI_MR |= SPI_MR_MSTR;
}
uint32_t spi_get_peripheral_select_mode(Spi *p_spi)
{
	if (p_spi->SPI_MR & SPI_MR_PS) {
		return 1;
	} else {
		return 0;
	}
}
void spi_disable_mode_fault_detect(Spi *p_spi)
{
	p_spi->SPI_MR |= SPI_MR_MODFDIS;
}
void spi_reset(Spi *p_spi)
{
	p_spi->SPI_CR = SPI_CR_SWRST;
}

void spi_enable_clock(Spi *p_spi)
{
#if (SAM4S || SAM3S || SAM3N || SAM3U || SAM4E || SAM4N || SAMG)
	UNUSED(p_spi);
	sysclk_enable_peripheral_clock(ID_SPI);
#elif (SAM3XA || SAM4C || SAM4CP || SAM4CM)
	if (p_spi == SPI0) {
		sysclk_enable_peripheral_clock(ID_SPI0);
	}
	#ifdef SPI1
	else if (p_spi == SPI1) {
		sysclk_enable_peripheral_clock(ID_SPI1);
	}
	#endif
#elif SAM4L
	sysclk_enable_peripheral_clock(p_spi);
#endif
}
void spi_set_peripheral_chip_select_value(Spi *p_spi, uint32_t ul_value)
{
	p_spi->SPI_MR &= (~SPI_MR_PCS_Msk);
	p_spi->SPI_MR |= SPI_MR_PCS(ul_value);
}
void spi_set_clock_polarity(Spi *p_spi, uint32_t ul_pcs_ch,
		uint32_t ul_polarity)
{
	if (ul_polarity) {
		p_spi->SPI_CSR[ul_pcs_ch] |= SPI_CSR_CPOL;
	} else {
		p_spi->SPI_CSR[ul_pcs_ch] &= (~SPI_CSR_CPOL);
	}
}

void spi_set_clock_phase(Spi *p_spi, uint32_t ul_pcs_ch, uint32_t ul_phase)
{
	if (ul_phase) {
		p_spi->SPI_CSR[ul_pcs_ch] |= SPI_CSR_NCPHA;
	} else {
		p_spi->SPI_CSR[ul_pcs_ch] &= (~SPI_CSR_NCPHA);
	}
}
void spi_set_cs_hold(Spi *p_spi, uint32_t ul_pcs_ch, uint32_t ul_hold)
{
        if (ul_hold) {
              p_spi->SPI_CSR[ul_pcs_ch] |= SPI_CSR_CSAAT;
        } else {
              p_spi->SPI_CSR[ul_pcs_ch] &= (~SPI_CSR_CSAAT);
	}
}
void spi_set_baudrate_div(Spi *p_spi, uint32_t ul_pcs_ch,
		uint8_t uc_baudrate_divider)
{
	p_spi->SPI_CSR[ul_pcs_ch] &= (~SPI_CSR_SCBR_Msk);
	p_spi->SPI_CSR[ul_pcs_ch] |= SPI_CSR_SCBR(uc_baudrate_divider);
}
void spi_set_bits_per_transfer(Spi *p_spi, uint32_t ul_pcs_ch,
		uint32_t ul_bits)
{
	p_spi->SPI_CSR[ul_pcs_ch] &= (~SPI_CSR_BITS_Msk);
	p_spi->SPI_CSR[ul_pcs_ch] |= ul_bits;
}
void spi_set_transfer_delay(Spi *p_spi, uint32_t ul_pcs_ch,
		uint8_t uc_dlybs, uint8_t uc_dlybct)
{
	p_spi->SPI_CSR[ul_pcs_ch] &= ~(SPI_CSR_DLYBS_Msk | SPI_CSR_DLYBCT_Msk);
	p_spi->SPI_CSR[ul_pcs_ch] |= SPI_CSR_DLYBS(uc_dlybs)
			| SPI_CSR_DLYBCT(uc_dlybct);
}


spi_status_t spi_write(Spi *p_spi, uint16_t us_data,
		uint8_t uc_pcs, uint8_t uc_last)
{
	uint32_t timeout = SPI_TIMEOUT;
	uint32_t value;

	while (!(p_spi->SPI_SR & SPI_SR_TDRE)) {
		if (!timeout--) {
			return SPI_ERROR_TIMEOUT;
		}
	}

	if (spi_get_peripheral_select_mode(p_spi)) {
		value = SPI_TDR_TD(us_data) | SPI_TDR_PCS(uc_pcs);
		if (uc_last) {
			value |= SPI_TDR_LASTXFER;
		}
	} else {
		value = SPI_TDR_TD(us_data);
	}

	p_spi->SPI_TDR = value;

	return SPI_OK;
}

void spi_master_transfer(void *p_buf, uint32_t size)
{
	uint32_t i;
	uint8_t uc_pcs;
	static uint16_t data;

	uint8_t *p_buffer;

	p_buffer = p_buf;

	for (i = 0; i < size; i++) {
		spi_write(SPI_MASTER_BASE, p_buffer[i], 0, 0);
//		/* Wait transfer done. */
//		while ((spi_read_status(SPI_MASTER_BASE) & SPI_SR_RDRF) == 0);
//		spi_read(SPI_MASTER_BASE, &data, &uc_pcs);
//		p_buffer[i] = data;
	}
}

void spi_put(Spi *p_spi, uint16_t data)
{
	p_spi->SPI_TDR = SPI_TDR_TD(data);
}

uint32_t spi_is_tx_ready(Spi *p_spi)
{
	if (p_spi->SPI_SR & SPI_SR_TDRE) {
		return 1;
	} else {
		return 0;
	}
}

uint32_t spi_is_rx_ready(Spi *p_spi)
{
	if ((p_spi->SPI_SR & (SPI_SR_RDRF | SPI_SR_TXEMPTY))
			== (SPI_SR_RDRF | SPI_SR_TXEMPTY)) {
		return 1;
	} else {
		return 0;
	}
}

status_code_t spi_read_packet(Spi *p_spi, uint8_t *data, size_t len)
{
	uint32_t timeout = SPI_TIMEOUT;
	uint8_t val;
	uint32_t i = 0;

	while (len) {
		timeout = SPI_TIMEOUT;
		while (!spi_is_tx_ready(p_spi)) {
			if (!timeout--) {
				return ERR_TIMEOUT;
			}
		}
		spi_put(p_spi, (uint16_t) 0xFF);

		timeout = SPI_TIMEOUT;
		while (!spi_is_rx_ready(p_spi)) {
			if (!timeout--) {
				return ERR_TIMEOUT;
			}
		}
		val = (p_spi->SPI_RDR & SPI_RDR_RD_Msk);

		data[i] = val;
		i++;
		len--;
	}

	return STATUS_OK;
}

void spi_master_initialize(void)
{
	/* Configure an SPI peripheral. */
        PMC->PMC_PCER0 = ( 1 << ID_SPI0);
	spi_enable_clock(SPI_MASTER_BASE);
	spi_disable(SPI_MASTER_BASE);
	spi_reset(SPI_MASTER_BASE);
	spi_set_lastxfer(SPI_MASTER_BASE);
	spi_set_master_mode(SPI_MASTER_BASE);
	spi_disable_mode_fault_detect(SPI_MASTER_BASE);
	spi_set_peripheral_chip_select_value(SPI_MASTER_BASE, SPI_CHIP_PCS);
	spi_set_clock_polarity(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_POLARITY);
	spi_set_clock_phase(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_PHASE);
	spi_set_bits_per_transfer(SPI_MASTER_BASE, SPI_CHIP_SEL,
			SPI_CSR_BITS_8_BIT);
        spi_set_cs_hold(SPI_MASTER_BASE, SPI_CHIP_SEL, 1);
//	spi_set_baudrate_div(SPI_MASTER_BASE, SPI_CHIP_SEL,
//			(sysclk_get_cpu_hz() / gs_ul_spi_clock));
int16_t baud_div = spi_calc_baudrate_div(115200, sysclk_get_cpu_hz());
	spi_set_baudrate_div(SPI_MASTER_BASE, SPI_CHIP_SEL, baud_div);
	spi_set_transfer_delay(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_DLYBS,
			SPI_DLYBCT);
	spi_enable(SPI_MASTER_BASE);
}

void spi_master_test(void)
{
  uint32_t cmd;
  uint32_t block;
  uint32_t i;

  cmd = 0x88191810;

 spi_master_transfer(&cmd, sizeof(cmd));
}

int16_t spi_calc_baudrate_div(const uint32_t baudrate, uint32_t mck)
{
	int baud_div = div_ceil(mck, baudrate);

	/* The value of baud_div is from 1 to 255 in the SCBR field. */
	if (baud_div <= 0 || baud_div > 255) {
		return -1;
	}

	return baud_div;
}