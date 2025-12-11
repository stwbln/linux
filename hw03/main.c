#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

typedef enum { RUNNING, READY, SLEEP, DONE } State;
typedef struct {
    pid_t pid;
    int cpu_burst;
    int tq_remaining;
    State state;
    int wait_time;
    int io_wait;
} PCB;

PCB processes[64];
int process_count = 0;

void scheduler_init(int base_quantum);
void scheduler_start();
void scheduler_on_alarm(int sig);
void scheduler_on_child_burst(int sig, siginfo_t* info, void* ctx); // burst 갱신용
void scheduler_on_child_io(int sig, siginfo_t* info, void* ctx);    // I/O 요청용
void show_analysis();

extern void run_child(int id); // process.c

int main(int argc, char *argv[]) {
    srand(2000);
    int n = 10;

    // 실행 시 인자로 받은 타임퀀텀 값
    int base_quantum = atoi(argv[1]);

    // ALRM 핸들러
    struct sigaction sa_alrm = {0};
    sa_alrm.sa_handler = scheduler_on_alarm;
    sigaction(SIGALRM, &sa_alrm, NULL);

    // SIGUSR1 -> burst 갱신
    struct sigaction sa_usr1 = {0};
    sa_usr1.sa_sigaction = scheduler_on_child_burst;
    sa_usr1.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    // SIGUSR2 -> I/O 요청 처리
    struct sigaction sa_usr2 = {0};
    sa_usr2.sa_sigaction = scheduler_on_child_io;
    sa_usr2.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &sa_usr2, NULL);

    // 자식 생성 및 PCB 초기화
    process_count = n;
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            run_child(i);
            _exit(0);
        }
        processes[i].pid = pid;
        processes[i].cpu_burst = rand() % 10 + 1;
        processes[i].tq_remaining = base_quantum;
        processes[i].state = READY;
        processes[i].wait_time = 0;
        processes[i].io_wait = 0;

        // 확인용 출력
        printf("Process %d (PID=%d) 초기 burst=%d\n",
            i, processes[i].pid, processes[i].cpu_burst);

    }

    printf("\n=== Time Quantum = %d ===\n", base_quantum);
    scheduler_init(base_quantum);
    scheduler_start();
    show_analysis();

    // 자식 회수
    for (int i = 0; i < n; ++i) {
        int status;
        waitpid(processes[i].pid, &status, 0);
    }

    return 0;
}