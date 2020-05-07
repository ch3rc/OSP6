//=====================================================================
//Date:		May 4,2020
//Author:	Cody Hawkins
//Project:	Assignment 6
//Class:	CS4760
//File:		shared.c
//=====================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <semaphore.h>
#include "share.h"

const key_t clockKey = CLOCK;
const key_t semKey = SEM;
const key_t ftKey = FT;
const key_t pcbKey = PCB1;
key_t msgTo;
key_t msgFrm;

int clockID = 0;
int semID = 0;
int ftID = 0;
int pcbID = 0;
int toOSS;
int toUSR;

const size_t clockSize = sizeof(struct Clock);
const size_t semSize = sizeof(sem_t);
const size_t ftSize = sizeof(struct Frame_table);
const size_t pcbSize = sizeof(struct PCB);

struct Clock *clock = NULL;
sem_t *sem = NULL;
struct Frame_table *frame = NULL;
struct PCB *pcb = NULL;

int pageFault = 0;
int memAcc = 0;
int memSpeed = 0;
FILE *output;

sem_t *getSem(const key_t key, const size_t size, int *shmid)
{
	*shmid = shmget(key, size, PERM);
	if(*shmid < 0)
	{
		perror("ERROR: shmget failed (sem_t)\n");
		cleanAll();
		exit(1);
	}

	sem_t *temp = shmat(*shmid, NULL, 0);
	if(temp == (sem_t*)-1)
	{
		perror("ERROR: shmat failed (sem_t)\n");
		cleanAll();
		exit(1);
	}

	if(sem_init(temp, 1, 1) == -1)
	{
		perror("ERROR: sem_init failed\n");
		cleanAll();
		exit(1);
	}

	return temp;
}

void *getMem(const key_t key, const size_t size, int* shmid)
{
	*shmid = shmget(key, size, PERM);
	if(*shmid < 0)
	{
		perror("ERROR: shmget failed (getMem)\n");
		cleanAll();
		exit(1);
	}

	void *temp = shmat(*shmid, NULL, 0);
	if(temp == (void*)-1)
	{
		perror("ERROR: shmat failed (getMem)\n");
		cleanAll();
		exit(1);
	}
		
	return temp;
}

void message()
{
	msgTo = MSG1;

	toUSR = msgget(msgTo, PERM);
	if(toUSR == -1)
	{
		perror("ERROR: msgget(toUSR)\n");
		clearMsg();
		exit(1);
	}

	msgFrm = MSG2;
	
	toOSS = msgget(msgFrm, PERM);
	if(toOSS == -1)
	{
		perror("ERROR: msgget(toOSS)\n");
		clearMsg();
		exit(1);
	}
}

void clearMsg()
{
	msgctl(toUSR, IPC_RMID, NULL);
	msgctl(toOSS, IPC_RMID, NULL);
}

void detach()
{
	shmdt(clock);
	shmdt(sem);
	shmdt(frame);
	shmdt(pcb);
}

void removeMem(int shmid)
{
	int temp = shmctl(shmid, IPC_RMID, NULL);
	if(temp == -1)
	{
		perror("OSS: shmctl failed\n");
		exit(1);
	}
}

void cleanAll()
{
	if(clockID > 0)
		removeMem(clockID);
	if(semID > 0)
		removeMem(semID);
	if(ftID > 0)
		removeMem(ftID);
	if(pcbID > 0)
		removeMem(pcbID);
}

void initClock(struct Clock* time)
{
	time->seconds = 0;
	time->nano = 0;
}

void initPcb(struct PCB* pcb)
{
	int i;
	for(i = 0; i < MAX_KIDS; i++)
	{
		pcb->pid[i] = -1;
		pcb->rpid[i] = -1;
	}

	for(i = 0; i < PT_SIZE; i++)
	{
		pcb->page.table[i] = -1;
	}
}

void initFrame(struct Frame_table *frame)
{
	int i;
	for(i = 0; i < FT_SIZE; i++)
	{
		frame->table[i].refBit = 0;
		frame->table[i].dirty = -1;
		frame->table[i].pid = -1;
		frame->table[i].time.seconds = 0;
		frame->table[i].time.nano = 0;
	}
}	

void initMem()
{
	clock = (struct Clock*)getMem(clockKey, clockSize, &clockID);
	frame = (struct Frame_table*)getMem(ftKey, ftSize, &ftID);
	pcb = (struct PCB*)getMem(pcbKey, pcbSize, &pcbID);
	sem = getSem(semKey, semSize, &semID);
}


int findSpot(struct PCB* pcb)
{
	int i;
	for(i = 0; i < MAX_KIDS; i++)
	{
		if(pcb->rpid[i] == -1)
		{
			return i;
		}
	}

	return -1;
}

void tickClock(struct Clock *clock, unsigned int nanos)
{
	unsigned int nano = clock->nano + nanos;

	while(nano >= 1000000000)
	{
		nano -= 1000000000;
		clock->seconds++;
	}
	clock->nano = nano;
}

int compare(struct Clock *time, struct Clock *newtime)
{
	long time1 = (long)time->seconds * 1000000000 + (long)time->nano;
	long time2 =  (long)newtime->seconds * 1000000000 + (long)newtime->nano;

	if(time1 > time2)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


void timesUp(int sig)
{
	char msg[] = "\nProgram has reached 20 seconds\n";
	int msgSize = sizeof(msg);
	write(STDERR_FILENO, msg, msgSize);

	int i;
	for(i = 0; i < MAX_KIDS; i++)
	{
		if(pcb->rpid[i] != -1)
		{
			if(kill(pcb->rpid[i], SIGTERM) == -1)
			{
				perror("ERROR: OSS: SIGTERM(timesUp)\n");
			}
		}
	}
	fprintf(output, "Page faults = %d\n", pageFault);
	memAcc = (int)(clock->seconds / memAcc);
	fprintf(output, "Number of accesses per second %d\n", memAcc);
	fclose(output);
	clearMsg();
	cleanAll();
	exit(0);
}

void killAll(int sig)
{
	char msg[] = "\nCaught CTRL+C\n";
	int msgSize = sizeof(msg);
	write(STDERR_FILENO, msg, msgSize);

	int i;
	for(i = 0; i < MAX_KIDS;i++)
	{
		if(pcb->rpid[i] != -1)
		{
			if(kill(pcb->rpid[i], SIGTERM) == -1)
			{
				perror("ERROR: OSS: SIGTERM(killALL)\n");
			}
		}
	}
	
	clearMsg();
	cleanAll();
	exit(0);
}

void resetFrame()
{
	int i;
	for(i = 0; i < FT_SIZE; i++)
	{
		frame->table[i].refBit = -1;
		frame->table[i].dirty = -1;
		frame->table[i].time.seconds = 0;
		frame->table[i].time.nano = 0;
		frame->table[i].pid = -1;
	}
}

void secondChance(struct Frame_table *f, struct Clock *time, int page, int dirty, int pos)
{
	struct MSG msg;

	int i, count = 0;
	for(i = 0; i < FT_SIZE; i++)
	{
		if(f->table[i].pid != -1)
		{
			count++;
		}
		if(count == 255)
		{
			resetFrame();
		}
	}

	if(dirty == 0)
	{
		fprintf(output, "MASTER: P%d requesting read of address %d at time %d:%d\n", 
			pos, page, time->seconds, time->nano);

		int newPage = (int)(page/PROC_SIZE);
		if(newPage == 0)
			newPage += 1;

		int frame = (int)(FT_SIZE / newPage) - 1;

		fprintf(output, "MASTER: Address %d in frame %d, giving data to P%d at time %d:%d\n",
			page, frame, pos, time->seconds, time->nano);
		
		if(f->table[frame].pid != -1 && f->table[frame].pid == pcb->rpid[pos] && f->table[frame].refBit != 0)
		{
			f->table[frame].refBit = 0;
			memAcc++;
		}
		else if(f->table[frame].pid == -1)
		{
			f->table[frame].refBit = 1;
			f->table[frame].dirty = 0;
			f->table[frame].time.seconds = time->seconds;
			f->table[frame].time.nano = time->seconds;
			pageFault++;
			memAcc++;
			f->table[frame].pid = pcb->rpid[pos];
			sem_wait(sem);
			tickClock(time, 10);
			sem_post(sem);
		}
		else
		{
			fprintf(output, "MASTER: Clearing frame %d and swapping in P%d page %d\n", frame, pos, newPage);
			f->table[frame].refBit = 1;
			f->table[frame].dirty = 0;
			f->table[frame].time.seconds = time->seconds;
			f->table[frame].time.nano = time->seconds;
			f->table[frame].pid = pcb->rpid[pos];
			pageFault++;
			memAcc++;
			sem_wait(sem);
			tickClock(time,10);
			sem_post(sem);
		}

	}

	else if(dirty == 1)
	{
		fprintf(output, "MASTER: P%d requesting write of address %d at time %d:%d\n",
			pos, page, time->seconds, time->nano);

		int newP = (int)(page/PROC_SIZE);
		if(newP == 0)
			newP += 1;
		int newF = (int)(FT_SIZE / newP) - 1;
		
		fprintf(output, "MASTER: Address %d in frame %d, writing data to frame at time %d:%d\n",
			page, newF, time->seconds, time->nano);

		if(f->table[newF].pid != -1 && f->table[newF].pid == pcb->rpid[pos] && f->table[newF].refBit != 0)
		{
			f->table[newF].refBit = 0;
			memAcc++;
		}
		else if(f->table[newF].pid == -1)
		{
			f->table[newF].refBit = 1;
			f->table[newF].dirty = 1;
			f->table[newF].time.seconds = time->seconds;
			f->table[newF].time.nano = time->nano;
			pageFault++;
			memAcc++;
			f->table[newF].pid = pcb->rpid[pos];

			fprintf(output, "MASTER: Dirty bit of frame %d set, adding additional time to the clock\n",
				newF);
			sem_wait(sem);
			tickClock(time, 1400000);
			sem_post(sem);		

			msg.mtype = pcb->rpid[pos];
			strcpy(msg.msg, "GRANTED");
			msgsnd(toUSR, &msg, sizeof(msg), IPC_NOWAIT);

			fprintf(output, "MASTER: Indicating to P%d that write has happened to address %d\n",
				pos, page); 
	 	}
		else
		{
			fprintf(output, "MASTER: Clearing frame %d and swapping in P%d page %d\n", newF, pos, newP);

			f->table[newF].refBit = 1;
			f->table[newF].dirty = 1;
			f->table[newF].time.seconds = time->seconds;
			f->table[newF].time.nano = time->nano;
			f->table[newF].pid = pcb->rpid[pos];
			pageFault++;
			memAcc++;
			fprintf(output, "MASTER: Dirty bit of frame %d set, adding additional time to the clock\n",
				newF);
			
			sem_wait(sem);
			tickClock(time, 1400000);
			sem_post(sem);

			msg.mtype = pcb->rpid[pos];
			strcmp(msg.msg, "GRANTED");
			msgsnd(toUSR, &msg, sizeof(msg), IPC_NOWAIT);
		}
	}
}

void memLayout()
{
	fprintf(output, "Current Memory layout at time %d:%d is:\n", clock->seconds, clock->nano);

	int i;
	for(i = 0; i < FT_SIZE; i++)
	{
		if(frame->table[i].pid != -1)
		{
			fprintf(output, "+");
		}
		if(frame->table[i].pid == -1)
		{
			fprintf(output, ".");
		}
		if(i % 50 == 0)
		{
			fprintf(output, "\n");
		}
	}
}			
