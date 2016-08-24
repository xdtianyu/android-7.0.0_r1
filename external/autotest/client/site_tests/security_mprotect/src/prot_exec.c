/*
 * Verifies that the "vm.mmap_noexec_taint" sysctl is operational.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

void err_sys(char *msg)
{
        perror(msg);
        exit(1);
}

void err_quit(char *msg)
{
        fprintf(stderr, "FAIL: %s\n", msg);
        exit(1);
}

int main(int argc, char *argv[])
{
        int fd;
        int *arearw, *areaex;

        if (argc < 2)
                err_quit("need to pass a filename");

        if ((fd = open(argv[1], O_RDWR|O_CREAT|O_EXCL, S_IRWXU)) < 0)
                err_sys("open error");
        lseek(fd, 100, SEEK_CUR);
        write(fd, "A", 1);
        if ((arearw = (int *)mmap(0, sizeof(int),
                                  PROT_READ | PROT_WRITE, MAP_SHARED,
                                  fd, 0)) == MAP_FAILED)
                err_sys("arearw mmap error");

        /* Make sure mmap with PROT_EXEC fails. */
        if ((areaex = (int *)mmap(0, sizeof(int),
                                  PROT_READ | PROT_EXEC, MAP_SHARED,
                                  fd, 0)) != MAP_FAILED)
                err_quit("areaex mmap allowed PROT_EXEC");

        if ((areaex = (int *)mmap(0, sizeof(int),
                                  PROT_READ, MAP_SHARED,
                                  fd, 0)) == MAP_FAILED)
                err_sys("areaex mmap error");
        if (mprotect(areaex, sizeof(int), PROT_READ | PROT_EXEC))
                err_sys("areaex mprotect error");
        close(fd);

        /* Make sure we start zero-filled. */
        if (*arearw != 0x0)
                err_quit("not zero-filled");

        /* Make sure we're sharing a memory area. */
        *arearw = 0xfabecafe;
        if (*arearw != *areaex)
                err_quit("memory regions are not shared");

        printf("pass\n");
        return 0;
}
