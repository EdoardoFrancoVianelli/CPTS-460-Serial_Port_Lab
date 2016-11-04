/**** The serial terminal data structure ****/

struct stty {
   /* input buffer */
   char inbuf[BUFLEN];
   int inhead, intail;
   struct semaphore inchars, inmutex;

   /* output buffer */
   char outbuf[BUFLEN];
   int outhead, outtail;
   struct semaphore outroom, outmutex;
   int tx_on;

   /* Control section */
   char echo;   /* echo inputs */
   char ison;   /* on or off */
   char erase, kill, intr, quit, x_on, x_off, eof;
   
   /* I/O port base address */
   int port;
} stty[NR_STTY];

/********  bgetc()/bputc() by polling *********/
int bputc(int port, int c)
{
    while ((in_byte(port+LSR) & 0x20) == 0);
    out_byte(port+DATA, c);
}

int bgetc(int port)
{
    while ((in_byte(port+LSR) & 0x01) == 0);
    return (in_byte(port+DATA) & 0x7F);
}

int enable_irq(u8 irq_nr)
{
   out_byte(0x21, in_byte(0x21) & ~(1 << irq_nr));
}

   
/************ serial ports initialization ***************/
char *p = "\n\rSerial Port Ready\n\r\007";

int sinit()
{
  int i;  
  struct stty *t;
  char *q; 

  /* initialize stty[] and serial ports */
  for (i = 0; i < NR_STTY; i++){
    q = p;

    printf("sinit : port #%d\n",i);

      t = &stty[i];

      /* initialize data structures and pointers */
      if (i==0)
          t->port = 0x3F8;    /* COM1 base address */
      else
          t->port = 0x2F8;    /* COM2 base address */  

      (t->inchars).value  = 0;  (t->inchars).queue = 0;
      (t->inmutex).value  = 1;  (t->inmutex).queue = 0;
      (t->outmutex).value = 1;  (t->outmutex).queue = 0;
      (t->outroom).value = BUFLEN; (t->outroom).queue = 0;

      t->inhead = t->intail = 0;
      t->outhead =t->outtail = 0;

      t->tx_on = 0;

      // initialize control chars; NOT used in MTX but show how anyway
      t->ison = t->echo = 1;   /* is on and echoing */
      t->erase = '\b';
      t->kill  = '@';
      t->intr  = (char)0177;  /* del */
      t->quit  = (char)034;   /* control-C */
      t->x_on  = (char)021;   /* control-Q */
      t->x_off = (char)023;   /* control-S */
      t->eof   = (char)004;   /* control-D */

    lock();  // CLI; no interrupts

      //out_byte(t->port+MCR,  0x09);  /* IRQ4 on, DTR on */ 
      out_byte(t->port+IER,  0x00);  /* disable serial port interrupts */

      out_byte(t->port+LCR,  0x80);  /* ready to use 3f9,3f8 as divisor */
      out_byte(t->port+DIVH, 0x00);
      out_byte(t->port+DIVL, 12);    /* divisor = 12 ===> 9600 bauds */

      /******** term 9600 /dev/ttyS0: 8 bits/char, no parity *************/ 
      out_byte(t->port+LCR, 0x03); 

      /*******************************************************************
        Writing to 3fc ModemControl tells modem : DTR, then RTS ==>
        let modem respond as a DCE.  Here we must let the (crossed)
        cable tell the TVI terminal that the "DCE" has DSR and CTS.  
        So we turn the port's DTR and RTS on.
      ********************************************************************/

      out_byte(t->port+MCR, 0x0B);  /* 1011 ==> IRQ4, RTS, DTR on   */
      out_byte(t->port+IER, 0x01);  /* Enable Rx interrupt, Tx off */

    unlock();
    
    enable_irq(4-i);  // COM1: IRQ4; COM2: IRQ3

    /* show greeting message */
    while (*q){
      bputc(t->port, *q);
      q++;
    }
  }
}  
         
int shandler(int port)
{  
   struct stty *t;
   int IntID, LineStatus, ModemStatus, intType, c;

   t = &stty[port];            /* IRQ 4 interrupt : COM1 = stty[0] */

   IntID     = in_byte(t->port+IIR);       /* read InterruptID Reg */
   LineStatus= in_byte(t->port+LSR);       /* read LineStatus  Reg */    
   ModemStatus=in_byte(t->port+MSR);       /* read ModemStatus Reg */

   intType = IntID & 7;     /* mask out all except the lowest 3 bits */
   switch(intType){
      case 6 : do_errors(t);  break;   /* 110 = errors */
      case 4 : do_rx(t);      break;   /* 100 = rx interrupt */
      case 2 : do_tx(t);      break;   /* 010 = tx interrupt */
      case 0 : do_modem(t);   break;   /* 000 = modem interrupt */
   }
   out_byte(0x20, 0x20);     /* reenable the 8259 controller */ 
}

//======================== LOWER HALF ROUTINES ===============================
int s0handler()
{
  shandler(0);
}

int s1handler()
{
  shandler(1);
}



int do_errors()
{ printf("assume no error\n"); }

int do_modem()
{  printf("don't have a modem\n"); }


// The following show how to enable and disable Tx interrupts 

enable_tx(struct stty *t)
{
	lock();
	out_byte(t->port+IER, 0x03);   // 0011 ==> both tx and rx on 
	t->tx_on = 1;
	unlock();
}

disable_tx(struct stty *t)
{ 
	lock();
	out_byte(t->port+IER, 0x01);   // 0001 ==> tx off, rx on 
	t->tx_on = 0;
	unlock();
}

int do_rx(struct stty *tty)   // interrupts already disabled 
{ 
	char c = in_byte(tty->port) & 0x7F;  // read the ASCII char from port 
	//printf("port %x interrupt:c=%c\n", tty->port, c,c);
	//printf("%c", c);
	if (c == '\r'){
		sputc(tty, '\n');
	}else {
		sputc(tty, c);
	}
	if (tty->inchars.value >= BUFLEN){
		out_byte(tty, BEEP);
		return;
	}

	if (c == 0x3){
		signal(2);
		c = '\n'; //force a line
	}

	tty->inbuf[tty->inhead++] = c; //put a char into inbuf
	tty->inhead %= BUFLEN; //advance inhead
	V(&tty->inchars); //increment inchars and unblock sgetc()
	//Write code to put c into inbuf[ ]; notify process of char available;  
}      
     
char sgetc(struct stty *tty)
{ 
	char c;
	// write Code to get a char from inbuf[ ]
	P(&tty->inchars);
	lock();
	c = tty->inbuf[tty->intail++];
	tty->intail %= BUFLEN;
	unlock();
	return(c);
}

int sgetline(struct stty *tty, char *line)
{  
	// write code to input a line from tty's inbuf[ ] 

	int count = 0;

	printf("sgetline ", running->pid);

	while (1){
		*line = sgetc(tty);
		if (*line == 0 || *line == '\n' || *line == '\r')
			break;
		count++;
		line++;
	}

	*line = 0;

	return count;
}


//Output driver

int do_tx(struct stty *tty)
{
	int c;
	//printf("tx interrupt\n");
	if (tty->outroom.value == BUFLEN){ // nothing to do 
		//printf("disable tx call\n");
		disable_tx(tty);                 // turn off tx interrupt
		return;
	}

    // write code to output a char from tty's outbuf[ ]
	c = tty->outbuf[tty->outtail++];
	tty->outtail %= BUFLEN;
	out_byte(tty->port, c); //will output c to port
	V(&tty->outroom);
}

int sputc(struct stty *tty, char c)
{
	P(&tty->outroom);
	lock();
	tty->outbuf[tty->outhead++] = c;
	tty->outhead %= BUFLEN;
	unlock();
	if (!tty->tx_on) enable_tx(tty);
	return c;
}

int sputline(struct stty *tty, char *line)
{
	// write code to output a line to tty
	printf("line to put is %s\n", line);

	tty = &stty[0];

	//P(&tty->outmutex);

	while(sputc(tty, *line++)){}

	//V(&tty->outmutex);
}


//**************** Syscalls from Umdoe ************************
int usgets(int port, char *y)
{  
  // get a line from serial port and write it to y in U space
	char *line;
	int i = 0;
	sgetline(stty[0], line);
	while (*line != '\n' && *line != 0 && line != '\r'){
		put_byte(*line++, running->uss, running->usp + y + i);
		i++;
	}
	put_byte(0, running->uss, running->usp + y + i);
}

int uputs(int port, char *y)
{
  // output line y in U space to serail port
}