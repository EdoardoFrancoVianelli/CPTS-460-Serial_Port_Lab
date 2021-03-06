


int ksleep(int event)
{
	//int_off;
	running->status = SLEEP;
	running->event = event;
	// enter sleepList FIFO
	enqueue(&sleepList, running);
	tswitch();
	//int_on;
}
  
int ready(PROC *p)
{
	//int_off;
	p->event = 0;
	p->status = READY;
	enqueue(&readyQueue, p);
	printf("wakeup proc %d\n", p->pid);
	//int_on;
}

/* wake up ALL procs sleeping on event */

int kwakeup(int event)
{
	PROC *p, *q = 0;

	while(p = dequeue(&sleepList)){
		if (p->event == event){
			p->status = READY;
			enqueue(&readyQueue, p);
			printf("kwaeup: wakeup %d\n", p->pid);
			continue;
		}
		enqueue(&q, p);
	}
	sleepList = q;
}

/**********************
int kwakeup(int event)
{
  PROC *p, *q; int i;
  p = q = sleepList;
  while(p){
    if (p->event != event){
      q = p;
      p = p->next;
      continue;
    }
    // p->event==event
    if (p==sleepList){
	 sleepList = p->next;
         ready(p);
         p = q = sleepList;
         continue;
    }
    // not the first element   
    q->next = p->next;   // delete p from list
    ready(p);
    p = q->next;
  }          
}
*************************/
int kexit(int exitValue)
{
	int i, wk1; PROC *p;
	/* send children (dead or alive) to P1's orphanage */
	wk1 = 0;
	for (i = 1; i< NPROC; i++){
		p = &proc[i];
		if (p->status != FREE && p->ppid == running->pid){
			p->ppid = 1;
			p->parent = &proc[1];
			wk1++;
		}
	}
	/* record exitValue and become a ZOMBIE */
	running->exitCode = exitValue;
	running->status = ZOMBIE;

	/* wakeup parent and P1 */
	kwakeup(running->parent);
	if (wk1) // if sent any child to P1: wake up P1 also
	kwakeup(&proc[1]);
	tswitch();
}

int kwait(int *status)
{
	PROC *p;
	int  i, found = 0;
	while(1){
		//found = 0;
		for (i=0; i<NPROC; i++){ 
			p = &proc[i];
			if (p->ppid == running->pid && p->status != FREE){ 
				found = 1;
				/* lay the dead child to rest */
				if (p->status == ZOMBIE){
					*status = p->exitCode;
					p->status = FREE;       /* free its PROC */
					put_proc(&freeList, p);
					nproc--;
					return(p->pid);         /* return its pid */
				}
			}
		}
		if (!found)                         /* no child */
		return(-1);
		ksleep(running);                     /* has kids still alive */
	}
}
