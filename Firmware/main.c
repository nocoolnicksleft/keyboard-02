

// #device PIC16F877 *=16 adc=8

#include <16F874.h>

#fuses HS,NOWDT,NOPROTECT,NOPUT,NOBROWNOUT,NOLVP


#include <stdlib.h>

#use fast_io(A)
#use fast_io(B)
#use fast_io(C)
#use fast_io(D)
#use fast_io(E)
#use delay(clock=20000000)
//#use rs232(stream=terminal, baud=9600, xmit=PIN_C6, rcv=PIN_C7, ERRORS)

#define TIMER1_50MSEC_OFF_10MHZ 30000
#define TIMER1_50MSEC_10MHZ 34285
#define TIMER1_100MSEC_10MHZ 3035
#define TIMER1_10MSEC_20MHZ 15536
#define TIMER1_1MSEC_20MHZ 60536
#define TIMER1_3MSEC_20MHZ 50536
#define TIMER1_5MSEC_20MHZ 40536

#define INT_PIN PIN_C2
#define CLR_PIN PIN_C0

int1 timeout1msec = 0;
//int8 timer200msec = 40;
//int1 timeout200msec = 0;

//int1 timerstatus = 0;

// CURRENT CONFIGURATION VALUES

/**********************************************************
/ Common Timer
/**********************************************************/

#INT_TIMER1
void timeproc()
{
   set_timer1(get_timer1() + TIMER1_1MSEC_20MHZ); // 10msec at 20MHz

   timeout1msec = 1;
/*
   if (timerstatus) {
    output_low(PIN_E0);
    timerstatus = 0;
   } else {
    output_high(PIN_E0);
    timerstatus = 1;
   }  
*/

/*
   if (!(--timer200msec)) {
    timer200msec = 200;
    timeout200msec = 1;
   }
*/

}



//**********************************************************
// QUEUE HANDLING
//**********************************************************
#define QUEUE_LENGTH 30
char queue[QUEUE_LENGTH];
int8 queue_start = 0;
int8 queue_stop = 0;
int8 popone = 0;
int1 bytesending = 0;

int8 get_queue_length()
{
  if (queue_start == queue_stop) return 0;
  if (queue_start < queue_stop) return  (queue_stop - queue_start);
  else return  (QUEUE_LENGTH-queue_start) + queue_stop;
}

void push(char c)
{
// fprintf(terminal,"push %c qlen: %u qstart: %u qstop: %u \r\n",c,get_queue_length(),queue_start,queue_stop);
 queue[queue_stop] = c;
 if (queue_stop == (QUEUE_LENGTH-1)) {
   if (queue_start>0) {
    queue_stop = 0;
   }
 } else {
  if (queue_stop == (queue_start - 1)) {
  } else {
   queue_stop++;
  }
 } 
 output_high(INT_PIN);
}

char pop()
{
 char c = 0;
 if (queue_start != queue_stop) {
  c = queue[queue_start];
//  fprintf(terminal,"pop %c qstart: %u qstop: %u \r\n",c,queue_start,queue_stop);
  if (queue_start == (QUEUE_LENGTH-1)) queue_start = 0;
  else {
    queue_start++;
  }
 }
 return c;
}


/**********************************************************
/ Serial Input Buffer
/**********************************************************/
/*
#INT_RDA
void serial_isr() {
   BYTE c;
   c = fgetc(terminal);
   if (c == 'p')  {
     popone = 1;
   }
}
*/


#define MBIT_MAKE  0b00010000
#define MBIT_BREAK 0b00100000

#define MBIT_LEFT  0b00000100
#define MBIT_RIGHT 0b00001000

#define MBIT_ID_KEYBOARD   0b00000000
#define MBIT_ID_ROTARY     0b01000000
#define MBIT_ID_ROTARYPUSH 0b10000000
#define MBIT_ID_LED 	   0b11000000

//**********************************************************
// 4x4 MATRIX KEYBOARD HANDLING
//**********************************************************
#byte KBPORT = 0x8

#define KCOL0 (1 << 4)
#define KCOL1 (1 << 5)
#define KCOL2 (1 << 6)
#define KCOL3 (1 << 7)

#define KROW0 (1 << 0)
#define KROW1 (1 << 1)
#define KROW2 (1 << 2)
#define KROW3 (1 << 3)

#define KALL_ROWS (KROW0|KROW1|KROW2|KROW3)
#define KALL_PINS (KALL_ROWS|KCOL0|KCOL1|KCOL2|KCOL3)

#define kset_tris(x) set_tris_d(x)

char const keycodes[16] = {1,4,7,10,2,5,8,0,3,6,9,11,12,13,14,15};

void dokeyboard()
{

	static int16 allkeys1 = 0xFFFF;
	int16 allkeys;
    int16 deltakeys;
    int16 i;

	kset_tris(KALL_PINS&~KCOL0);
    KBPORT=~KCOL0&KALL_PINS;
    delay_cycles(10);
    allkeys = (int16)(KBPORT & KALL_ROWS);

	kset_tris(KALL_PINS&~KCOL1);
    KBPORT=~KCOL1&KALL_PINS;
    delay_cycles(10);
    allkeys |= (int16)((KBPORT & KALL_ROWS) << 4);

	kset_tris(KALL_PINS&~KCOL2);
    KBPORT=~KCOL2&KALL_PINS;
    delay_cycles(10);
    *(&allkeys+1) = (KBPORT & KALL_ROWS);

	kset_tris(KALL_PINS&~KCOL3);
    KBPORT=~KCOL3&KALL_PINS;
    delay_cycles(10);
    *(&allkeys+1) = *(&allkeys+1) | ((KBPORT & KALL_ROWS) << 4);

    if (allkeys != allkeys1) {
     deltakeys = (allkeys ^ allkeys1);
     for (i=0;i<16;i++) {
      if (deltakeys & ( 1 << i)) {
        if (allkeys & ( 1 << i)) push(MBIT_ID_KEYBOARD | MBIT_BREAK | keycodes[i]);
        else push(MBIT_ID_KEYBOARD | MBIT_MAKE | keycodes[i]);
      }
     }
     //fprintf(terminal,"kstat: %X %X\r\n",*(&allkeys+1),allkeys);
     allkeys1 = allkeys;
    }
    

}

//**********************************************************
// 1x8 STATIC KEYBOARD HANDLING (ROTARY SWITCH PUSH)
//**********************************************************
void dorotarypush()
{
  static int8 keystate1 = 0xFF;
  int8 keystate;
  int8 keydelta;
  int8 i;

  keystate = input_e();
  if (keystate != keystate1) 
  {
    keydelta = (keystate ^ keystate1);
    for (i=0;i<3;i++) {
     if (keydelta & ( 1 << i)) {
       if (keystate & (1 << i)) push (MBIT_ID_ROTARYPUSH | MBIT_BREAK | i );
       else push (MBIT_ID_ROTARYPUSH | MBIT_MAKE | i );
     }
    }    
    keystate1 = keystate;
  }
  
}

//**********************************************************
// ROTARY SWITCH HANDLING
//**********************************************************
void dorotary(int8 id)
{
     int8 yy;
     int8 abn;
     static int8 abn1[3];
   
     if (id > 1) abn = (input_c() & 0b00000011);  // rotary 2 liegt auf port c
     else abn = (input_a() & (0b00000011 << (id*2)) ) >> (id*2); // die anderen auf port a

     // der folgende algorithmus stammt aus dem datenblatt -
     // der zweite zweig mit den alternativen kombinationen wurde mit absicht weggelassen,
     // sonst werden die umschaltungen doppelt gezählt!!!
     if (abn != abn1[id]) {
      yy = (abn ^ abn1[id]);
      if ((abn & 0b00000001) == ((abn & 0b00000010) >> 1)) {
        if (yy == 0b00000001) push(MBIT_ID_ROTARY | MBIT_MAKE | MBIT_LEFT | id);
        else if (yy == 0b00000010) push(MBIT_ID_ROTARY | MBIT_MAKE | MBIT_RIGHT | id);
      } 
      abn1[id] = abn;
     }
}


/**********************************************************
/ LED LOOP
/**********************************************************/

int8 ledstate = 0x00;

void doled()
{
    // ACHTUNG: DIE LEDs LIEGEN AUF PORT B - WENN DER DEBUGGER EINGESETZT WIRD
    // MUSS DAS OUTPUT-KOMMANDO ABGESCHALTET WERDEN, SONST KOMMEN SICH BEIDE INS GEHEGE!!

    // da für die existierende hardware nur 4 leds im einsatz sind wurde
    // das multiplexing nicht implementiert. bei mehr leds sind die folgenden
    // zeilen wieder einzubauen und müssen noch gedebugged werden!!
/*
    static int8 ledcol = 3;
    int8 ledcoldata;
    ledcoldata = (ledstate >> ((ledcol*4)) << 4) ;
    output_b( (1 << ledcol) | (~ledcoldata));
    if (ledcol == 3) ledcol = 0;
    else ledcol++;
*/

   // dies ist die einzelvariante für col 3, wenn obiges implementiert wird muss sie raus!!
   output_b( (1 << 3) | (~ledstate  << 4));

}

#byte   SSPBUF = 0x13
#byte   SSPCON = 0x14
#byte   SSPSTAT = 0x94
/* Now the SSP handler code. Using my own, since the supplied routines test the wrong way round for my needs */
#DEFINE   READ_SSP()   (SSPBUF)
#DEFINE   WAIT_FOR_SSP()   while((SSPSTAT & 1)==0)
#DEFINE   WRITE_SSP(x)   SSPBUF=(x)
#DEFINE   CLEAR_WCOL()   SSPCON=SSPCON & 0x3F 

/**********************************************************
/ SPI Communication (SLAVE)
/**********************************************************/
#INT_SSP
void ssp_isr()
{
  int8 recvd;


  BIT_CLEAR(SSPCON,7);

  if ((SSPSTAT & 0x1) == 0x1) {}

  recvd = SSPBUF;
  SSPBUF = 0x0;


  if ((recvd & 0b11000000) == 0b11000000) { // LED Message received
   if ((recvd & 0b00100000) == 0b00100000) { // CLR Message
     ledstate &= ~(0x1 << (recvd & 0b00001111));
   } 
   if ((recvd & 0b00010000) == 0b00010000) { // SET Message
     ledstate |=  (0x1 << (recvd & 0b00001111));
   }     
  }

  if (bytesending) {
   popone = 1;
  }

}


/**********************************************************
/ MAIN
/**********************************************************/

void main () {

  setup_adc(ADC_OFF);
  setup_adc_ports(NO_ANALOGS);
  output_a(0x00);
  output_b(0x00);
  output_c(0x00);
  output_d(0x00);
  output_e(0x00);

  set_tris_a(0b11111111); // rotary 1-2 & ssel & clr
  set_tris_b(0b00000000); // led
  set_tris_c(0b11011011); // kommunikation und rotary3
  set_tris_d(0b11110000); // keyboard
  set_tris_e(0b00000111); // rotary-push 1-3


  setup_spi(spi_slave | spi_l_to_h );

//  setup_timer_0(RTCC_INTERNAL | RTCC_DIV_256);
  setup_timer_1(T1_INTERNAL | T1_DIV_BY_1 );    // Start timer 1
  enable_interrupts(INT_TIMER1);
//  enable_interrupts(INT_RDA);
  enable_interrupts(INT_SSP);
  enable_interrupts(global);


  output_low(INT_PIN);



  delay_ms(500); // wait for things to calm

//  fprintf(terminal,"BUZZKEY v.2 READY\r\n");

 /**********************************************************
 / STATE LOOP
 /**********************************************************/
  for (;;) {


    if (popone) {
     popone = 0;
     bytesending = 0;
     pop();
     output_low(INT_PIN);
    }

    if (queue_start != queue_stop) {
      if (!bytesending) {
       WRITE_SSP(queue[queue_start]);
       bytesending = 1;
       output_high(INT_PIN); 
      }
    }
/*
    if (input_state(CLR_PIN)) {
      queue_start = 0;
      queue_stop = 0;
      ledstate = 0;
    }
*/
    doled();

    // ***********************************
    // alle 1 ms 
    // ***********************************
    if (timeout1msec) {
     timeout1msec = 0;


     dorotary(0);
     dorotary(1);
     dorotary(2);
     dorotarypush();
 	 dokeyboard();

    } // if (timeout1msec)


  } // for (;;)

} // main()
