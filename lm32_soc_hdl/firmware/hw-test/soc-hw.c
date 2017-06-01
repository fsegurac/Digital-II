#include "soc-hw.h"

uart_t  *uart0  = ( uart_t * )  0x20000000;
timer_t *timer0 = ( timer_t *)  0x30000000;
gpio_t  *gpio0  = ( gpio_t * )  0x40000000;
spi_t   *spi0   = ( spi_t *  )  0x50000000;
i2c_t   *i2c0   = ( i2c_t *  )  0x60000000;
uart_t  *uart1  = ( uart_t * )  0x70000000;

isr_ptr_t isr_table[32];

void prueba()
{
	   uart0->rxtx=30;
       uart1->rxtx=31;
	   timer0->tcr0 = 0xAA;
	   gpio0->ctrl=0x55;
	   spi0->rxtx=1;
	   spi0->nop1=2;
	   spi0->cs=3;
	   spi0->divisor=4;
	   spi0->nop2=5;

}
void tic_isr();
/***************************************************************************
 * IRQ handling
 */
void isr_null()
{
}

void irq_handler(uint32_t pending)
{
	int i;

	for(i=0; i<32; i++) {
		if (pending & 0x01) (*isr_table[i])();
		pending >>= 1;
	}
}

void isr_init()
{
	int i;
	for(i=0; i<32; i++)
		isr_table[i] = &isr_null;
}

void isr_register(int irq, isr_ptr_t isr)
{
	isr_table[irq] = isr;
}

void isr_unregister(int irq)
{
	isr_table[irq] = &isr_null;
}

/***************************************************************************
 * TIMER Functions
 */
void msleep(uint32_t msec)
{
	uint32_t tcr;

	// Use timer0.1
	timer0->compare1 = (FCPU/1000)*msec;
	timer0->counter1 = 0;
	timer0->tcr1 = TIMER_EN| TIMER_IRQEN;

	do {
		//halt();
 		tcr = timer0->tcr1;
 	} while ( ! (tcr & TIMER_TRIG) );
}

void nsleep(uint32_t nsec)
{
	uint32_t tcr;

	// Use timer0.1
	timer0->compare1 = (FCPU/1000000)*nsec;
	timer0->counter1 = 0;
	timer0->tcr1 = TIMER_EN| TIMER_IRQEN;

	do {
		//halt();
 		tcr = timer0->tcr1;
 	} while ( ! (tcr & TIMER_TRIG) );
}

uint32_t tic_msec;

void tic_isr()
{
	tic_msec++;
	timer0->tcr0     = TIMER_EN | TIMER_AR | TIMER_IRQEN;
}

void tic_init()
{
	tic_msec = 0;

	// Setup timer0.0
	timer0->compare0 = (FCPU/10000);
	timer0->counter0 = 0;
	timer0->tcr0     = TIMER_EN | TIMER_AR | TIMER_IRQEN;

	isr_register(1, &tic_isr);
}

/***************************************************************************
 * UART WIFI ESP8266 Functions
 */

void init_wifi(){ //configurar el modulo como servidor con puerto 80

    int c = 0;
   /* while(c==0){
		uart_putstr1(" AT\r\n");
		c = ok();
	}

	msleep(20);
	c = 0;*/
	while(c==0){
		uart_putstr1(" AT+RST\r\n");
		c = ok();
	}

    msleep(200);
    c = 0;
    while(c==0){
		uart_putstr1("AT+CIPMUX=1\r\n");
		c = ok();
	}

    msleep(20);
    c = 0;
	while (c==0){
		uart_putstr1("AT+CIPSERVER=1,80\r\n");
		c = ok();
	}

    msleep(20);
    c = 0;
    while(c==0){
		uart_putstr1("AT+CSWAP_CUR=\"guardDog\",\"mydog\",5,3\r\n");
		c = ok();
	}

}

void wifi_putchar(char a){
	int c = 0; 	  
	while(c == 0){
		uart_putstr1("AT+CIPSEND=0,1\r\n");
        msleep(20);
        uart_gen_putchar(a);
		c = ok();
        msleep(100);
	}
    c = 0;
    while(c==0){
        uart_putstr1("AT+CIPCOLSE=0\r\n");
        c = ok();
    }
}

void wifi_putstr(char *a){
	int c = 0;   
	char *cc = a;
	int counter = 0;
	while(*cc) {
		uart_putchar1(*cc);
		cc++;
		counter ++;
	}

	while(c == 0){
		uart_putstr1("AT+CIPSEND=0,");
		uart_putstr1(counter);
		uart_putstr1("\r\n");
		//uart_putstr("hala");		//posible control de salida
		msleep(20);
		uart_gen_putchar(a);
		c = ok();
		msleep(20);
	}
	c = 0; 	  
	while(c == 0){
		uart_putstr1("AT+CIPCLOSE=0\r\n");
		c = ok();
	}
}


char wifi_getchar(){
	char c='\n';
	int i=0;
	for(i=0;i<20;i++){
		c = uart_getchar1();
		if (c ==':'){
			c = uart_getchar1();
			return c;
		}
	}
	return '\n';
}

int ok(){
	int i=0;
	char a;
	for(i=0;i<30;i++){
		a=uart_getchar1();
		if(a=='K'){
			return 1;
		}
	}
	return 0;
}
/***************************************************************************
 * UART Functions
 */
void uart_init()
{
	//uart0->ier = 0x00;  // Interrupt Enable Register
	//uart0->lcr = 0x03;  // Line Control Register:    8N1
	//uart0->mcr = 0x00;  // Modem Control Register

	// Setup Divisor register (Fclk / Baud)
	//uart0->div = (FCPU/(57600*16));
}

char uart_getchar()
{   
	while (! (uart0->ucr & UART_DR)) ;
	return uart0->rxtx;
}

void uart_putchar(char c)
{
	while (uart0->ucr & UART_BUSY) ;
	uart0->rxtx = c;
}

void uart_putstr(char *str)
{
	char *c = str;
	while(*c) {
		uart_putchar(*c);
		c++;
	}
}
/**********************************************************************
##### UART1 #####
**********************************************************************/

char uart_getchar1()
{   
	while (! (uart1->ucr & UART_DR)) ;
	return uart1->rxtx;
}

void uart_putchar1(char c)
{   
	while (uart1->ucr & UART_BUSY) ;
	uart1->rxtx = c;
}

void uart_putstr1(char *str)
{
	char *c = str;
	while(*c) {
		uart_gen_putchar(*c);
		c++;
	}
}

void uart_gen_putchar(char c)
{
    uart_putchar1(c);
}

/***************************************************************************
 * i2c Functions
 */
