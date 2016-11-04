typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

#define NULL      0
#define NPROC     9
#define SSIZE   512

#define FREE      0    /* PROC status */
#define READY     1 
#define SLEEP     2
#define ZOMBIE    3
#define BLOCK     4

/**************** CONSTANTS ***********************/
#define INT_CTL     0x20
#define ENABLE      0x20

#define NULLCHAR      0
#define BEEP          7
#define BACKSPACE     8
#define ESC          27
#define SPACE        32

#define BUFLEN      64
#define NULLCHAR     0

#define NR_STTY      2    /* number of serial ports */

/* offset from serial ports base */
#define DATA         0   /* Data reg for Rx, Tx   */
#define DIVL         0   /* When used as divisor  */
#define DIVH         1   /* to generate baud rate */
#define IER          1   /* Interrupt Enable reg  */
#define IIR          2   /* Interrupt ID rer      */
#define LCR          3   /* Line Control reg      */
#define MCR          4   /* Modem Control reg     */
#define LSR          5   /* Line Status reg       */
#define MSR          6   /* Modem Status reg      */

typedef struct proc{
        struct proc *next;
        int    *ksp; 
        int    uss,usp;
        int    inkmode;

        int     pid;
        int     ppid;
  struct proc *parent;
        int     status;
        int     pri;          
        char    name[16];    
        int     event;
        int     exitCode;
        int     size;        
  int kstack[SSIZE];
}PROC;        