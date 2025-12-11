#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static int cpu_burst = 0;

static void on_run(int sig) {
    if (cpu_burst > 0) cpu_burst--;

    // 부모에게 현재 남은 burst 전달
    union sigval val;
    val.sival_int = cpu_burst;
    sigqueue(getppid(), SIGUSR1, val);

    if (cpu_burst == 0) {
        if (rand() % 2 == 0) {
            _exit(0); // 종료
        } else {
            // I/O 요청 알림 (burst=0 상태에서)
            kill(getppid(), SIGUSR2);
            // 부모가 I/O 완료 후 다시 SIGUSR2를 보낼 때까지 대기
        }
    }
}

void run_child(int id) {
    cpu_burst = rand() % 10 + 1;

    struct sigaction sa = {0};
    sa.sa_handler = on_run;
    sigaction(SIGUSR2, &sa, NULL);

    for (;;) pause();
}