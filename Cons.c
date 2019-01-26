#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>	
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "Headers/JobDetails.h"
#include "Headers/LogFunc.h"

char* GetTime();
void Termination();
int GetPrintJob(JobDetails*, int);
int nShmid;

int main(int argc, char *argv[]){
	
	int counter,MaxJobs,JPC,PJPosition,NumofJobs,tempPos,loop;/* JPC = JobsPerCycle*/
	key_t nKey;
	char *SharedMem,answer;	
	JobDetails *PrintJobs;
	time_t current_time;
	NumofJobs = 0;
	
	atexit(Termination);
	LogString(1,"Application started");
	
	if(argc > 3 || argc < 2){
		printf("Incorrect arguments!(Arg1 = Num of jobs deleted per cycle, Arg2 = Maximum number of jobs)\n\n");
		exit(1);
	}
	
	JPC = atoi(argv[1]);
	MaxJobs = atoi(argv[2]);
	nKey = 3000;
	
	LogStringInt(1,"Jobs processed per cycle set to",JPC);
	LogStringInt(1,"Max jobs set to",MaxJobs);
	LogStringInt(1,"Shared memory key set to",nKey);

	if((nShmid = shmget(nKey,(sizeof(int)*MaxJobs*3) + 1,IPC_CREAT | 0666)) < 0){  /*Create and attach shared memory*/
		LogStringError(2,"Shared Memory",strerror(errno));
		exit(1);
	}
	
	if((int*)(PrintJobs = shmat(nShmid,NULL,0)) == (int*)-1){
		LogStringError(2,"Shared Memory",strerror(errno));
		exit(1);
	}
	
	LogStringInt(1,"Shared memory created and attatched with ID: ",nShmid);
	
	loop = 1;
	while(loop){	
		while(NumofJobs < JPC){
						
			do{
				printf("%c[2K", 27);
				printf("\rScanning for print jobs.\r");
				fflush(stdout);
				PJPosition = GetPrintJob(PrintJobs,MaxJobs);
				usleep(1000 * 500);
				printf("\rScanning for print jobs..\r");
				fflush(stdout);
				usleep(1000 * 500);
				printf("\rScanning for print jobs...\r");
				fflush(stdout);
				usleep(1000 * 500);
				tempPos = GetPrintJob(PrintJobs,MaxJobs);
			
			}while(PrintJobs[PJPosition].jobid == -1 || PJPosition != tempPos || PrintJobs[PJPosition].jobid == 0); /*Second check is just to make sure 
																													a job doesnt skip the queue with a higher priority
																													becuase it was added slightly earlier. 
																													Also check if empty(Jobid set to 0 or -1) */
	
			printf("Print Job [%d] Successfully Processed!\n",PrintJobs[PJPosition].jobnumber);
			printf("------------------------------\n");
			printf("Job Number: %d    ID: %d\nPriority: %d\nTime: %s",PrintJobs[PJPosition].jobnumber,PrintJobs[PJPosition].jobid,PrintJobs[PJPosition].jobpriority, GetTime());
			printf("------------------------------\n\n");
			
			LogPrintJob(4,"Successfully processed print job",PrintJobs[PJPosition].jobid, PrintJobs[PJPosition].jobnumber,PrintJobs[PJPosition].jobpriority);
			
			kill(PrintJobs[PJPosition].jobid, SIGKILL);
			
			PrintJobs[PJPosition].jobid = -1; /*Tells system job is empty, and allows to override*/
			PrintJobs[PJPosition].jobnumber = 0;
			PrintJobs[PJPosition].jobpriority = 0;
			NumofJobs++;
			
			printf("%d Processes left this cycle\n\n",JPC - NumofJobs);
			LogStringInt(1,"Processes left", JPC - NumofJobs);
				
		}	
		LogString(1,"All processes completed this cycle");
		PrintJobs[MaxJobs].jobnumber = -1;
		printf("\nWaiting for producer...\n");
		while(PrintJobs[MaxJobs].jobnumber == -1);
		
		if(PrintJobs[MaxJobs].jobnumber == 1){
			printf("\nRe-Cycling....\n");
			NumofJobs = 0;
			LogStringInt(1,"Starting a new cycle with JPC set to", JPC);
			sleep(1);
		
		}else if(PrintJobs[MaxJobs].jobnumber == 0){ 
		    printf("\nExiting...\n");
			LogStringInt(1,"Clearing shared memory ID: ", nShmid);
			exit(0);
		
		}
		
	}
	exit(0);
}

void Termination(){
	LogString(1,"Application ended");
	shmctl(nShmid, IPC_RMID, NULL); /* Clear shared memory segment*/
}

int GetPrintJob(JobDetails* JobList, int Length){
	JobDetails LowestPPnt;
	int counter,position,priority;
	
	
	LowestPPnt = JobList[0];
	position = 0;
	priority = JobList[0].jobpriority;
	
	for(counter = 0; counter < Length; counter++){
		
		if(JobList[counter].jobid != -1 && JobList[counter].jobpriority < priority){
			priority = JobList[counter].jobpriority;
			position = counter;	
		}
	}
	
	return position;
}

void LogString(int LogCode, char *String){

	char LogText[80];

	sprintf(LogText, "[%d] %s",LogCode, String);
	SubmitLog(LogText);
}

void LogStringInt(int LogCode, char *String, int Value){

	char LogText[80];
	sprintf(LogText, "[%d] %s [%d]",LogCode, String,Value);
	SubmitLog(LogText);
}

void LogPrintJob(int LogCode, char *String, int JobID, int JobNumber, int Priority){

	char LogText[80];
	sprintf(LogText, "[%d] %s [JobID: %d, JobNumber: %d, JobPriority: %d]",LogCode, String,JobID,JobNumber,Priority);
	SubmitLog(LogText);
}

void LogStringError(int LogCode, char *String,char *Error){

	char LogText[80];

	sprintf(LogText, "[Error] [%d] %s - %s ",LogCode, String,Error);
	SubmitLog(LogText);
}

void SubmitLog(char *LogText){
	FILE 	*LogFile;

	LogFile = fopen("Cons_LogFile.txt", "a");
	
	fprintf(LogFile, "[%s] %s\r\n",GetTime(),LogText);
	fclose(LogFile);
}

char* GetTime(){
	time_t current_time;
    char* c_time_string;

	current_time = time(NULL);

    /* Convert to local time format. */
    c_time_string = ctime(&current_time);
    
    return c_time_string;
}
