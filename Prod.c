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
int nShmid;

/*Error Codes---

1 - Standard program message
2 - Shared memory error
3 - Forking/child error
4 - PrintJob logs
*/

int main(int argc, char *argv[]){
	
	int pid,childpid, status, counter, JPC,MaxJobs,NumofJobs,offset, loop;/* JPC = JobsPerCycle*/
	key_t nKey;
	char *SharedMem,answer;	
	JobDetails *PrintJobs;
	time_t current_time;


	atexit(Termination);
	LogString(1,"Application started");
	
	
	if(argc > 3 || argc < 2){
		printf("Incorrect arguments!(Arg1 = Num of jobs created per cycle, Arg2 = Maximum number of jobs)\n\n");
		exit(1);
	}
	JPC = atoi(argv[1]);
	MaxJobs = atoi(argv[2]);
	nKey = 3000;
	NumofJobs = 1;/*Default value cant be 0, as new memory is initialised at 0*/
	
	LogStringInt(1,"Jobs created per cycle set to",JPC);
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
	
	for(counter = 0; counter <MaxJobs;counter++){
		PrintJobs[counter].jobid = -1; /*Set each printer job to -1 to flag it as empty*/
	}
	LogStringInt(1,"Shared memory created and attatched with ID: ",nShmid);

	
	loop = 1;
	while(loop){
	
		for(counter = 0; counter <JPC;counter++){
			pid = fork();
			
			
			
			if(pid < 0){ /*Catch error, exit if found*/
				LogStringError(3,"Forking Child",strerror(errno));
				exit(1);
			}else if(pid == 0){
				childpid = getpid();
				LogStringInt(1,"Child created with id",childpid);
				
				srand((unsigned) time(&current_time) * childpid);
				
				offset = 0;		
				while(PrintJobs[offset].jobid != -1){ /*Find next availible Job space in memory/Queue*/
					offset++;
					if(offset >= MaxJobs){ 
						offset = 0;

						printf("\r[Error] When adding Job - Print Queue Full (JobId: %d) retrying...\r", childpid);/*Queue is full for the printer, 
																														so keep attempting to add next printer job*/
						
						usleep(1000 * 200);
					}										
				}
											
				PrintJobs[offset].jobid = childpid;
				PrintJobs[offset].jobnumber = NumofJobs;
				PrintJobs[offset].jobpriority = (rand() % 1000)+ 1;
				
				LogPrintJob(4,"Successfully created print job",PrintJobs[offset].jobid,	PrintJobs[offset].jobnumber,PrintJobs[offset].jobpriority);
				
						
				printf("\n\nPrint Job [%d] Successfully created!\n",PrintJobs[offset].jobnumber);
				printf("------------------------------\n");
				printf("Job Number: %d    ID: %d\nPriority: %d\nTime: %s",PrintJobs[offset].jobnumber,PrintJobs[offset].jobid,PrintJobs[offset].jobpriority, GetTime());
				printf("------------------------------\n\n");
				
				
				
				_Exit(0);/* Calls exit without triggering atexit inherited from its parent*/
			
			}else{			
				wait(&status);
				usleep(1000 * 200); /*Sleep between creating jobs*/
				NumofJobs++;
			}
		}
		
		while (! (wait(0) == -1 && errno == ECHILD) ); /*Wait for all childeren to finish*/
		
		LogString(1,"All childeren finished processing");
		
		printf("\nDo you want to cycle again?(Create %d more jobs) Y or N: \n",JPC);
		scanf(" %c", &answer);
		if(answer == 'y' || answer == 'Y'){
			printf("\nRe-Cycling....\n");
			LogStringInt(1,"Starting a new cycle with JPC set to", JPC);
			PrintJobs[MaxJobs].jobnumber = 1;
			sleep(1);
		}else{
			PrintJobs[MaxJobs].jobnumber = 0;
			exit(0);
		}
		
	}
	
	exit(0);

}

void Termination(){
	LogString(1,"Application ended");
	shmctl(nShmid, IPC_RMID, NULL); /* Clear shared memory segment*/
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

	LogFile = fopen("Prod_LogFile.txt", "a");
	
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
