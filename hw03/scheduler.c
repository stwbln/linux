#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

typedef enum { RUNNING, READY, SLEEP, DONE } State;
typedef struct {
    pid_t pid;
    int cpu_burst;
    int tq_remaining;
    State state;
    int wait_time;
    int io_wait;
} PCB;

extern PCB processes[];
extern int process_count;

static int base_q = 1;
static int current = -1;
static int tick = -1;

static int all_quantum_zero(void) {
    for (int i = 0; i < process_count; i++) {
        if (processes[i].state == READY || processes[i].state == RUNNING) {
            if (processes[i].tq_remaining > 0) return 0;
        }
    }
    return 1;
}

static int next_runnable(int start) {
    for (int k = 0; k < process_count; k++) {
        int i = (start + k) % process_count;
        if (processes[i].state == READY) return i;
    }
    return -1;
}

void scheduler_init(int base_quantum) {
    base_q = base_quantum;
    for (int i = 0; i < process_count; ++i) {
        if (processes[i].state == READY) {
            processes[i].tq_remaining = base_q;
        }
    }
    current = next_runnable(0);
}

static void dispatch(int idx) {
    if (idx < 0) return;
    processes[idx].state = RUNNING;
    kill(processes[idx].pid, SIGUSR2);
}

// 자식이 burst 값을 전달할 때
void scheduler_on_child_burst(int sig, siginfo_t* info, void* ctx) {
    pid_t child = info->si_pid;
    int burst = info->si_value.sival_int;
    for (int i = 0; i < process_count; i++) {
        if (processes[i].pid == child) {
            processes[i].cpu_burst = burst;
            break;
        }
    }
}

// 자식이 I/O 요청을 보낼 때
void scheduler_on_child_io(int sig, siginfo_t* info, void* ctx) {
    pid_t child = info->si_pid;
    int idx = -1;
    for (int i = 0; i < process_count; i++) {
        if (processes[i].pid == child) { idx = i; break; }
    }
    if (idx < 0) return;

    PCB* p = &processes[idx];
    p->state = SLEEP;
    p->io_wait = rand() % 5 + 1;
    printf("[Tick %d] Process %d I/O 요청 (대기=%d)\n", tick, p->pid, p->io_wait);

    if (current == idx) {
        current = next_runnable((current + 1) % process_count);
        if (current >= 0) dispatch(current);
    }
}

void scheduler_on_alarm(int sig) {
    tick++;

    // I/O 대기 감소
    for (int i = 0; i < process_count; i++) {
        if (processes[i].state == SLEEP) {
            processes[i].io_wait--;
            if (processes[i].io_wait <= 0) {
                processes[i].state = READY;
                if (processes[i].tq_remaining == 0) processes[i].tq_remaining = base_q;
                printf("[Tick %d] Process %d I/O 완료 → READY\n", tick, processes[i].pid);
            }
        }
    }

    if (current < 0) current = next_runnable(0);

    PCB* p = &processes[current];

    if (p->state == RUNNING) {
        // 매 틱마다 자식에게 실행 시그널 전송 -> 자식이 cpu_burst-- 하고 SIGUSR1로 보고
        kill(p->pid, SIGUSR2);

        if (p->tq_remaining > 0) p->tq_remaining--;
        printf("[Tick %d] Process %d 실행 (남은 burst=%d, tq=%d)\n",
            tick, p->pid, p->cpu_burst, p->tq_remaining);

        for (int i = 0; i < process_count; i++) {
            if (i != current && processes[i].state == READY) {
                processes[i].wait_time++;
            }
        }
    } else if (p->state == READY) {
        dispatch(current);
    }

    if (p->state == RUNNING && p->tq_remaining == 0) {
        p->state = READY;
        p->tq_remaining = base_q;   // ready로 돌아갈때 타임퀀텀 재설정
        printf("[Tick %d] Process %d 타임퀀텀 소진 → READY\n", tick, p->pid);
        current = next_runnable((current + 1) % process_count);
        if (current >= 0) dispatch(current);
    }

    alarm(1);
}

void scheduler_start() {
    struct sigaction sa_burst = {0};
    sa_burst.sa_sigaction = scheduler_on_child_burst;
    sa_burst.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa_burst, NULL);

    struct sigaction sa_io = {0};
    sa_io.sa_sigaction = scheduler_on_child_io;
    sa_io.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &sa_io, NULL);

    alarm(1);

    int done = 0;
    while (done < process_count) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            for (int i = 0; i < process_count; i++) {
                if (processes[i].pid == pid) {
                    processes[i].state = DONE;
                    printf("[Tick %d] Process %d 종료\n", tick, processes[i].pid);
                    break;
                }
            }
            done++;
            if (current >= 0 && processes[current].pid == pid) {
                current = next_runnable((current + 1) % process_count);
                if (current >= 0) dispatch(current);
            }
        }
        pause();
    }
}