/*
 * Copyright (c) 2026
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static int
expect_errno(const char *name, long result, int expected)
{
    if (result == -1 && errno == expected)
        return 0;

    fprintf(stderr, "%s: expected errno %d, got result %ld errno %d\n",
            name, expected, result, errno);
    return 1;
}

int
main(void)
{
    cpu_set_t original;
    CPU_ZERO(&original);
    long mask_size = syscall(SYS_sched_getaffinity, 0,
                             sizeof(original), &original);
    if (mask_size <= 0 || !CPU_ISSET(0, &original)) {
        fprintf(stderr, "initial sched_getaffinity failed: result %ld "
                "errno %d\n", mask_size, errno);
        return 1;
    }

    cpu_set_t pinned;
    CPU_ZERO(&pinned);
    CPU_SET(0, &pinned);
    // CPUs outside the simulated system must be clipped from the policy.
    CPU_SET(CPU_SETSIZE - 1, &pinned);
    if (syscall(SYS_sched_setaffinity, 0, sizeof(pinned), &pinned) != 0) {
        perror("sched_setaffinity CPU 0");
        return 1;
    }

    cpu_set_t observed;
    CPU_ZERO(&observed);
    long observed_size = syscall(SYS_sched_getaffinity, 0,
                                 sizeof(observed), &observed);
    if (observed_size != mask_size || !CPU_ISSET(0, &observed)) {
        fprintf(stderr, "affinity round trip failed\n");
        return 1;
    }
    for (int cpu = 1; cpu < CPU_SETSIZE; ++cpu) {
        if (CPU_ISSET(cpu, &observed)) {
            fprintf(stderr, "unexpected CPU %d in returned mask\n", cpu);
            return 1;
        }
    }

    cpu_set_t empty;
    CPU_ZERO(&empty);
    errno = 0;
    if (expect_errno("empty mask",
                     syscall(SYS_sched_setaffinity, 0,
                             sizeof(empty), &empty), EINVAL)) {
        return 1;
    }

    errno = 0;
    if (expect_errno("negative pid",
                     syscall(SYS_sched_setaffinity, -1,
                             sizeof(pinned), &pinned), EINVAL)) {
        return 1;
    }

    errno = 0;
    if (expect_errno("unknown pid",
                     syscall(SYS_sched_setaffinity, 32767,
                             sizeof(pinned), &pinned), ESRCH)) {
        return 1;
    }

    errno = 0;
    if (expect_errno("bad mask pointer",
                     syscall(SYS_sched_setaffinity, 0, sizeof(pinned),
                             (void *)(uintptr_t)1), EFAULT)) {
        return 1;
    }

    errno = 0;
    if (expect_errno("short get mask",
                     syscall(SYS_sched_getaffinity, 0, 0, &observed),
                     EINVAL)) {
        return 1;
    }

    puts("sched affinity tests passed");
    return 0;
}
