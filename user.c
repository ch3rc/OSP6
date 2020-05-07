//========================================================================
//Date:		May 4,2020
//Author:	Cody Hawkins
//Class:	CS4760
//Project:	Assignment 6
//File:		user.c
//=========================================================================

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

int run_flag;
float numbers[PT_SIZE];

int main(int argc, char *argv[])
{
	srand(time(0) ^ (getpid() << 16));

	int pos = atoi(argv[0]);

	initMem();
	message();
	struct MSG msg;
	
	struct Clock waitTime;
	waitTime.seconds = 0;
	waitTime.nano = 0;

	int count = 0;

	int index;

	for(index = 0; index < PT_SIZE; index++)
	{
		numbers[index] = (float)(1.f / (index + 1));
	}	
	
	for(index = 0; index < PT_SIZE; index++)
	{
		numbers[index + 1] = numbers[index] + numbers[index + 1];
	}
 
	while(1)
	{
		
		if(run_flag)
		{
			int page;
			float number = ((float)rand() / (float)numbers[31]) + numbers[0];
			int i;
			for(i = 0; i < PT_SIZE; i++)
			{
				if(numbers[i] > number)
				{
					page = i;
					break;
				}
			}
			
			page = (page * 1024) + (rand() % 1023);
		
			if((rand() % 100) > 85)
			{
				msg.mtype = pcb->rpid[pos];
				strcpy(msg.msg, "WRITE");
				msgsnd(toOSS, &msg, sizeof(msg), IPC_NOWAIT);
				
				msg.mtype = pcb->rpid[pos];
				snprintf(msg.msg, sizeof(msg), "%d", page);
				msgsnd(toOSS, &msg, sizeof(msg), IPC_NOWAIT);
				waitTime.seconds = clock->seconds + 5;
				waitTime.nano = clock->seconds;
				while(1)
				{
					if(msgrcv(toUSR, &msg, sizeof(msg), pcb->rpid[pos], IPC_NOWAIT) > 0)
					{
						if(strcmp(msg.msg, "GRANTED") == 0)
						{
							break;
						}
					}

					if(compare(clock, &waitTime) == 1)
					{
						break;
					}
				}
			}
		
			else
			{
				msg.mtype = pcb->rpid[pos];
				strcpy(msg.msg, "READ");
				msgsnd(toOSS, &msg, sizeof(msg), IPC_NOWAIT);

				msg.mtype = pcb->rpid[pos];
				snprintf(msg.msg, sizeof(msg), "%d", page);
				msgsnd(toOSS, &msg, sizeof(msg), IPC_NOWAIT);
			}
		}		
				
		if(!run_flag)
		{
			int pageNum = (rand() % PAGE_MEM);
			int pagePos = (int)(pageNum / PROC_SIZE);
			pcb->page.table[pagePos] = pos;
	
			if((rand() % 100) > 85)
			{
				//fprintf(stderr, "child requesting write of address %d\n", pageNum);
				msg.mtype = pcb->rpid[pos];
				strcpy(msg.msg,"WRITE");
				msgsnd(toOSS, &msg, sizeof(msg), IPC_NOWAIT);

				msg.mtype = pcb->rpid[pos];
				snprintf(msg.msg, sizeof(msg), "%d", pageNum);
				msgsnd(toOSS, &msg, sizeof(msg), IPC_NOWAIT);
				waitTime.seconds = clock->seconds + 5;
				waitTime.seconds = clock->nano;
				while(1)
				{
					if(msgrcv(toUSR, &msg, sizeof(msg), pcb->rpid[pos], IPC_NOWAIT) > 0)
					{
						if(strcmp(msg.msg, "GRANTED") == 0)
						{
							//fprintf(stderr, "child %d granted page\n", pos);
							break;
						}
					}

					if(compare(clock, &waitTime) == 1)
					{
						break;
					}
				}
			}
			else
			{
				//fprintf(stderr, "child requesting read of address %d\n", pageNum);
				msg.mtype = pcb->rpid[pos];
				strcpy(msg.msg, "READ");
				msgsnd(toOSS, &msg, sizeof(msg), IPC_NOWAIT);

				msg.mtype = pcb->rpid[pos];
				snprintf(msg.msg, sizeof(msg), "%d", pageNum);
				msgsnd(toOSS, &msg, sizeof(msg), IPC_NOWAIT);
			}
		}

		if(count == 100)	
		{
			if((rand() % 100) > 10)
			{
				//fprintf(stderr, "TEMINATION MESSAGE BEING SENT\n");
				msg.mtype = pcb->rpid[pos];
				strcpy(msg.msg, "TERM");
				msgsnd(toOSS, &msg, sizeof(msg), 0);
				break;
			}
			else
			{
				count = 0;
			}
		}

		sem_wait(sem);
		tickClock(clock, 1000000);
		sem_post(sem);
		count++;
	}	
	
	//fprintf(stderr, "death message from child\n");
	detach();
	exit(12);
}
