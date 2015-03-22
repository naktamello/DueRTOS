#include <dht11.h>

//uint32_t micros( void )
//{
//    uint32_t ticks, ticks2;
//    uint32_t pend, pend2;
//    uint32_t count, count2;
//
//    ticks2  = SysTick->VAL;
//    pend2   = !!((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk)||((SCB->SHCSR & SCB_SHCSR_SYSTICKACT_Msk)))  ;
//    count2  = GetTickCount();
//
//    do {
//        ticks=ticks2;
//        pend=pend2;
//        count=count2;
//        ticks2  = SysTick->VAL;
//        pend2   = !!((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk)||((SCB->SHCSR & SCB_SHCSR_SYSTICKACT_Msk)))  ;
//        count2  = GetTickCount();
//    } while ((pend != pend2) || (count != count2) || (ticks < ticks2));
//
//    return ((count+pend) * 1000) + (((SysTick->LOAD  - ticks)*(1048576/(F_CPU/1000000)))>>20) ; 
//    // this is an optimization to turn a runtime division into two compile-time divisions and 
//    // a runtime multiplication and shift, saving a few cycles
//}


 uint32_t micros( void )
 {
     uint32_t ticks ;
     uint32_t count ;
 
     SysTick->CTRL;
     do {
         ticks = SysTick->VAL;
         count = GetTickCount();
     } while (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk);
 
     return count * 1000 + (SysTick->LOAD + 1 - ticks) / (SystemCoreClock/1000000) ;
 }

void dht_init(void)
{
pmc_enable_periph_clk(DHT_PORT_ID);
	pmc_enable_periph_clk(DHT_PORT_ID);

	pio_configure_pin(PIO_PC12_IDX, (0x3u << PIO_TYPE_Pos) | PIO_DEFAULT);

	pio_set_output(PIOC, DHT_PIN, 1, 0, 0);
	pio_set(DHT_PORT, DHT_PIN);
}

uint32_t dht_read_sensor(uint8_t *bits)
{
	volatile uint32_t counter = 0;
	uint32_t j = 0, i;
	volatile uint32_t retry = 0;
	
	// EMPTY BUFFER
	for (i = 0; i < 5; i++) bits[i] = 0;

	// REQUEST SAMPLE
	pio_clear(DHT_PORT, DHT_PIN);
	delay_ms(DHTLIB_DHT11_WAKEUP);


	pio_set(DHT_PORT, DHT_PIN);
	delay_us(20);

	pio_set_input(PIOC, DHT_PIN, PIO_DEFAULT);

	
	while ((pio_get(DHT_PORT, PIO_TYPE_PIO_INPUT, DHT_PIN) == 0) && (retry<85))
	{
		retry++;
		delay_us(1);
	}
	
	retry = 0;
	while ((pio_get(DHT_PORT, PIO_TYPE_PIO_INPUT, DHT_PIN) == 1) && (retry<85))
	{
		retry++;
		delay_us(2);
	}
	
	

	for ( i = 0; i < NUM_BYTES; i++) {
		
		for (j = 0; j < NUM_BITS; j++){
			counter = 0;
			
			//50us low signal before each bit
			retry = 0;
			while ((pio_get(DHT_PORT, PIO_TYPE_PIO_INPUT, DHT_PIN) == 0) && (retry<60))
			{
				retry++;
				delay_us(2);
			}
			
			while ((pio_get(DHT_PORT, PIO_TYPE_PIO_INPUT, DHT_PIN) == 1) && (counter<80))
			{
				counter++;
				delay_us(2);
			}
			
			if (counter > 10)
			bits[i] |= (1 << (NUM_BITS-1-j));
		}
		
	}


	
	pio_set_output(PIOC, DHT_PIN, 1, 0, 0);
	pio_set(DHT_PORT, DHT_PIN);
	
	if (((i*j) >= 40) &&
	(bits[4] == ((bits[0] + bits[1] + bits[2] + bits[3]) & 0xFF)) ) {
		return true;
	}

	return false;
}
