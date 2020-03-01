#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <inttypes.h>

#include "UDP_Sender.h"

void sigHndl(int tmp)
{
    cont = 0;
}

uint16_t randNum(uint16_t min, uint16_t max)
{
    return (rand() % (max - min + 1)) + min;
}

// Calculates IP Header checksum.
uint16_t ip_csum(uint16_t *buf, size_t len)
{
    uint32_t sum;

    for (sum = 0; len > 0; len--)
    {
        sum += *buf++;
    }

    sum = (sum >> 16) + (sum & 0XFFFF);
    sum += (sum >> 16);

    return (uint16_t) ~sum;
}

// Calculates UDP Header checksum. Rewrote - https://gist.github.com/GreenRecycleBin/1273763
uint16_t udp_csum(struct udphdr *udphdr, size_t len, uint32_t srcAddr, uint32_t dstAddr)
{
    const uint16_t *buf = (const uint16_t*) udphdr;
    uint16_t *ip_src = (void *) &srcAddr, *ip_dst = (void *) &dstAddr;
    uint32_t sum;
    size_t length = len;

    sum = 0;

    while (len > 1)
    {
        sum += *buf++;

        if (sum & 0x80000000)
        {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }

        len -= 2;
    }

    if (len & 1)
    {
        sum += *((uint8_t *) buf);
    }

    // Pseudo header.
    sum += *(ip_src++);
    sum += *ip_src;

    sum += *(ip_dst++);
    sum += *ip_dst;

    sum += htons(IPPROTO_UDP);
    sum += htons(length);

    while (sum >> 16)
    {
        sum = (sum & 0XFFFF) + (sum >> 16);
    }

    return (uint16_t) ~sum;
}

void *connHndl(void *data)
{
    // Get passed data.
    struct connection *con = data;

    while (1)
    {
        // Initiate variables.
        unsigned char buffer[MAX_PCKT_LEN];

        struct iphdr *iphdr = (struct iphdr *) buffer;
        struct udphdr *udphdr = (struct udphdr *) (buffer + sizeof(struct iphdr));
        unsigned char *pcktData = (unsigned char *) (buffer + sizeof(struct iphdr) + sizeof(struct udphdr));

        // Set everything to 0.
        memset(buffer, 0, MAX_PCKT_LEN);

        // Let's generate a random amount of data.
        uint16_t len = randNum(con->min, con->max);

        for (uint16_t i = 1; i <= len; i++)
        {
            *pcktData++ = randNum(0, 255);
        }

        uint16_t port = randNum(10000, 60000);
            
        // Initiate socket variables.
        struct sockaddr_in sin;

        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = inet_addr(con->dIP);
        memset(&sin.sin_zero, 0, sizeof(sin.sin_zero));

        // Fill out IP and UDP headers.
        iphdr->ihl = 5;
        iphdr->frag_off = 0;
        iphdr->version = 4;
        iphdr->protocol = IPPROTO_UDP;
        iphdr->tos = 16;
        iphdr->ttl = 64;
        iphdr->id = 0;
        iphdr->check = 0;
        iphdr->saddr = inet_addr(con->sIP);
        iphdr->daddr = inet_addr(con->dIP);
        iphdr->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + len;

        udphdr->uh_dport = htons(con->dPort);
        udphdr->uh_sport = htons(port);
        udphdr->len = htons(sizeof(struct udphdr) + len);
        udphdr->check = udp_csum(udphdr, sizeof(struct udphdr) + len, iphdr->saddr, iphdr->daddr);

        iphdr->check = ip_csum((uint16_t *)buffer, sizeof(struct iphdr));

        //fprintf(stdout, "Attempting to send packet to %s:%u from %s:%u with payload length %" PRIu16 " on sock " PRIu8 ". IP Hdr Length is %" PRIu16 " and UDP Hdr length is %" PRIu16 ".\n", con->dIP, con->dPort, con->sIP, port, len, con->sockfd, iphdr->tot_len, udphdr->len);
        
        uint16_t dataSent;

        // Send the packet.
        if ((dataSent = sendto(con->sockfd, buffer, iphdr->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin))) < 0)
        {
            fprintf(stdout, "Failed to send a packet. Error - %s (Sock FD is %" PRIu8 ")\n", strerror(errno), con->sockfd);
        }

        // Wait.
        if (con->time > 0)
        {
            usleep(con->time);
        }
    }

    // Close thread.
    pthread_exit(NULL);
}

int main(uint8_t argc, char *argv[])
{
    // Check argument count.
    if (argc < 4)
    {
        fprintf(stdout, "Usage: %s <Source IP> <Destination IP> <Destination IP> [<Max> <Min> <Interval> <Thread Count>]", argv[0]);

        exit(0);
    }

    // Create connection struct and fill out with argument data.
    struct connection con;

    con.sIP = argv[1];
    con.dIP = argv[2];
    con.dPort = atoi(argv[3]);
    con.min = 1000;
    con.max = 60000;
    con.time = 100000;
    con.threads = 1;

    // Min argument (optional).
    if (argc > 4)
    {
        con.min = atoi(argv[4]);
    }

    // Max argument (optional).
    if (argc > 5)
    {
        con.max = atoi(argv[5]);
    }

    // Interval argument (optional).
    if (argc > 6)
    {
        con.time = strtoul(argv[6], NULL, 10);
    }

    // Threads argument (optional).
    if (argc > 7)
    {
        con.threads = atoi(argv[7]);
    }

    // Create socket.
    int8_t sockfd;
    int8_t one = 1;

    sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);

    // Check for socket error.
    if (sockfd <= 0)
    {
        fprintf(stderr, "Socket() Error - %s\n", strerror(errno));
        perror("socket");

        exit(1);
    }

    // Assign sockfd.
    con.sockfd = sockfd;

    // Set socket option that tells the socket we want to send our own headers.
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
    {
        fprintf(stderr, "SetSockOpt() Error - %s\n", strerror(errno));
        perror("setsockopt");

        // Close socket.
        close(sockfd);

        exit(1);
    }

    // Signal.
    signal(SIGINT, sigHndl);

    // Create threads.
    for (uint16_t i = 1; i <= con.threads; i++)
    {
        // Create thread to handle packets.
        pthread_t tid;

        pthread_create(&tid, NULL, connHndl, (void *)&con);

        fprintf(stdout, "Spawned thread #%" PRIu16 "...\n", i);
    }

    // Prevent program from closing.
    while (cont)
    {
        
    }

    // Close socket.
    close(sockfd);

    // Close the program successfully.
    exit(0);
}