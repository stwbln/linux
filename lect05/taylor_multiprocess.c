#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>

#define N 4
#define TERMS 3

static double sinx_taylor(double x, int terms) {
    double value = x, numer = x*x*x, denom = 6.0;
    int sign = -1;
    for (int j = 1; j <= terms; j++) {
        value += sign * numer / denom;
        numer *= x*x;
        denom *= (2.0*j + 2) * (2.0*j + 3);
        sign = -sign;
    }
    return value;
}

int main(void) {
    double x[N] = {0.0, M_PI/6.0, M_PI/3.0, 0.134};
    double result[N];
    int fd[N][2];
    pid_t pids[N];

    for (int i = 0; i < N; i++) {
        if (pipe(fd[i]) == -1) { perror("pipe"); return 1; }
    }

    for (int i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }
        if (pid == 0) {
            for (int k = 0; k < N; k++) {
                if (k == i) {
                    close(fd[k][0]);
                } else {
                    close(fd[k][0]);
                    close(fd[k][1]);
                }
            }
            double res = sinx_taylor(x[i], TERMS);
            ssize_t w = write(fd[i][1], &res, sizeof(double));
            if (w != (ssize_t)sizeof(double)) { perror("write"); }
            close(fd[i][1]);
            _exit(0);
        } else {
            pids[i] = pid;
            close(fd[i][1]);
        }
    }

    for (int i = 0; i < N; i++) {
        ssize_t r = read(fd[i][0], &result[i], sizeof(double));
        if (r != (ssize_t)sizeof(double)) { perror("read"); result[i] = NAN; }
        close(fd[i][0]);
    }

    for (int i = 0; i < N; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0) { perror("waitpid"); }
    }

    for (int i = 0; i < N; i++) {
        printf("sin(%.3f) by Taylor = %.6f\n", x[i], result[i]);
        printf("sin(%.3f)          = %.6f\n", x[i], sin(x[i]));
    }
    return 0;
}
