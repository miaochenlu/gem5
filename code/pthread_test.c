

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sched.h>

void WasteTime(void)
{
    
    int abc = 1000;
	int tmp = 0;
    while(abc--)
        tmp = 10000*10000;
    	// sleep(1);
}

int main(int argc, char *argv[])
{
    
    cpu_set_t cpu_set;
    int i = 10;
    while(i > 0)
    {
        i--;
        CPU_ZERO(&cpu_set);
        CPU_SET(0, &cpu_set);
        if(sched_setaffinity(0, sizeof(cpu_set), &cpu_set) < 0)
            perror("sched_setaffinity");
        WasteTime();

        CPU_ZERO(&cpu_set);
        CPU_SET(1, &cpu_set);
        if(sched_setaffinity(0, sizeof(cpu_set), &cpu_set) < 0)
            perror("sched_setaffinity");
        WasteTime();
        CPU_ZERO(&cpu_set);
        CPU_SET(2, &cpu_set);
        if (sched_setaffinity(0, sizeof(cpu_set), &cpu_set) < 0)
            perror("sched_setaffinity");
        WasteTime();


        CPU_ZERO(&cpu_set);
        CPU_SET(3, &cpu_set);
        if(sched_setaffinity(0, sizeof(cpu_set), &cpu_set) < 0)
            perror("sched_setaffinity");
        WasteTime();
    }
    return 0;
}