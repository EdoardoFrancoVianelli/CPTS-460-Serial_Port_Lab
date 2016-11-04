struct semaphore{
    int value;
    PROC *queue;
};

SIGNAL(struct semaphore *s)
{
	PROC *p = dequeue(&s->queue);
	p->status = READY;
	enqueue(&readyQueue, p);
}

block(struct semaphore *s){
		running->status = BLOCK;
		enqueue(&s->queue, running);
		//int_on();
		//tswitch();
		unlock();
		while (running->status == BLOCK){}
}

int P(struct semaphore *s)
{
	// write YOUR C code for P()
	int ps = int_off();
	s->value--;
	if (s->value < 0){
		block(s);
	}
	int_on(ps);
}

int V(struct semaphore *s)
{
	// write YOUR C code for V()
	PROC *p;
	int ps = int_off();
	s->value++;
	if (s->value <= 0){
		SIGNAL(s);
	}
	int_on(ps);
}