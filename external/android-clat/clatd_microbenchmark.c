/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * clatd_microbenchmark.c - micro-benchmark for clatd tun send path
 *
 * Run with:
 *
 * adb push {$ANDROID_PRODUCT_OUT,}/data/nativetest/clatd_microbenchmark/clatd_microbenchmark
 * adb shell /data/nativetest/clatd_microbenchmark/clatd_microbenchmark
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include "checksum.h"
#include "tun.h"

#define DEVICENAME "clat4"

#define PORT 51339
#define PAYLOADSIZE (1280 - sizeof(struct iphdr) - sizeof(struct udphdr))
#define NUMPACKETS 1000000
#define SEC_TO_NANOSEC (1000 * 1000 * 1000)

void init_sockaddr_in(struct sockaddr_in *sin, const char *addr) {
    sin->sin_family = AF_INET;
    sin->sin_port = 0;
    sin->sin_addr.s_addr = inet_addr(addr);
}

void die(const char *str) {
    perror(str);
    exit(1);
}

int setup_tun() {
    int fd = tun_open();
    if (fd == -1) die("tun_open");

    char dev[IFNAMSIZ] = DEVICENAME;
    int ret = tun_alloc(dev, fd);
    if (ret == -1) die("tun_alloc");
    struct ifreq ifr = {
        .ifr_name = DEVICENAME,
    };

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    init_sockaddr_in((struct sockaddr_in *) &ifr.ifr_addr, "192.0.0.4");
    if (ioctl(s, SIOCSIFADDR, &ifr) < 0) die("SIOCSIFADDR");
    init_sockaddr_in((struct sockaddr_in *) &ifr.ifr_addr, "255.255.255.248");
    if (ioctl(s, SIOCSIFNETMASK, &ifr) < 0) die("SIOCSIFNETMASK");
    if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0) die("SIOCGIFFLAGS");
    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
    if (ioctl(s, SIOCSIFFLAGS, &ifr) < 0) die("SIOCSIFFLAGS");
    return fd;
}

int send_packet(int fd, uint8_t payload[], int len, uint32_t payload_checksum) {
    struct tun_pi tun = { 0, htons(ETH_P_IP) };
    struct udphdr udp = {
        .source = htons(1234),
        .dest = htons(PORT),
        .len = htons(len + sizeof(udp)),
        .check = 0,
    };
    struct iphdr ip = {
        .version = 4,
        .ihl = 5,
        .tot_len = htons(len + sizeof(ip) + sizeof(udp)),
        .frag_off = htons(IP_DF),
        .ttl = 55,
        .protocol = IPPROTO_UDP,
        .saddr = htonl(0xc0000006),  // 192.0.0.6
        .daddr = htonl(0xc0000004),  // 192.0.0.4
    };
    clat_packet out = {
        { &tun, sizeof(tun) },  // tun header
        { &ip, sizeof(ip) },    // IP header
        { NULL, 0 },            // Fragment header
        { &udp, sizeof(udp) },  // Transport header
        { NULL, 0 },            // ICMP error IP header
        { NULL, 0 },            // ICMP error fragment header
        { NULL, 0 },            // ICMP error transport header
        { payload, len },       // Payload
    };

    ip.check = ip_checksum(&ip, sizeof(ip));

    uint32_t sum;
    sum = ipv4_pseudo_header_checksum(&ip, ntohs(udp.len));
    sum = ip_checksum_add(sum, &udp, sizeof(udp));
    sum += payload_checksum;
    udp.check = ip_checksum_finish(sum);

    return send_tun(fd, out, sizeof(out) / sizeof(out[0]));
}

double timedelta(const struct timespec tv1, const struct timespec tv2) {
    struct timespec end = tv2;
    if (end.tv_nsec < tv1.tv_nsec) {
        end.tv_sec -= 1;
        end.tv_nsec += SEC_TO_NANOSEC;
    }
    double seconds = (end.tv_sec - tv1.tv_sec);
    seconds += (((double) (end.tv_nsec - tv1.tv_nsec)) / SEC_TO_NANOSEC);
    return seconds;
}

void benchmark(const char *name, int fd, int s, int num, int do_read,
               uint8_t payload[], int len, uint32_t payload_sum) {
    int i;
    char buf[4096];
    struct timespec tv1, tv2;
    int write_err = 0, read_err = 0;
    clock_gettime(CLOCK_MONOTONIC, &tv1);
    for (i = 0; i < num; i++) {
        if (send_packet(fd, payload, len, payload_sum) == -1) write_err++;
        if (do_read && recvfrom(s, buf, sizeof(buf), 0, NULL, NULL) == -1) {
            read_err++;
            if (errno == ETIMEDOUT) {
                printf("Timed out after %d packets!\n", i);
                break;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tv2);
    double seconds = timedelta(tv1, tv2);
    int pps = (int) (i / seconds);
    double mbps = (i * PAYLOADSIZE / 1000000 * 8 / seconds);
    printf("%s: %d packets in %.2fs (%d pps, %.2f Mbps), ", name, i, seconds, pps, mbps);
    printf("read err %d (%.2f%%), write err %d (%.2f%%)\n",
           read_err, (float) read_err / i * 100,
           write_err, (float) write_err / i * 100);
}

int open_socket() {
    int sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);

    int on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) die("SO_REUSEADDR");

    struct timeval tv = { 1, 0 };
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) die("SO_RCVTIMEO");

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = ntohs(PORT),
        .sin_addr = { INADDR_ANY }
    };
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) die ("bind");

   return sock;
}

int main() {
    int fd = setup_tun();
    int sock = open_socket();

    int i;
    uint8_t payload[PAYLOADSIZE];
    for (i = 0; i < (int) sizeof(payload); i++) {
        payload[i] = (uint8_t) i;
    }
    uint32_t payload_sum = ip_checksum_add(0, payload, sizeof(payload));

    // Check things are working.
    char buf[4096];
    if (send_packet(fd, payload, sizeof(payload), payload_sum) == -1) die("send_packet");
    if (recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL) == -1) die("recvfrom");

    benchmark("Blocking", fd, sock, NUMPACKETS, 1, payload, sizeof(payload), payload_sum);
    close(fd);

    fd = setup_tun();
    set_nonblocking(fd);
    benchmark("No read", fd, sock, NUMPACKETS, 0, payload, sizeof(payload), payload_sum);
    close(fd);

    fd = setup_tun();
    set_nonblocking(fd);
    benchmark("Nonblocking", fd, sock, NUMPACKETS, 1, payload, sizeof(payload), payload_sum);
    close(fd);

    return 0;
}
