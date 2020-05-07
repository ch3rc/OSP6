//===========================================================
//Date:		May 4,2020
//Author:	Cody Hawkins
//Class:	CS4760
//Project:	Assignment 6
//File:		share.h
//==========================================================

#ifndef SHARE_H
#define SHARE_H

#define MAX_KIDS 18

//========================
//memory and table sizes
//========================
#define TOTAL_MEM 262144
#define PAGE_MEM 32768
#define PROC_SIZE 1024

#define PT_SIZE 32
#define FT_SIZE 256
//=========================
//memory keys
//=========================
#define CLOCK 0x11111111
#define SEM 0x22222222
#define FT 0x33333333
#define PCB1 0x44444444
#define MSG1 0x55555555
#define MSG2 0x66666666

#define PERM (IPC_CREAT | 0666)

extern const key_t clockKey;
extern const key_t semKey;
extern const key_t ftKey;
extern const key_t pcbKey;

extern int clockID;
extern int semID;
extern int ftID;
extern int pcbID;
extern int toOSS;
extern int toUSR;

extern const size_t clockSize;
extern const size_t semSize;
extern const size_t ftSize;
extern const size_t pcbSize;

//=========================

extern int help_flag;
extern int run_flag;
extern int pageFault;
extern int memAcc;
extern int memSpeed;
extern FILE *output;


struct MSG{

	long mtype;
	char msg[120];
};

struct Clock{
	
	unsigned int seconds;
	unsigned int nano;
};

struct Page_table{

	int table[PT_SIZE];
};

struct Frame{

	int refBit;
	int dirty;
	int pid;
	struct Clock time;
};

struct Frame_table{

	struct Frame table[FT_SIZE];
};

struct PCB{

	int pid[MAX_KIDS];
	pid_t rpid[MAX_KIDS];
	struct Page_table page;
};	

extern struct Clock *clock;
extern sem_t* sem;
extern struct Frame_table *frame;
extern struct PCB *pcb;

//initial setup functions
void initFlags();
void help();
void setopts(int, char **);

//memory functions
sem_t *getSem(const key_t, const size_t, int*);
void *getMem(const key_t, const size_t, int*);
void initMem();
void message();
void clearMsg();
void detach();
void removeMem(int);
void cleanAll();
void initClock(struct Clock*);
void initPcb(struct PCB*);
void initFrame(struct Frame_table*);

//helper functions
int findSpot(struct PCB* pcb);
void tickClock(struct Clock *, unsigned int);
int compare(struct Clock *, struct Clock *);
void secondChance(struct Frame_table*, struct Clock*, int, int, int);
void resetFrame();
void memLayout();

//signal functions
void timesUp(int);
void killAll(int);


#endif
