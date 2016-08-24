/*
 * Copyright (c) 2012 The Chromium OS Authors.
 *
 * Based on:
 * http://bazaar.launchpad.net/~ubuntu-bugcontrol/qa-regression-testing/master/view/head:/scripts/kernel-security/ptrace/thread-prctl.c
 * Copyright 2011 Canonical, Ltd
 * License: GPLv3
 * Author: Kees Cook <kees.cook@canonical.com>
 *
 * Based on reproducer written by Philippe Waroquiers in:
 * https://launchpad.net/bugs/729839
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/ptrace.h>
#include <sys/prctl.h>
#ifndef PR_SET_PTRACER
# define PR_SET_PTRACER 0x59616d61
#endif

int tracee_method = 0;
#define TRACEE_FORKS_FROM_TRACER        0
#define TRACEE_CALLS_PRCTL_FROM_MAIN    1
#define TRACEE_CALLS_PRCTL_FROM_THREAD  2

/* Define some distinct exit values to aid failure debugging. */
#define EXIT_FORK_TRACEE             1
#define EXIT_FORK_TRACER             2
#define EXIT_PIPE_COMMUNICATION      3
#define EXIT_PIPE_NOTIFICATION       4
#define EXIT_TRACEE_PIPE_READ        5
#define EXIT_TRACEE_UNREACHABLE      6
#define EXIT_TRACER_PIPE_READ        7
#define EXIT_TRACER_PTRACE_ATTACH    8
#define EXIT_TRACER_PTRACE_CONTINUE  9
#define EXIT_TRACER_UNREACHABLE     10

int main_does_ptrace = 0;

int ret;
int pipes[2];
int notification[2];
pid_t tracer, tracee;

static void *thr_fn(void *v)
{
    printf("tracee thread started\n");
    if (tracee_method == TRACEE_CALLS_PRCTL_FROM_THREAD) {
        ret = prctl (PR_SET_PTRACER, tracer, 0, 0, 0);
        printf("tracee thread prtctl result: %d\n", ret);
    }
    printf("tracee thread finishing\n");
    return NULL;
}

void start_tracee(void);

void * tracer_main(void * data)
{
    long ptrace_result;
    char buf[8];
    int saw;

    tracer = getpid();
    printf("tracer %d waiting\n", tracer);

    if (tracee_method == TRACEE_FORKS_FROM_TRACER) {
        printf("forking tracee from tracer\n");
        start_tracee();
    }

    close(pipes[1]);
    close(notification[0]);
    close(notification[1]);

    saw = read(pipes[0], buf, 3);
    if (saw < 3) {
        perror("tracer pipe read");
        exit(EXIT_TRACER_PIPE_READ);
    }

    printf("tracer to PTRACE_ATTACH my tracee %d\n", tracee);
    ptrace_result = ptrace(PTRACE_ATTACH, tracee, NULL, NULL);
    if (ptrace_result != 0) {
        fflush(NULL);
        perror ("tracer ptrace attach has failed");
        exit(EXIT_TRACER_PTRACE_ATTACH);
    }
    printf ("tracer ptrace attach successful\n");

    /* Wait for signal. */
    printf("tracer waiting for tracee to SIGSTOP\n");
    waitpid(tracee, NULL, 0);

    printf("tracer to PTRACE_CONT tracee\n");
    ptrace_result = ptrace(PTRACE_CONT, tracee, NULL, NULL);
    if (ptrace_result != 0) {
        fflush(NULL);
        perror ("tracer ptrace continue has failed");
        exit(EXIT_TRACER_PTRACE_CONTINUE);
    }
    printf ("tracer ptrace continue successful\n");

    printf("tracer returning 0\n");
    fflush(NULL);
    exit(EXIT_SUCCESS);

    return NULL;
}

/* Tracee knows nothing, needs tracee and tracer pid. */
void tracee_main(void) {
    char buf[1024];
    int saw;
    pthread_t thr;

    tracee = getpid();
    close(pipes[0]);

    printf("tracee %d reading tracer pid\n", tracee);
    close(notification[1]);
    saw = read(notification[0], buf, 1024);
    if (saw < 1) {
        perror("pipe read");
        exit(EXIT_TRACEE_PIPE_READ);
    }
    buf[saw]='\0';
    tracer = atoi(buf);

    printf("tracee %d started (expecting %d as tracer)\n", tracee, tracer);

    /* Handle setting PR_SET_PTRACER. */
    switch (tracee_method) {
        case TRACEE_CALLS_PRCTL_FROM_MAIN:
            ret = prctl (PR_SET_PTRACER, tracer, 0, 0, 0);
            printf("tracee main prtctl result: %d \n", ret);
            break;
        case TRACEE_CALLS_PRCTL_FROM_THREAD:
            printf("tracee thread starting\n");
            pthread_create(&thr, NULL, thr_fn, NULL);
            pthread_join(thr, NULL);
            printf("tracee thread finished\n");
            break;
        default:
            break;
    }

    /* Wait for Oedipal action. */
    printf("tracee triggering tracer\n");
    fflush(NULL);
    write(pipes[1], "ok\n", 3);

    printf("tracee waiting for master\n");
    saw = read(notification[0], buf, 1024);
    buf[saw] = '\0';

    printf("tracee finished (%s)\n", buf);
    exit(EXIT_SUCCESS);
}

void start_tracee(void)
{
    fflush(NULL);
    tracee = fork();
    if (tracee < 0) {
        perror("fork tracee");
        exit(EXIT_FORK_TRACEE);
    }
    if (tracee == 0) {
        tracee_main();
        exit(EXIT_TRACEE_UNREACHABLE);
    }
}

/* Tracer knows tracee, needs tracer pid. */
int main(int argc, char*argv[])
{
    int status;
    char buf[1024];

    if (argc > 1) {
        /* Operational states:
         * 0: tracer forks tracee.
         * 1: tracee calls prctl from main process.
         * 2: tracee calls prctl from non-leader thread.
         */
        tracee_method = atoi(argv[1]);
    }
    if (argc > 2) {
        /* Operational states:
         * 0: ptrace happens from non-leader thread.
         * 1: ptrace happens from main process.
         */
        main_does_ptrace = atoi(argv[2]) != 0;
    }

    if (tracee_method != TRACEE_FORKS_FROM_TRACER) {
        printf("will issue prctl from %s\n",
               tracee_method == TRACEE_CALLS_PRCTL_FROM_MAIN ?
                    "main" : "thread");
    }
    else {
        printf("will fork tracee from tracer\n");
    }
    printf("will issue ptrace from tracer %s\n",
           main_does_ptrace ? "main" : "thread");

    printf("master is %d\n", getpid());

    if (pipe(notification)<0) {
        perror("pipe");
        exit(EXIT_PIPE_NOTIFICATION);
    }
    if (pipe(pipes)<0) {
        perror("pipe");
        exit(EXIT_PIPE_COMMUNICATION);
    }

    if (tracee_method != TRACEE_FORKS_FROM_TRACER) {
        printf("forking tracee from master\n");
        start_tracee();
    }

    fflush(NULL);
    tracer = fork();
    if (tracer < 0) {
        perror("fork tracer");
        exit(EXIT_FORK_TRACER);
    }
    if (tracer == 0) {
        printf("tracer is %d\n", getpid());
        if (main_does_ptrace) {
            tracer_main(NULL);
        }
        else {
            pthread_t thread;
            pthread_create(&thread, NULL, tracer_main, NULL);
            pthread_join(thread, NULL);
        }
        exit(EXIT_TRACER_UNREACHABLE);
    }

    /* Leave the pipes for the tracee and tracer. */
    close(pipes[0]);
    close(pipes[1]);

    /* Close our end of pid notification. */
    close(notification[0]);
    sprintf(buf, "%d", tracer);
    write(notification[1], buf, strlen(buf));

    printf("master waiting for tracer to finish\n");
    fflush(NULL);
    waitpid(tracer, &status, 0);

    printf("master waiting for tracee to finish\n");
    fflush(NULL);
    write(notification[1], "stop", 4);
    kill(tracee, SIGCONT); // Just in case.
    waitpid(tracee, NULL, 0);

    status = WEXITSTATUS(status);
    printf("master saw rc %d from tracer\n", status);
    return status;
}

