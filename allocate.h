//Linked list functionality taken from code from COMP20007 
//(Design of Algorithms) Assessment 1 2020
//Deque.c and Deque.h
//Used in pretty much every struct

//Typedefs to avoid unknown name problems
typedef struct process Process;
typedef struct subprocess Subprocess;
typedef struct processor Processor;
typedef struct printtoken Printtoken;
typedef struct printfinish Printfinish;
typedef struct printstart Printstart;

//Each process is made up of 1 or more subprocesses, when each subprocess is finished
//it reports to the main process. Processes are kept in a doubly linked list.
//Process will NEVER touch processors directly, even if parallelisation is disabled
//we will use "1 subprocess".
typedef struct process
{
	bool running;
	int runtime;
	int subprocessesleft;
	int processnum;
	int starttime;
	bool canparallel;
	Process* prev;
	Process* next;
} process;

//Subprocesses are kept in a doubly linked list by their assigned processor
//When a subprocess finishes it reports to the main process
typedef struct subprocess
{
	int remainingruntime;
	int processnum;
	int subprocessnum;
	Process* parentprocess;
	Subprocess* prev;
	Subprocess* next;
} subprocess;

//Processors are kept in a SINGLY linked list
//Used is IMPORTANT FOR MULTIPROCESSING, variable length arrays don't seem to work
typedef struct processor
{
	int processorid;
	int totalruntimeleft;
	Subprocess* listhead;
	Subprocess* currentlyrunning;
	bool used;
} processor;

//We now use different finish and start tokens
typedef struct printfinish
{
	int processnum;
	Printfinish* nexttoken;
	Printfinish* prevtoken;
} printfinish;

typedef struct printstart
{
	int remainingtime;
	int processnum;
	int subprocessnum;
	int processorid;
	Printstart* nexttoken;
	Printstart* prevtoken;
} printstart;

//Holder for stats
//We will hold total turnaround time, total overhead, max overhead and total processes.
//Then, we can get avg turnaround and avg overhead when we need them.
typedef struct statholder
{
	double totalturnaroundtime;
	double totaloverhead;
	double maxoverhead;
	double totalprocesses;
} statholder;

process* processinit(int Processnum, int Starttime, int Timetorun, bool Canparallel);

subprocess* subprocessinit(process* Parentprocess, int Subprocessnum, int Runtime);

//Functions to delete objects after we finish with them.
//This handles clearing memory and also handles linked list cohesion:
//If this is the head make next the head, else prev's next is this's next and vice versa
void removeprocess(process* processtodel);

//Linked list addition functionality
void recursiveaddprocess(process* addprocess, process* checkingprocess);

void recursiveaddsubprocess(subprocess* addsubprocess, subprocess* checkingsubprocess);

printstart* printstartinit(subprocess* subprocessprint, int processorid);

printfinish* printfinishinit(process* processprint);

//We'll use this to add print tokens to the list
void addprintstart(printstart* addprintstart);

void recursiveaddprintstart(printstart* newprintstart, printstart* checkingprintstart);

//Doubly linked list
void removeprintstart(printstart* printstarttoremove);

//We'll use this to add print tokens to the list
void addprintfinish(printfinish* addprintfinish);

void recursiveaddprintfinish(printfinish* newprintfinish, printfinish* checkingprintfinish);

//Doubly linked list
void removeprintfinish(printfinish* printfinishtoremove);

//Finish only needs to be used for full processes
//Processes remaining only counts processes already activated
//Tasks finish simultaneously, so should have same processes left
void printfinishtoken(int currenttime, int processid, int processesremain);

//Start is used for full processes(via the -1 interface) and subprocesses
void printstarttoken(int currenttime, int processid, int subprocessid, int timetorun, int processorid);

statholder* statholderinit();

void addstats(statholder* statholder, process* processtoadd, int currenttime);

//Print out stats 
//We need to round to integer for turnabouttime
//Round to 2dp for overhead and avoid trailing 0s
void printstats(statholder* statholder, int finaltime);

void printall(int currenttime, int processesrunning);

//This is to fix test_p4_n_2
//Although it also fixes as other situation where tasks at the same time are not in order
void reorderprocesslist();