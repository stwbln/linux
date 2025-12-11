#include <stdio.h>

typedef enum { RUNNING, READY, SLEEP, DONE } State;
typedef struct {
    int pid;
    int cpu_burst;
    int tq_remaining; 
    State state; // 현재 상태
    int wait_time; // 누적 대기 시간
    int io_wait; // I/O 대기 시간
} PCB;

extern PCB processes[];
extern int process_count;

void show_analysis() {
    double total = 0.0;
    for (int i = 0; i < process_count; ++i) {
        total += processes[i].wait_time;
    }
    printf("Average waiting time: %.2f (over %d processes)\n",
           total / (process_count ? process_count : 1), process_count);
}