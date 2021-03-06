#include <init_my.h>

uint32_t pmc_is_locked_pllack(void)
{
	return (PMC->PMC_SR & PMC_SR_LOCKA);
}

uint32_t pmc_is_locked_upll(void)
{
	return (PMC->PMC_SR & PMC_SR_LOCKU);
}

uint32_t pmc_enable_periph_clk(uint32_t ul_id)
{
	if (ul_id > MAX_PERIPH_ID) {
		return 1;
	}

	if (ul_id < 32) {
		if ((PMC->PMC_PCSR0 & (1u << ul_id)) != (1u << ul_id)) {
			PMC->PMC_PCER0 = 1 << ul_id;
		}
#if (SAM3S || SAM3XA || SAM4S || SAM4E || SAM4C || SAM4CM || SAM4CP)
	} else {
		ul_id -= 32;
		if ((PMC->PMC_PCSR1 & (1u << ul_id)) != (1u << ul_id)) {
			PMC->PMC_PCER1 = 1 << ul_id;
		}
#endif
	}

	return 0;
}

uint32_t pmc_switch_mck_to_pllack(uint32_t ul_pres)
{
	uint32_t ul_timeout;

	PMC->PMC_MCKR = (PMC->PMC_MCKR & (~PMC_MCKR_PRES_Msk)) | ul_pres;
	for (ul_timeout = PMC_TIMEOUT; !(PMC->PMC_SR & PMC_SR_MCKRDY);
			--ul_timeout) {
		if (ul_timeout == 0) {
			return 1;
		}
	}

	PMC->PMC_MCKR = (PMC->PMC_MCKR & (~PMC_MCKR_CSS_Msk)) |
			PMC_MCKR_CSS_PLLA_CLK;

	for (ul_timeout = PMC_TIMEOUT; !(PMC->PMC_SR & PMC_SR_MCKRDY);
			--ul_timeout) {
		if (ul_timeout == 0) {
			return 1;
		}
	}

	return 0;
}

void pmc_disable_pllack()
{
  PMC->CKGR_PLLAR = CKGR_PLLAR_ONE | CKGR_PLLAR_MULA(0);

}

uint32_t osc_get_rate(uint32_t ul_id)
{
	switch (ul_id) {
	case OSC_SLCK_32K_RC:
		return OSC_SLCK_32K_RC_HZ;

	case OSC_SLCK_32K_XTAL:
		return BOARD_FREQ_SLCK_XTAL;

	case OSC_SLCK_32K_BYPASS:
		return BOARD_FREQ_SLCK_BYPASS;

	case OSC_MAINCK_4M_RC:
		return OSC_MAINCK_4M_RC_HZ;

	case OSC_MAINCK_8M_RC:
		return OSC_MAINCK_8M_RC_HZ;

	case OSC_MAINCK_12M_RC:
		return OSC_MAINCK_12M_RC_HZ;

	case OSC_MAINCK_XTAL:
		return BOARD_FREQ_MAINCK_XTAL;

	case OSC_MAINCK_BYPASS:
		return BOARD_FREQ_MAINCK_BYPASS;
	}

	return 0;
}

uint32_t sysclk_get_main_hz()
{
#if (defined CONFIG_SYSCLK_DEFAULT_RETURNS_SLOW_OSC)
	if (!sysclk_initialized ) {
		return OSC_MAINCK_4M_RC_HZ;
	}
#endif

	/* Config system clock setting */
	if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_SLCK_RC) {
		return OSC_SLCK_32K_RC_HZ;
	} else if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_SLCK_XTAL) {
		return OSC_SLCK_32K_XTAL_HZ;
	} else if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_SLCK_BYPASS) {
		return OSC_SLCK_32K_BYPASS_HZ;
	} else if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_MAINCK_4M_RC) {
		return OSC_MAINCK_4M_RC_HZ;
	} else if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_MAINCK_8M_RC) {
		return OSC_MAINCK_8M_RC_HZ;
	} else if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_MAINCK_12M_RC) {
		return OSC_MAINCK_12M_RC_HZ;
	} else if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_MAINCK_XTAL) {
		return OSC_MAINCK_XTAL_HZ;
	} else if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_MAINCK_BYPASS) {
		return OSC_MAINCK_BYPASS_HZ;
	}
#ifdef CONFIG_PLL0_SOURCE
	else if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_PLLACK) {
		return pll_get_default_rate(0);
	}
#endif

#ifdef CONFIG_PLL1_SOURCE
	else if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_UPLLCK) {
		return PLL_UPLL_HZ;
	}
#endif
	else {
		/* unhandled_case(CONFIG_SYSCLK_SOURCE); */
		return 0;
	}
}

uint32_t sysclk_get_cpu_hz(void)
{
	/* CONFIG_SYSCLK_PRES is the register value for setting the expected */
	/* prescaler, not an immediate value. */
	return sysclk_get_main_hz() /
		((CONFIG_SYSCLK_PRES == SYSCLK_PRES_3) ? 3 :
			(1 << (CONFIG_SYSCLK_PRES >> PMC_MCKR_PRES_Pos)));
}

void pll_config_init(struct pll_config *p_cfg,
		enum pll_source e_src, uint32_t ul_div, uint32_t ul_mul)
{
	uint32_t vco_hz;

	Assert_my(e_src < PLL_NR_SOURCES);

	if (ul_div == 0 && ul_mul == 0) { /* Must only be true for UTMI PLL */
		p_cfg->ctrl = CKGR_UCKR_UPLLCOUNT(PLL_COUNT);
	} else { /* PLLA */
		/* Calculate internal VCO frequency */
		vco_hz = osc_get_rate(e_src) / ul_div;
		Assert_my(vco_hz >= PLL_INPUT_MIN_HZ);
		Assert_my(vco_hz <= PLL_INPUT_MAX_HZ);

		vco_hz *= ul_mul;
		Assert_my(vco_hz >= PLL_OUTPUT_MIN_HZ);
		Assert_my(vco_hz <= PLL_OUTPUT_MAX_HZ);

		/* PMC hardware will automatically make it mul+1 */
		p_cfg->ctrl = CKGR_PLLAR_MULA(ul_mul - 1) | CKGR_PLLAR_DIVA(ul_div) | CKGR_PLLAR_PLLACOUNT(PLL_COUNT);
	}
}

uint32_t pll_is_locked(uint32_t ul_pll_id)
{
	Assert_my(ul_pll_id < NR_PLLS);

	if (ul_pll_id == PLLA_ID) {
		return pmc_is_locked_pllack();
	} else {
		return pmc_is_locked_upll();
	}
}

int pll_wait_for_lock(unsigned int pll_id)
{
	Assert_my(pll_id < NR_PLLS);

	while (!pll_is_locked(pll_id)) {
		/* Do nothing */
	}

	return 0;
}

void pll_enable(const struct pll_config *p_cfg, uint32_t ul_pll_id)
{
	Assert_my(ul_pll_id < NR_PLLS);

	if (ul_pll_id == PLLA_ID) {
		pmc_disable_pllack(); // Always stop PLL first!
		PMC->CKGR_PLLAR = CKGR_PLLAR_ONE | p_cfg->ctrl;
	} else {
		PMC->CKGR_UCKR = p_cfg->ctrl | CKGR_UCKR_UPLLEN;
	}
}

void arch_ioport_init()
{
#ifdef ID_PIOA
	pmc_enable_periph_clk(ID_PIOA);
#endif
#ifdef ID_PIOB
	pmc_enable_periph_clk(ID_PIOB);
#endif
#ifdef ID_PIOC
	pmc_enable_periph_clk(ID_PIOC);
#endif
#ifdef ID_PIOD
	pmc_enable_periph_clk(ID_PIOD);
#endif
#ifdef ID_PIOE
	pmc_enable_periph_clk(ID_PIOE);
#endif
#ifdef ID_PIOF
	pmc_enable_periph_clk(ID_PIOF);
#endif
}

void clock_init()
{
struct pll_config pllcfg;

  PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCXTBY) |
		CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCXTEN |
		CKGR_MOR_MOSCXTST(pmc_us_to_moscxtst(BOARD_OSC_STARTUP_US, OSC_SLCK_32K_RC_HZ));
  /* Wait the Xtal to stabilize */
  while (!(PMC->PMC_SR & PMC_SR_MOSCXTS));

  PMC->CKGR_MOR |= CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCSEL;

  pll_config_defaults(&pllcfg, 0);
  pll_enable(&pllcfg, 0);
  pmc_switch_mck_to_pllack(CONFIG_SYSCLK_PRES);
  SystemCoreClockUpdate();
  system_init_flash(sysclk_get_cpu_hz());
  //delay(1);
}


Pio *pio_get_pin_group(uint32_t ul_pin)
{
	Pio *p_pio;

#if (SAM4C || SAM4CP)
#  ifdef ID_PIOD
	if (ul_pin > PIO_PC9_IDX) {
		p_pio = PIOD;
	} else if (ul_pin > PIO_PB31_IDX) {
#  else
	if  (ul_pin > PIO_PB31_IDX) {
#  endif
		p_pio = PIOC;
	} else {
		p_pio = (Pio *)((uint32_t)PIOA + (PIO_DELTA * (ul_pin >> 5)));
	}
#elif (SAM4CM)
	if (ul_pin > PIO_PB21_IDX) {
		p_pio = PIOC;
	} else {
		p_pio = (Pio *)((uint32_t)PIOA + (PIO_DELTA * (ul_pin >> 5)));
	}
#else
	p_pio = (Pio *)((uint32_t)PIOA + (PIO_DELTA * (ul_pin >> 5)));
#endif
	return p_pio;
}

void pio_set_peripheral(Pio *p_pio, const pio_type_t ul_type,
		const uint32_t ul_mask)
{
	uint32_t ul_sr;

	/* Disable interrupts on the pin(s) */
	p_pio->PIO_IDR = ul_mask;

	switch (ul_type) {
	case PIO_PERIPH_A:
		ul_sr = p_pio->PIO_ABSR;
		p_pio->PIO_ABSR &= (~ul_mask & ul_sr);
		break;

	case PIO_PERIPH_B:
		ul_sr = p_pio->PIO_ABSR;
		p_pio->PIO_ABSR = (ul_mask | ul_sr);
		break;

		// other types are invalid in this function
	case PIO_INPUT:
	case PIO_OUTPUT_0:
	case PIO_OUTPUT_1:
	case PIO_NOT_A_PIN:
		return;
	}


	/* Remove the pins from under the control of PIO */
	p_pio->PIO_PDR = ul_mask;
}

void pio_pull_up(Pio *p_pio, const uint32_t ul_mask,
		const uint32_t ul_pull_up_enable)
{
	/* Enable the pull-up(s) if necessary */
	if (ul_pull_up_enable) {
		p_pio->PIO_PUER = ul_mask;
	} else {
		p_pio->PIO_PUDR = ul_mask;
	}
}

void pio_disable_interrupt(Pio *p_pio, const uint32_t ul_mask)
{
	p_pio->PIO_IDR = ul_mask;
}

void pio_set_input(Pio *p_pio, const uint32_t ul_mask,
		const uint32_t ul_attribute)
{
	pio_disable_interrupt(p_pio, ul_mask);
	pio_pull_up(p_pio, ul_mask, ul_attribute & PIO_PULLUP);

	/* Enable Input Filter if necessary */
	if (ul_attribute & (PIO_DEGLITCH | PIO_DEBOUNCE)) {
		p_pio->PIO_IFER = ul_mask;
	} else {
		p_pio->PIO_IFDR = ul_mask;
	}


	/* Enable de-glitch or de-bounce if necessary */
	if (ul_attribute & PIO_DEGLITCH) {
		p_pio->PIO_SCIFSR = ul_mask;
	} else {
		if (ul_attribute & PIO_DEBOUNCE) {
			p_pio->PIO_DIFSR = ul_mask;
		}
	}


	/* Configure pin as input */
	p_pio->PIO_ODR = ul_mask;
	p_pio->PIO_PER = ul_mask;
}

void pio_set_output(Pio *p_pio, const uint32_t ul_mask,
		const uint32_t ul_default_level,
		const uint32_t ul_multidrive_enable,
		const uint32_t ul_pull_up_enable)
{
	pio_disable_interrupt(p_pio, ul_mask);
	pio_pull_up(p_pio, ul_mask, ul_pull_up_enable);

	/* Enable multi-drive if necessary */
	if (ul_multidrive_enable) {
		p_pio->PIO_MDER = ul_mask;
	} else {
		p_pio->PIO_MDDR = ul_mask;
	}

	/* Set default value */
	if (ul_default_level) {
		p_pio->PIO_SODR = ul_mask;
	} else {
		p_pio->PIO_CODR = ul_mask;
	}

	/* Configure pin(s) as output(s) */
	p_pio->PIO_OER = ul_mask;
	p_pio->PIO_PER = ul_mask;
}

uint32_t pio_configure_pin(uint32_t ul_pin, const uint32_t ul_flags)
{
	Pio *p_pio = pio_get_pin_group(ul_pin);

	/* Configure pins */
	switch (ul_flags & PIO_TYPE_Msk) {
	case PIO_TYPE_PIO_PERIPH_A:
		pio_set_peripheral(p_pio, PIO_PERIPH_A, (1 << (ul_pin & 0x1F)));
		pio_pull_up(p_pio, (1 << (ul_pin & 0x1F)),
				(ul_flags & PIO_PULLUP));
		break;
	case PIO_TYPE_PIO_PERIPH_B:
		pio_set_peripheral(p_pio, PIO_PERIPH_B, (1 << (ul_pin & 0x1F)));
		pio_pull_up(p_pio, (1 << (ul_pin & 0x1F)),
				(ul_flags & PIO_PULLUP));
		break;
#if (SAM3S || SAM3N || SAM4S || SAM4E || SAM4N || SAM4C || SAM4CP || SAM4CM)
	case PIO_TYPE_PIO_PERIPH_C:
		pio_set_peripheral(p_pio, PIO_PERIPH_C, (1 << (ul_pin & 0x1F)));
		pio_pull_up(p_pio, (1 << (ul_pin & 0x1F)),
				(ul_flags & PIO_PULLUP));
		break;
	case PIO_TYPE_PIO_PERIPH_D:
		pio_set_peripheral(p_pio, PIO_PERIPH_D, (1 << (ul_pin & 0x1F)));
		pio_pull_up(p_pio, (1 << (ul_pin & 0x1F)),
				(ul_flags & PIO_PULLUP));
		break;
#endif

	case PIO_TYPE_PIO_INPUT:
		pio_set_input(p_pio, (1 << (ul_pin & 0x1F)), ul_flags);
		break;

	case PIO_TYPE_PIO_OUTPUT_0:
	case PIO_TYPE_PIO_OUTPUT_1:
		pio_set_output(p_pio, (1 << (ul_pin & 0x1F)),
				((ul_flags & PIO_TYPE_PIO_OUTPUT_1)
				== PIO_TYPE_PIO_OUTPUT_1) ? 1 : 0,
				(ul_flags & PIO_OPENDRAIN) ? 1 : 0,
				(ul_flags & PIO_PULLUP) ? 1 : 0);
		break;

	default:
		return 0;
	}

	return 1;
}

void setup_io_pin()
{
    /* Enable PIO clocks */
  PMC->PMC_PCER0 = (1 << ID_PIOB) ;
  /* Setup LEDs */
  PIOB->PIO_SODR = LED;
  PIOB->PIO_PER = LED;
  PIOB->PIO_OER = LED;


}

void boardio_init()
{
  WDT->WDT_MR = WDT_MR_WDDIS;   //disable watchdog

  arch_ioport_init();
  pio_configure_pin(SPI0_MISO_GPIO, SPI0_MISO_FLAGS);
  pio_configure_pin(SPI0_MOSI_GPIO, SPI0_MOSI_FLAGS);
  pio_configure_pin(SPI0_SPCK_GPIO, SPI0_SPCK_FLAGS);
  pio_configure_pin(SPI0_NPCS0_GPIO, SPI0_NPCS0_FLAGS);
}
