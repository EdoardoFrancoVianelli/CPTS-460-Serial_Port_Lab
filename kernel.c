/***********************************************************
  kfork() creates a child proc and returns the child pid.
  When scheduled to run, the child process resumes to body();
************************************************************/

/*
(3).1    SETUP new PROC's ustack for it to return to Umode to execute filename;
(3.2).   Set new PROC.uss = segment;
PROC.usp = TOP of ustack (per INT 80)

new PROC
------
|uss | = new PROC's SEGMENT value
|usp | = -24
--|---                                    Assume P1 in segment=0x2000:
|                                              0x30000
|                                              HIGH END of segment
(LOW) | |   ----- by SAVE in int80h ----- | by INT 80  |
--------|-------------------------------------------------------------
|uDS|uES| di| si| bp| dx| cx| bx| ax|uPC|uCS|flag|NEXT segment
----------------------------------------------------------------------
-12 -11 -10  -9  -8  -7  -6  -5  -4  -3  -2  -1 |
*/

PROC *kfork(char *filename) // create a child process, begin from body()
{
	int i;
	PROC *p = get_proc(&freeList);
	u16 segment;

	if (!p) {
		printf("no more PROC, kfork() failed\n");
		return 0;
	}
	p->status = READY;
	p->pri = 1; // priority = 1 for all proc except P0
	p->ppid = running->pid; // parent = running

	p->inkmode = 1;

							/* initialize new proc's kstack[ ], MUST BE 12 SINCE 2 NEW ENTRIES */
	for (i = 1; i < 10; i++)
	{ // saved CPU registers
		p->kstack[SSIZE - i] = 0; // all 0's
	}

	p->kstack[SSIZE - 1] = (int)body; // resume point=address of body()
	p->ksp = &p->kstack[SSIZE - 9]; // proc saved sp
	enqueue(&readyQueue, p); // enter p into readyQueue by priority
	segment = (p->pid + 1) * 0x1000; // Create the user mode for this process!

	if (filename)
	{
		load(filename, segment);

		put_word(0x0200, segment, -2); //flag is two bytes off the top
		put_word(segment, segment, -4); // UPC is set to the segment

		for (i = -6; i >= -20; i -= 2){ //set all CPU registers to 0
			put_word(0, segment, i);
		}

		put_word(segment, segment, -22); //set ues to the current segment
		put_word(segment, segment, -24); //set uds to the current segment

		p->uss = segment; //set uss to the segment
		p->usp = -24;     //set the offset to -24, represents uds
	}

	return p; // return child PROC pointer
}

int do_tswitch()
{
	printf("proc is %d before the tswitch()\n", running->pid);
	tswitch();
	printf("proc is %d after the switch\n", running->pid);
}

int do_kfork()
{
	PROC *p;
	printf("proc %d is kforking a child ", running->pid); 
	if ((p = kfork("/bin/u1")) == 0){
		printf("kfork failed\n");
	}
	else{
		printf("child pid = %d\n", p->pid);
	}
	return p;
}

int do_exit(int exitValue)
{
	int exitValue;
	if (running->pid == 1 && nproc > 2){
		printf("other procs still exist, P1 can't die yet !\n");
		return -1;
	}
	
	printf("enter an exitValue between 0 and 9: ");
	exitValue = getc() - '0'; 
	printf("entered exit value = %d\n", exitValue);
	kexit(exitValue);
}

int do_wait(int *ustatus)
{
	int child;
	int status;
	int segment = running->uss;
	if ((child = kwait(&status)) < 0){
		printf("there is no child, could not wait\n", running->pid);
		return -1;
	}
	printf("proc %d found a ZOMBIE child %d with an exitValue=%d\n", running->pid, child, status);
	//put the word status at segment segment with an offset of ustatus
	//since ustatus is a pointer
	put_word(status, segment, ustatus);// write status to Umode *ustatus
	//so that the kernel can retrive it
	return child;
}

int printList(PROC *p){
	int i=0;
	while (p){
		if (i > 0)
			printf(" -> ");
		printf("%d", p->pid);
		p = p->next;
		i++;
	}
	printf("\n");
}

int body()
{
	char c;
	printf("proc %d resumes to body()\n", running->pid);
	while(1){
		printf("-----------------------------------------\n");
		printList("freelist  ", freeList);
		printList("readyQueue", readyQueue);
		printList("sleepList ", sleepList);
		printf("-----------------------------------------\n");

		printf("proc %d running: parent = %d  enter a char [s|f|w|q|u] : ", 
		   running->pid, running->parent->pid);
		c = getc(); printf("%c\n", c);
		switch(c){
			case 's':
				printf("about to enter tswitch()\n");
				do_tswitch();  
				break;
			case 'f': 
				printf("about to enter do_kfork()\n");
				do_kfork();
				break;
			case 'w': 
				printf("about to enter do_wait\n");
				do_wait();
			        break;
			case 'q': 
				printf("about to enter do_exit\n");
				do_exit();
				break;
			case 'u': 
				printf("about to enter umode\n");
				goUmode();
				break;
			default:
				printf("unrecognized character\n");
				break;
		}
	}
}

extern PROC proc[];

int kmode()
{
	body();
}

int timer(int n){
	printf("About to sleep for %d seconds\n", n);
}

int do_ps() //implement do_ps
{
	int i = 0, k = 0, j, status_i = 15, pid_i = 27, tot = 0;
	char *start;
	PROC *current = 0;
	char *status_descr = "FREE   READY   RUNNINGSTOPPEDSLEEP  ZOMBIE ";
	printf("============================================\n"); // 43
	printf("  name         status      pid       ppid   \n");
	printf("--------------------------------------------\n");
	for (i = 0; i < 9; i++){
		current = &proc[i];
		start = current->name;
		while (k < 32 && *start != 0){
			putc(*start);
			start++;
			k++;
		}
		/*
			#define FREE     0
			#define READY    1
			#define RUNNING  2
			#define STOPPED  3
			#define SLEEP    4
			#define ZOMBIE   5
		*/
		
		for (j = status_i - k; j >= 0; j--){ putc(' '); }
	
		
		for (k = proc->status * 7; k <= (proc->status * 7) + 7; k++){
			putc(status_descr[k]);
			tot++;
		}
		
		for (j = pid_i - status_i - tot; j >= 0; j--){ putc(' '); }
		
		printf("%d", current->pid);
		
		for (j = 7; j >= 0; j--){ putc(' '); }
		
		printf("%d", current->ppid);
		
		printf("\n");
		k = 0;
		tot = 0;
	}
}

int copyImage(u16 pseg, u16 cseg, u16 size)
{
	u16 i;
	for (i = 0; i < size; i++){
		put_word(get_word(pseg, 2*i), cseg, 2 * i);
	}
}

int fork()
{
	int pid;
	u16 segment;
	PROC *p = kfork(0); //kfork a child, do not load image file
	if (p == 0) return -1;  //kfork failed	
	segment = (p->pid + 1) * 0x1000; //child segment
	copyImage(running->uss, segment, 32 * 1024); //copy 32k words
	p->uss = segment; //child's own segment
	p->usp = running->usp; //same as parent's usp
	//*** change uDS, uES, uCS, AX in child's ustack ****
	put_word(segment, segment, p->usp);          //uDS=segment
	put_word(segment, segment, p->usp + 2);      //uES=segment
	put_word(0,       segment, p->usp + 2 * 8);  //uAX=0
	put_word(segment, segment, p->usp + 2 * 10); //uCS=segment		
	return p->pid;
}

int kexec(char *y)
{
	int i, length = 0, saved_i = 0, offset = 0;
	char filename[64], *cp = filename;
	char cmd[64];
	char prev,temp;
	u16 segment = running->uss; // same segment
	/* get filename from U space with a length limit of 64 */
	
	printf("About to load filename\n");
	getc();
	
	while ( (*cp++ = get_byte(running->uss, y++)) && length++ < 64)
		continue;	
	
	filename[length] = 0;
	printf("Length of filename is %d, filename is [%s]\n", length, filename);
	getc();
		
	for (i = 0; i < 64 && filename[i] != ' '; i++){ cmd[i] = filename[i]; }
	
	cmd[i] = 0;
	
	for (i = 0; i < length; i++){	
		printf("Putting %c|%don stack\n", filename[i], filename[i]);
		put_byte(filename[i], segment, (length - i) * -1);			
	}
	
	//put an extra byte if the string length is odd
	if (length % 2 != 0){
		length++;
		put_byte(7, segment, length * -1);
	}
	
	//add the pointer
	
	put_word(-1 * length, segment, (length + 2) * -1);
	
	if (!load(cmd, segment)) // load filename to segment
		return -1;
		
	/* re-initialize process ustack for it to return VA=0 */
	printf("After load\n");
	offset = -2 - (length);
	for (i = 1; i <= 12; i++)
		put_word(0, segment, (-2 * i) + offset);
	running->usp = -24 - (2 + length); //new usp = -24
	// -1	-2	-3	-4	-5	-6	-7	-8	-9	-10	-11	-12
	//flag	uCS	uPC 	ax	bx	cx	dx	bp	si	di	uES	uDS
	put_word(segment, segment, -(2 * 12) + offset); //saved uDS        = segment
	put_word(segment, segment, -(2 * 11) + offset); //saved uES        = segment
	put_word(segment, segment, -(2 *  2) + offset); //uCS=segment, uPC = 0
	put_word(0x0200,  segment, -(2 *  1) + offset); //set umode flag   = 0x0200
	printf("about to go to umode\n");
	getc();
}

int chname(char * y) //implement chname
{
	//const int max_len = 32;
	//change running name to *y
	
	int i = 8;
	char b;
	char buffer[32];
	
	printf("y is %d\n", y);
	
	while (i < 32){
		b = get_byte(running->uss, y + i);
		i++;
	}
	buffer[i] = 0;
	strcpy(running->name, buffer);
}

int kkfork()
{
	PROC *p = kfork("/bin/u1");
	if (p==0){
		return -1;
	}
	return p->pid;
}
