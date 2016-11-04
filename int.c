/************** syscall routing table ***********/

int ksin(char *y)
{
	
	// get a line from serial port 0; write line to Umode
	struct stty *t;
	char *x, c; 
	int length;
	int i = 0;

	printf("ksin()\n");
	length = sgetline(&stty[0], x);
	printf("length is %d\n", length);

	while (i < length){
		put_byte(x[i], running->uss, y + i);
		i++;
	}
	put_byte(0, running->uss, y+i);
}


int ksout(char *y)
{
	char buffer[BUFLEN];
	char *x = buffer, c;

	printf("ksout()\n");
	
	while ((c = get_byte(running->uss, y++)) != 0){
		*x++ = c;
	}

	*x++ = '\n';
	*x = 0;

	printf("Line from user mode is %s\n", buffer);
	sputline(&stty[0], buffer);//write line to serial port 0	
	//printf("ksout() end\n");
	return 1;
}

int kcinth() 
{
  u16 x, y, z, w, r; 
  u16 seg, off;

  seg = running->uss; off = running->usp;

  x = get_word(seg, off+13*2);
  y = get_word(seg, off+14*2);
  z = get_word(seg, off+15*2);
  w = get_word(seg, off+16*2);
  
   switch(x){
       case 0 : r = running->pid;    break;
       case 1 : r = kps();           break;
       case 2 : r = chname(y);       break;
       case 3 : r = kmode();         break;
       case 4 : r = tswitch();       break;
       case 5 : r = kwait();         break;
       case 6 : r = kexit();         break;
       case 7 : r = fork();          break;
       case 8 : r = kexec(y);        break;

       case 9  : r = ksout(y);       break;
       case 10 : r = ksin(y);        break;

       case 99: r = kexit();         break;

       default: printf("invalid syscall # : %d\n", x);

   }
   put_word(r, seg, off+2*8);
}




