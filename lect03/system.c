#include <sys/types.h>  /* system.c */
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

int system(const char *cmdstring)
{
    pid_t pid;
    int status;

    if (cmdstring == NULL) /* 명령어가 NULL인 경우 */
        return 1;

    if ((pid = fork()) < 0) {
        status = -1; /* 프로세스 생성 실패 */
    } else if (pid == 0) {   /* 자식 */
        execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
        _exit(127); /* exec() 실패 */
    } else {   /* 부모 */
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1; /* waitpid()로부터 EINTR 외의 오류 */
                break;
            }
        }
    }
    return status;
}

