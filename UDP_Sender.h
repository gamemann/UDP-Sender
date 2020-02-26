#define MAX_PCKT_LEN 65535

extern int errno;
uint8_t cont = 1;

struct connection 
{
    char *sIP;
    char *dIP;
    uint16_t dPort;
    uint8_t sockfd;
    uint16_t max;
    uint16_t min;
    uint64_t time;
    uint16_t threads;
};