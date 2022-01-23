#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include "allocate.h"

//Global variables, these being global eases access by subfunctions
process *processlisthead = NULL;
printstart *printstarthead = NULL;
printfinish *printfinishhead = NULL;
int main(int argc, char* argv[]) {
	char* filename = NULL;
	int processorcount = 1;
	FILE* inputfile;
	//let us handle the command line input

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-f") == 0) 
		{
			filename = argv[i + 1];
			i++;
		}
		if (strcmp(argv[i], "-p") == 0)
		{
			processorcount = atoi(argv[i + 1]);
			i++;
		}
	}

	inputfile = fopen(filename, "r");

	//Processors in array
	processor *processorlist = malloc(processorcount * sizeof(processor));
	//initalise each processor
	for (int i = 0; i < processorcount; i++) 
	{
		processorlist[i].processorid = i;
		processorlist[i].totalruntimeleft = 0;
		processorlist[i].used = false;
		processorlist[i].listhead = NULL;
		processorlist[i].currentlyrunning = NULL;
	}
	

	//Keep a static linked list of processes
	//We'll read all of the input file before starting the simulation
	int timein;
	int p_id;
	int timetorun;
	char canparallelchar[5];
	bool canparallel;
	fscanf(inputfile, "%d %d %d %s", &timein, &p_id, &timetorun, canparallelchar);
	//Convert parallelisation character into boolean
	if (strcmp(canparallelchar, "p") == 0)
	{
		canparallel = true;
	}
	else
	{
		canparallel = false;
	}
	//create first process and set to head
	bool anyparallel = canparallel;
	processlisthead = processinit(p_id, timein, timetorun, canparallel);
	//Variable to check if we have any parallelisable processes at all
	//Create the rest of the processes
	while (true)
	{
		//Check if end of file has been reached, if so, end loop and move to simulation
		//create process and add to end of linked list
		fscanf(inputfile, "%d %d %d %c", &timein, &p_id, &timetorun, canparallelchar);
		if (feof(inputfile) != 0)
		{
			break;
		}
		if (strcmp(canparallelchar, "p") == 0)
		{
			canparallel = true;
			anyparallel = true;
		}
		else
		{
			canparallel = false;
		}
		process* currentprocess = processinit(p_id, timein, timetorun, canparallel);
		recursiveaddprocess(currentprocess, processlisthead);
	}
	//Initalise statholder
	statholder* statholder = statholderinit();
	//Check order of processes
	reorderprocesslist();
	//Start of simulation code
	int time = 0;
	int timeinterval = 1;
	int processesleft = 0;
	bool* processorstarted = malloc(processorcount * sizeof(bool));
	for (int i = 0; i < processorcount; i++)
	{
		processorstarted[i] = false;
	}
	//Use better loop condition when possible!
	if (anyparallel == false)
	{
		while (true)
		{

			//Each processor runs tasks for time interval
			for (int i = 0; i < processorcount; i++)
			{
				if (processorlist[i].totalruntimeleft > 0)
				{
					processorlist[i].totalruntimeleft -= 1;
					processorlist[i].currentlyrunning->remainingruntime -= 1;
					//If done finish subprocess
					if (processorlist[i].currentlyrunning->remainingruntime == 0)
					{
						//If doing parallelisation check if last subprocess
						processorlist[i].currentlyrunning->parentprocess->subprocessesleft -= 1;
						//Add printfinish to queue
						printfinish* finishtoken = printfinishinit(processorlist[i].currentlyrunning->parentprocess);
						addprintfinish(finishtoken);
						processesleft -= 1;
						//Also add stats
						addstats(statholder, processorlist[i].currentlyrunning->parentprocess, time);
						removeprocess(processorlist[i].currentlyrunning->parentprocess);
						//Replacing removesubprocess
						if (processorlist[i].currentlyrunning->prev == NULL)
						{
							processorlist[i].listhead = processorlist[i].currentlyrunning->next;
							//If this wasn't the last subprocess, set next subprocess prev to NULL
							if (processorlist[i].currentlyrunning->next != NULL)
							{
								processorlist[i].currentlyrunning->next->prev = NULL;
							}
						}
						else
						{
							processorlist[i].currentlyrunning->prev->next = processorlist[i].currentlyrunning->next;
							if (processorlist[i].currentlyrunning->next != NULL)
							{
								processorlist[i].currentlyrunning->next->prev = processorlist[i].currentlyrunning->prev;
							}
						}
						free(processorlist[i].currentlyrunning);
						//Look for a new process to run
						processorlist[i].currentlyrunning = NULL;
						processorstarted[i] = true;
					}
				}
			}

			//Each process checks to see if it starts
			process* processcheckstart = processlisthead;
			while (processcheckstart != NULL)
			{
				//Start process
				if (time == processcheckstart->starttime)
				{
					subprocess* subprocessstart = subprocessinit(processcheckstart, -1, processcheckstart->runtime);
					processesleft += 1;
					//This will be helpful later because we can reuse it
					int subprocesstime = subprocessstart->remainingruntime;
					//find processor to put subprocess into and do so
					int targetprocessor = 0;
					for (int i = 0; i < processorcount; i++)
					{
						if (processorlist[i].totalruntimeleft < processorlist[targetprocessor].totalruntimeleft)
						{
							targetprocessor = i;
						}
					}
					//replace subprocessadd
					if (processorlist[targetprocessor].listhead == NULL)
					{
						processorlist[targetprocessor].listhead = subprocessstart;
					}
					else
					{
						recursiveaddsubprocess(subprocessstart, processorlist[targetprocessor].listhead);
					}
					processorlist[targetprocessor].totalruntimeleft += subprocesstime;
					//Now check if subprocess immediately starts
					//Happens if no current subprocess or if faster than current subprocess
					if (processorlist[targetprocessor].currentlyrunning == NULL)
					{
						//Don't immediately set, important
						processorstarted[targetprocessor] = true;
					}
					else if (processorlist[targetprocessor].currentlyrunning->remainingruntime > subprocesstime)
					{
						processorlist[targetprocessor].currentlyrunning = subprocessstart;
						processorstarted[targetprocessor] = true;
					}
					//Case to check if processnum is lower.
					else if (processorlist[targetprocessor].currentlyrunning->remainingruntime == subprocesstime && subprocessstart->processnum < processorlist[targetprocessor].currentlyrunning->processnum)
					{
						processorlist[targetprocessor].currentlyrunning = subprocessstart;
						processorstarted[targetprocessor] = true;
					}

				}
				processcheckstart = processcheckstart->next;
			}
			//Generate printstart token
			for (int i = 0; i < processorcount; i++)
			{
				if (processorstarted[i] == true)
				{
					//If we haven't set the currently running subprocess, do so
					if (processorlist[i].currentlyrunning == NULL && processorlist[i].listhead != NULL)
					{
						subprocess* selected = processorlist[i].listhead;
						subprocess* check = processorlist[i].listhead->next;
						while (check != NULL)
						{
							if (selected->remainingruntime > check->remainingruntime)
							{
								selected = check;
							}
							else if (selected->remainingruntime == check->remainingruntime && selected->processnum > check->processnum)
							{
								selected = check;
							}
							check = check->next;
						}
						processorlist[i].currentlyrunning = selected;
					}
					if (processorlist[i].currentlyrunning != NULL)
					{
						printstart* starttoken = printstartinit(processorlist[i].currentlyrunning, i);
						addprintstart(starttoken);
					}
					processorstarted[i] = false;
				}
			}
			//Print everything
			printall(time, processesleft);
			if (processlisthead == NULL)
			{
				break;
			}
			time += timeinterval;
		}
	}
	else
	{
		while (true)
		{
			//Each processor runs tasks for time interval
			for (int i = 0; i < processorcount; i++)
			{
				if (processorlist[i].totalruntimeleft > 0)
				{
					processorlist[i].totalruntimeleft -= 1;
					processorlist[i].currentlyrunning->remainingruntime -= 1;
					//If done finish subprocess
					if (processorlist[i].currentlyrunning->remainingruntime == 0)
					{
						//Check if last subprocess
						processorlist[i].currentlyrunning->parentprocess->subprocessesleft -= 1;
						if (processorlist[i].currentlyrunning->parentprocess->subprocessesleft == 0)
						{
							//Add printfinish to queue
							printfinish* finishtoken = printfinishinit(processorlist[i].currentlyrunning->parentprocess);
							addprintfinish(finishtoken);
							processesleft -= 1;
							//Also add stats before removing
							addstats(statholder, processorlist[i].currentlyrunning->parentprocess, time);
							removeprocess(processorlist[i].currentlyrunning->parentprocess);
						}
						//Replacing removesubprocess
						if (processorlist[i].currentlyrunning->prev == NULL)
						{
							processorlist[i].listhead = processorlist[i].currentlyrunning->next;
							//If this wasn't the last subprocess, set next subprocess prev to NULL
							if (processorlist[i].currentlyrunning->next != NULL)
							{
								processorlist[i].currentlyrunning->next->prev = NULL;
							}
						}
						else
						{
							processorlist[i].currentlyrunning->prev->next = processorlist[i].currentlyrunning->next;
							if (processorlist[i].currentlyrunning->next != NULL)
							{
								processorlist[i].currentlyrunning->next->prev = processorlist[i].currentlyrunning->prev;
							}
						}
						free(processorlist[i].currentlyrunning);
						//Look for a new process to run
						processorlist[i].currentlyrunning = NULL;
						processorstarted[i] = true;
					}
				}
			}

			//Each process checks to see if it starts
			process* processcheckstart = processlisthead;
			while (processcheckstart != NULL)
			{
				//Start process
				if (time == processcheckstart->starttime)
				{
					processesleft += 1;
					//default behaviour from non-parallel prcesses
					if (processcheckstart->canparallel == false)
					{
						subprocess* subprocessstart = subprocessinit(processcheckstart, -1, processcheckstart->runtime);
						//This will be helpful later because we can reuse it
						int subprocesstime = subprocessstart->remainingruntime;
						//find processor to put subprocess into and do so
						int targetprocessor = 0;
						for (int i = 0; i < processorcount; i++)
						{
							if (processorlist[i].totalruntimeleft < processorlist[targetprocessor].totalruntimeleft)
							{
								targetprocessor = i;
							}
						}
						//replace subprocessadd
						if (processorlist[targetprocessor].listhead == NULL)
						{
							processorlist[targetprocessor].listhead = subprocessstart;
						}
						else
						{
							recursiveaddsubprocess(subprocessstart, processorlist[targetprocessor].listhead);
						}
						processorlist[targetprocessor].totalruntimeleft += subprocesstime;
						//Now check if subprocess immediately starts
						//Happens if no current subprocess or if faster than current subprocess
						if (processorlist[targetprocessor].currentlyrunning == NULL)
						{
							//Don't immediately set, important
							processorstarted[targetprocessor] = true;
						}
						else if (processorlist[targetprocessor].currentlyrunning->remainingruntime > subprocesstime)
						{
							processorlist[targetprocessor].currentlyrunning = subprocessstart;
							processorstarted[targetprocessor] = true;
						}
						//Case to check if processnum is lower.
						else if (processorlist[targetprocessor].currentlyrunning->remainingruntime == subprocesstime && subprocessstart->processnum < processorlist[targetprocessor].currentlyrunning->processnum)
						{
							processorlist[targetprocessor].currentlyrunning = subprocessstart;
							processorstarted[targetprocessor] = true;
						}
					}
					//Parallelable behaviour
					else
					{
						//set subprocess time and number
						int subprocesstime;
						int subprocesscount;
						if (processcheckstart->runtime < processorcount)
						{
							subprocesstime = 2;
							subprocesscount = processcheckstart->runtime;
						}
						else
						{
							double a = processcheckstart->runtime;
							double b = processorcount;
							subprocesstime = ceil(a / b) + 1;
							subprocesscount = processorcount;
						}
						//iterate through all subprocesses
						//reset used
						for (int i = 0; i < processorcount; i++)
						{
							processorlist[i].used = false;
						}
						processcheckstart->subprocessesleft = subprocesscount;
						for (int i = 0; i < subprocesscount; i++)
						{
							subprocess* subprocessstart = subprocessinit(processcheckstart, i, subprocesstime);
							//find processor to put subprocess into and do so
							//Find first unused
							int targetprocessor = 0;
							for (int i = 0; i < processorcount; i++)
							{
								if (processorlist[i].used == false)
								{
									targetprocessor = i;
									break;
								}
							}
							//Now find optimal spot
							for (int i = targetprocessor; i < processorcount; i++)
							{
								if (processorlist[i].totalruntimeleft < processorlist[targetprocessor].totalruntimeleft)
								{
									targetprocessor = i;
								}
							}
							//replace subprocessadd
							if (processorlist[targetprocessor].listhead == NULL)
							{
								processorlist[targetprocessor].listhead = subprocessstart;
							}
							else
							{
								recursiveaddsubprocess(subprocessstart, processorlist[targetprocessor].listhead);
							}
							processorlist[targetprocessor].totalruntimeleft += subprocesstime;
							//Now check if subprocess immediately starts
							//Happens if no current subprocess or if faster than current subprocess
							if (processorlist[targetprocessor].currentlyrunning == NULL)
							{
								//Don't immediately set, important
								processorstarted[targetprocessor] = true;
							}
							else if (processorlist[targetprocessor].currentlyrunning->remainingruntime > subprocesstime)
							{
								processorlist[targetprocessor].currentlyrunning = subprocessstart;
								processorstarted[targetprocessor] = true;
							}
							//Case to check if processnum is lower.
							else if (processorlist[targetprocessor].currentlyrunning->remainingruntime == subprocesstime && subprocessstart->processnum < processorlist[targetprocessor].currentlyrunning->processnum)
							{
								processorlist[targetprocessor].currentlyrunning = subprocessstart;
								processorstarted[targetprocessor] = true;
							}
						}
					}
				}
				processcheckstart = processcheckstart->next;
			}
			//Generate printstart token
			for (int i = 0; i < processorcount; i++)
			{
				if (processorstarted[i] == true)
				{
					//If we haven't set the currently running subprocess, do so
					if (processorlist[i].currentlyrunning == NULL && processorlist[i].listhead != NULL)
					{
						subprocess* selected = processorlist[i].listhead;
						subprocess* check = processorlist[i].listhead->next;
						while (check != NULL)
						{
							if (selected->remainingruntime > check->remainingruntime)
							{
								selected = check;
							}
							else if (selected->remainingruntime == check->remainingruntime && selected->processnum > check->processnum)
							{
								selected = check;
							}
							check = check->next;
						}
						processorlist[i].currentlyrunning = selected;
					}
					if (processorlist[i].currentlyrunning != NULL)
					{
						printstart* starttoken = printstartinit(processorlist[i].currentlyrunning, i);
						addprintstart(starttoken);
					}
					processorstarted[i] = false;
				}
			}
			//Print everything
			printall(time, processesleft);
			if (processlisthead == NULL)
			{
				break;
			}
			time += timeinterval;
		}
	}
	printstats(statholder, time);
	return 0;
}

//Former contents of processor.c
//Since C doesn't have "this" keyword, we will capitalise inputs to make it more clear what is going on

process* processinit(int Processnum, int Starttime, int Timetorun, bool Canparallel)
{
	process* newprocess = malloc(sizeof(process));
	newprocess->running = false;
	newprocess->processnum = Processnum;
	newprocess->starttime = Starttime;
	newprocess->canparallel = Canparallel;
	newprocess->runtime = Timetorun;
	newprocess->subprocessesleft = 1;
	newprocess->prev = NULL;
	newprocess->next = NULL;
	return newprocess;
}

subprocess* subprocessinit(process* Parentprocess, int Subprocessnum, int Runtime)
{
	subprocess* newsubprocess = malloc(sizeof(subprocess));
	newsubprocess->parentprocess = Parentprocess;
	newsubprocess->processnum = Parentprocess->processnum;
	newsubprocess->subprocessnum = Subprocessnum;
	newsubprocess->remainingruntime = Runtime;
	newsubprocess->prev = NULL;
	newsubprocess->next = NULL;
	return newsubprocess;
}

//Functions to delete objects after we finish with them.
//This handles clearing memory and also handles linked list cohesion:
//If this is the head make next the head, else prev's next is this's next and vice versa
void removeprocess(process* processtodel)
{
	if (processtodel->prev == NULL)
	{
		processlisthead = processtodel->next;
		//If this wasn't the last process, set next process prev to NULL
		if (processtodel->next != NULL)
		{
			processtodel->next->prev = NULL;
		}
	}
	else
	{
		processtodel->prev->next = processtodel->next;
		if (processtodel->next != NULL)
		{
			processtodel->next->prev = processtodel->prev;
		}
	}
	free(processtodel);
}

//Linked list addition functionality
void recursiveaddprocess(process* addprocess, process* checkingprocess)
{
	if (checkingprocess->next == NULL)
	{
		checkingprocess->next = addprocess;
		addprocess->prev = checkingprocess;
	}
	else
	{
		recursiveaddprocess(addprocess, checkingprocess->next);
	}
}

void recursiveaddsubprocess(subprocess* addsubprocess, subprocess* checkingsubprocess)
{
	if (checkingsubprocess->next == NULL)
	{
		checkingsubprocess->next = addsubprocess;
		addsubprocess->prev = checkingsubprocess;
	}
	else
	{
		recursiveaddsubprocess(addsubprocess, checkingsubprocess->next);
	}
}


//Printstart and Printfinish

printstart* printstartinit(subprocess* subprocessprint, int processorid)
{
	printstart* newprinttoken = malloc(sizeof(printstart));
	newprinttoken->remainingtime = subprocessprint->remainingruntime;
	newprinttoken->processnum = subprocessprint->processnum;
	newprinttoken->subprocessnum = subprocessprint->subprocessnum;
	newprinttoken->processorid = processorid;
	newprinttoken->nexttoken = NULL;
	newprinttoken->prevtoken = NULL;
	return newprinttoken;
}

printfinish* printfinishinit(process* processprint)
{
	printfinish* newprinttoken = malloc(sizeof(printfinish));
	newprinttoken->processnum = processprint->processnum;
	newprinttoken->nexttoken = NULL;
	newprinttoken->prevtoken = NULL;
	return newprinttoken;
}

//We'll use this to add print tokens to the list
void addprintstart(printstart* addprintstart)
{
	if (printstarthead == NULL)
	{
		printstarthead = addprintstart;
	}
	else
	{
		recursiveaddprintstart(addprintstart, printstarthead);
	}
}

void recursiveaddprintstart(printstart* newprintstart, printstart* checkingprintstart)
{
	if (checkingprintstart->nexttoken == NULL)
	{
		checkingprintstart->nexttoken = newprintstart;
	}
	else
	{
		recursiveaddprintstart(newprintstart, checkingprintstart->nexttoken);
	}
}

//Doubly linked list
void removeprintstart(printstart* printstarttoremove)
{
	if (printstarttoremove->prevtoken == NULL)
	{
		printstarthead = printstarttoremove->nexttoken;
		//If this wasn't the last subprocess, set next subprocess prev to NULL
		if (printstarttoremove->nexttoken != NULL)
		{
			printstarttoremove->nexttoken->prevtoken = NULL;
		}
	}
	else
	{
		printstarttoremove->prevtoken->nexttoken = printstarttoremove->nexttoken;
		printstarttoremove->nexttoken->prevtoken = printstarttoremove->prevtoken;
	}
	free(printstarttoremove);
}

//We'll use this to add print tokens to the list
void addprintfinish(printfinish* addprintfinish)
{
	if (printfinishhead == NULL)
	{
		printfinishhead = addprintfinish;
	}
	else
	{
		recursiveaddprintfinish(addprintfinish, printfinishhead);
	}
}

void recursiveaddprintfinish(printfinish* newprintfinish, printfinish* checkingprintfinish)
{
	if (checkingprintfinish->nexttoken == NULL)
	{
		checkingprintfinish->nexttoken = newprintfinish;
	}
	else
	{
		recursiveaddprintfinish(newprintfinish, checkingprintfinish->nexttoken);
	}
}

//Doubly linked list
void removeprintfinish(printfinish* printfinishtoremove)
{
	if (printfinishtoremove->prevtoken == NULL)
	{
		printfinishhead = printfinishtoremove->nexttoken;
		//If this wasn't the last subprocess, set next subprocess prev to NULL
		if (printfinishtoremove->nexttoken != NULL)
		{
			printfinishtoremove->nexttoken->prevtoken = NULL;
		}
	}
	else
	{
		printfinishtoremove->prevtoken->nexttoken = printfinishtoremove->nexttoken;
		printfinishtoremove->nexttoken->prevtoken = printfinishtoremove->prevtoken;
	}
	free(printfinishtoremove);
}



//Finish only needs to be used for full processes
//Processes remaining only counts processes already activated
//Tasks finish simultaneously, so should have same processes left
void printfinishtoken(int currenttime, int processid, int processesremain)
{
	printf("%d,FINISHED,pid=%d,proc_remaining=%d\n", currenttime, processid, processesremain);
}

//Start is used for full processes(via the -1 interface) and subprocesses
void printstarttoken(int currenttime, int processid, int subprocessid, int timetorun, int processorid)
{
	//This is meant to represent a non-parallelised process
	if (subprocessid == -1)
	{
		printf("%d,RUNNING,pid=%d,remaining_time=%d,cpu=%d\n", currenttime, processid, timetorun, processorid);
	}
	//This represents a subprocess
	else
	{
		printf("%d,RUNNING,pid=%d.%d,remaining_time=%d,cpu=%d\n", currenttime, processid, subprocessid, timetorun, processorid);
	}
}

statholder* statholderinit()
{
	statholder* newstatholder = malloc(sizeof(statholder));
	newstatholder->totalturnaroundtime = 0;
	newstatholder->totaloverhead = 0;
	newstatholder->maxoverhead = 0;
	newstatholder->totalprocesses = 0;
	return newstatholder;
}

void addstats(statholder* statholder, process* processtoadd, int currenttime)
{
	double timediff = currenttime - processtoadd->starttime;
	statholder->totalturnaroundtime += timediff;
	double overhead = timediff / processtoadd->runtime;
	if (overhead > statholder->maxoverhead)
	{
		statholder->maxoverhead = overhead;
	}
	statholder->totaloverhead += overhead;
	statholder->totalprocesses += 1;
}

//Print out stats 
//We need to round to integer for turnabouttime
//Round to 2dp for overhead and avoid trailing 0s
void printstats(statholder* statholder, int finaltime)
{
	int avgturnabout = (int)ceil(statholder->totalturnaroundtime / statholder->totalprocesses);
	double avgoverhead = statholder->totaloverhead / statholder->totalprocesses;
	//rounding to 2 dp for overhead
	double maxoverhead = round(statholder->maxoverhead * 100) / 100;
	avgoverhead = round(avgoverhead * 100) / 100;
	printf("Turnaround time %d\n", avgturnabout);
	printf("Time overhead %g %g\n", maxoverhead, avgoverhead);
	printf("Makespan %d\n", finaltime);
}

//
void printall(int currenttime, int processesrunning) 
{
	while (printfinishhead != NULL) 
	{
		//Look for next token to print
		printfinish* nextprintf = printfinishhead;
		printfinishtoken(currenttime,nextprintf->processnum,processesrunning);
		removeprintfinish(nextprintf);
	}
	while (printstarthead != NULL)
	{
		//Look for next token to print
		printstart* nextprints = printstarthead;
		printstarttoken(currenttime, nextprints->processnum, nextprints->subprocessnum, nextprints->remainingtime, nextprints->processorid);
		removeprintstart(nextprints);
	}
}

//This is to fix test_p4_n_2
//Although it also fixes as other situation where tasks at the same time are not in order
void reorderprocesslist() 
{
	process *processtocheck = processlisthead;
	//Work until all are in order
	while (true) 
	{
		bool allclear = true;
		while (processtocheck != NULL) 
		{
			//Do NOT dereference nullpointer
			if (processtocheck->next != NULL) 
			{
				//If true trade places
				if (processtocheck->starttime == processtocheck->next->starttime && processtocheck->runtime > processtocheck->next->runtime) 
				{
					allclear = false;
					//If this is the list head, make next the list head
					if (processlisthead == processtocheck) 
					{
						processlisthead = processtocheck->next;
					}
					process* tempholder = processtocheck->next;
					//Take care of far references
					if (processtocheck->prev != NULL) 
					{
						processtocheck->prev->next = tempholder;
					}
					if (tempholder->next != NULL)
					{
						tempholder->next->prev = processtocheck;
					}
					//Take care of tempholder and processtocheck
					tempholder->prev = processtocheck->prev;
					processtocheck->next = tempholder->next;
					tempholder->next = processtocheck;
					processtocheck->prev = tempholder;
				}
				//If next != null but not valid to swap
				else
				{
					processtocheck = processtocheck->next;
				}
			}
			else
			{
				processtocheck = processtocheck->next;
			}
		}

		//If we were all clear, break, else, try again
		if (allclear == true) 
		{
			break;
		}
		else 
		{
			processtocheck = processlisthead;
		}
	}
}
