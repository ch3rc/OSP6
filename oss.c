//====================================================================
//Date:		May 4,2020
//Author:	Cody Hawkins
//Class:	CS4760
//Project	assignment 6
//File:		oss.c
//====================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <semaphore.h>
#include "share.h"

int help_flag;
int run_flag;

pid_t childPid;
int exitpid = 0;
int status = 0;
int launched = 0;
int dead = 0;
FILE *output;

int main(int argc, char *argv[])
{

	signal(SIGINT, &killAll);
	signal(SIGALRM, &timesUp);
	
	initFlags();
	
	setopts(argc, argv);

	if(help_flag)
	{
		help();
	}

	output = fopen("log.dat", "a");
	if(output == NULL)
	{
		perror("ERROR: OSS: file\n");
		exit(1);
	}

	initMem();
	message();
	struct MSG msg;
	initClock(clock);
	initPcb(pcb);
	initFrame(frame);


	struct Clock newProc;
	newProc.seconds = 0;
	newProc.nano = (rand() % 500000000) + 1;
	
	struct Clock layout;
	layout.seconds = 1;
	layout.nano = 0;

	int i = 0;
	
	alarm(20);

	

	while(1)
	{
		//fprintf(stderr, "seconds = %d, nanos = %d\n", clock->seconds,clock->nano);
		if(compare(clock, &newProc) == 1 && launched < MAX_KIDS)
		{
			int seat = findSpot(pcb);
			if(seat > -1)
			{

				childPid = fork();

				if(childPid < 0)
				{
					perror("ERROR: fork\n");
					clearMsg();
					cleanAll();
					exit(1);
				}

				if(childPid == 0)
				{
					char str[20];
					snprintf(str, sizeof(str), "%d", seat);
					execlp("./user", str, NULL);
				}
				pcb->rpid[seat] = childPid;

				launched++;

				pcb->pid[seat] = seat + 1;
				
				newProc.seconds += clock->seconds;
				newProc.nano += clock->nano;
				unsigned int spawn = (rand() % 5000000) + 1;
				tickClock(&newProc, spawn);
						
				
			}
		}


		if(msgrcv(toOSS, &msg, sizeof(msg), pcb->rpid[i], IPC_NOWAIT) > 0)
		{
			if(strcmp(msg.msg, "WRITE") == 0)
			{
				msgrcv(toOSS, &msg, sizeof(msg), pcb->rpid[i], 0);
				int pt = atoi(msg.msg);
				secondChance(frame, clock, pt, 1, i);	
			}

			if(strcmp(msg.msg, "READ") == 0)
			{
				msgrcv(toOSS, &msg, sizeof(msg), pcb->rpid[i], 0);
				int pt = atoi(msg.msg);
				secondChance(frame, clock, pt, 0, i);
			}

			if(strcmp(msg.msg, "TERM") == 0)
			{
				if((exitpid = waitpid(pcb->rpid[i], &status, NULL)) > -1)
				{
					if(WIFEXITED(status))
					{
						fprintf(output, "Child %d has exited\n", i);
						pcb->rpid[i] = -1;
						pcb->pid[i] = -1;
						dead++;
						launched--;
					}
				}
			}	
		}

		
		if(compare(clock, &layout) == 1)
		{
			memLayout();
			
			layout.seconds += clock->seconds + 1;
			layout.nano += clock->nano;
		}
		
		sem_wait(sem);
		tickClock(clock, 10000000);
		sem_post(sem);

		i++;

		if(i == 17)
		{
			i = 0;
		}

		if(dead == 99)
		{
			break;
		}

	}
	fclose(output);
	clearMsg();
	cleanAll();
	exit(0);
}

//========================================================
//initial arguments
//========================================================
void initFlags()
{
	help_flag = 0;
	run_flag = 0;
}

void setopts(int argc, char *argv[])
{
	int c;

	while((c = getopt(argc, argv, "hm:")) != -1)
	{
		switch(c)
		{
			case 'h':
				help_flag = 1;
				break;

			case 'm':
				run_flag = atoi(optarg);
				break;
			case '?':
				fprintf(stderr, "unknown option %s\n", optarg);
				exit(1);
		}
	}
}


void help()
{
	fprintf(stderr, "\t\tHELP MESSAGE\n\n");
	fprintf(stderr, "-h = %d for help\n", help_flag);
	fprintf(stderr, "-m = %d choose zero or one for page frame schemes, defaulted to zero\n", run_flag);
}
//========================================================

