#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>

#define MEM_SIZE (100 * 1024 * 1024) // 100MB
#define PAGE_SIZE 4096

// 현재 프로세스의 Minor Page Fault 횟수를 반환하는 함수
long get_minor_page_faults() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_minflt;
}

int main() {
    // 1. 100MB 메모리 할당 및 초기화
    char *data = malloc(MEM_SIZE);
    memset(data, 1, MEM_SIZE); 

    pid_t pid = fork();

    if (pid == 0) { // 자식 프로세스 (Child)
        long pf_before = get_minor_page_faults();

        // 2. 읽기 (Read) 시도
        // 이론상 CoW로 인해 복사가 안 일어나므로 Page Fault가 거의 없어야 함
        volatile char tmp;
        for (int i = 0; i < MEM_SIZE; i += PAGE_SIZE) tmp = data[i];
        
        long pf_after_read = get_minor_page_faults();

        // 3. 쓰기 (Write) 시도
        // 이때 CoW 발생! 페이지 복사가 일어나며 Page Fault가 급증해야 함
        for (int i = 0; i < MEM_SIZE; i += PAGE_SIZE) data[i] = 2;
        
        long pf_after_write = get_minor_page_faults();

        // 결과 출력 (이 부분은 눈으로 확인하려고 추가했습니다)
        printf("=== 결과 확인 ===\n");
        printf("Read로 인한 Page Fault 증가량 : %ld\n", pf_after_read - pf_before);
        printf("Write로 인한 Page Fault 증가량: %ld (여기가 훨씬 커야 함!)\n", pf_after_write - pf_after_read);

        free(data);
        exit(0);
    } else { // 부모 프로세스 (Parent)
        wait(NULL);
    }

    free(data);
    return 0;
}
