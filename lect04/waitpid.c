#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
    int status;
    pid_t pid;

    // fork 생성
    if ((pid = fork()) < 0) {  // fork failed
        perror("fork");
        exit(1);
    }

    if (pid == 0) {  // child process
        printf("--> Child process\n");
        sleep(3);
        exit(3);
    } else {  // parent process
        printf("--> Parent process\n");

        // 자식이 끝날 때까지 기다리되, WNOHANG으로 비동기 확인
        while (waitpid(pid, &status, WNOHANG) == 0) {
            printf("Parent still waiting...\n");
            sleep(1);
        }

        printf("Child Exit Status: %d\n", status >> 8);
    }
}

