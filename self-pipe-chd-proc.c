#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int pipefds[2];
    int count, err;
    pid_t child;

    if (pipe(pipefds)) {
        perror("pipe");
        return EX_OSERR;
    }
    if (fcntl(pipefds[1], F_SETFD, fcntl(pipefds[1], F_GETFD) | FD_CLOEXEC)) {
        perror("fcntl");
        return EX_OSERR;
    }

    switch (child = fork()) {
    case -1:
        perror("fork");
        return EX_OSERR;
    case 0:
        close(pipefds[0]);
        execvp(argv[1], argv + 1);
        write(pipefds[1], &errno, sizeof(int));
        //_exit(0);
        _exit(errno);
    default:
        close(pipefds[1]);
        while ((count = read(pipefds[0], &err, sizeof(errno))) == -1)
            if (errno != EAGAIN && errno != EINTR) break;
        if (count) {
            fprintf(stderr, "child's execvp: %s,errno=%d\n", strerror(err),err);
            //return EX_UNAVAILABLE;
        }
        close(pipefds[0]);
        puts("waiting for child...");
        while (waitpid(child, &err, 0) == -1)
            if (errno != EINTR) {
                perror("waitpid");
                return EX_SOFTWARE;
            }
        if (WIFEXITED(err))
            printf("child exited with %d\n", WEXITSTATUS(err));
        else if (WIFSIGNALED(err))
            printf("child killed by %d\n", WTERMSIG(err));
    }
    return err;
}
