#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define BUF_SIZE 1024
#define TEN_MB 10000000

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>
#include <Windows.h>
#include <string.h>
#include<time.h>
#include<assert.h>
#pragma comment(lib, "ws2_32.lib")

void error_handle(char* message);
void input();
int check_sender();
void subf();

HANDLE hThread;
UINT threadID;

WSADATA wsa_data;
SOCKET sock;
SOCKADDR_IN send_addr;

char send_buf[BUF_SIZE], rcv_buf[BUF_SIZE];
int nbyte;
int len, rcv_buf_size; double loss_prob;

int main(void)
{
	input();

	// 소켓 라이브러리 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		error_handle("WSAStartup() errer!");

	// 소켓 생성
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
		error_handle("socket() error!");
	memset(&send_addr, 0, sizeof(send_addr));
	send_addr.sin_family = AF_INET;
	//send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	send_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	send_addr.sin_port = htons(12000);

	if (!check_sender())
	{
		printf("sender가 접속해 있지 않습니다. sender를 먼저 접속시켜주세요!\n");
		return 0;
	}

	//hThread = (HANDLE)_beginthreadex(NULL, 0, thread_func, 0, 0, &threadID);
	subf();

	closesocket(sock);
	WSACleanup();
	return 0;
}

void error_handle(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int check_sender()
{
	int addrlen = sizeof(send_addr);

	memcpy(send_buf, "connect", 8);
	sendto(sock, send_buf, 8, 0, (struct sockaddr*)&send_addr, sizeof(send_addr));
	nbyte = recvfrom(sock, rcv_buf, BUF_SIZE, 0, (struct sockaddr *)&send_addr, &addrlen);

	if (strcmp(rcv_buf, "accept"))	return 0;
	else return 1;
}

void input()
{
	printf("packet loss probability :");
	scanf("%lf", &loss_prob);

	printf("socket recv buffer size :");
	len = sizeof(rcv_buf_size);

	getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcv_buf_size, &len);

	printf("%d\n", rcv_buf_size);

	if (rcv_buf_size < TEN_MB)
	{
		rcv_buf_size = TEN_MB;
		setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcv_buf_size, sizeof(rcv_buf_size));
		printf("socket recv buffer size updated: %d\n", rcv_buf_size);
	}

	puts("");
}

void subf()
{
	int pkt = -1;
	int addrlen = sizeof(send_addr);
	int is_first = 1;
	float t, percent;
	char temp[BUF_SIZE];
	clock_t start;
	srand((unsigned)time(0));


	while (1)
	{
		nbyte = recvfrom(sock, rcv_buf, BUF_SIZE, 0, (struct sockaddr *)&send_addr, &addrlen);

		if (nbyte <= 0) continue;

		if (rcv_buf[nbyte - 1])  /* Do not printf unless it is terminated */
			printf("Received an unterminated string\n");
		else
		{
			int rcv_pkt = atoi(rcv_buf);
			if (is_first)
			{
				//assert(rcv_pkt == 0);
				start = clock();
				is_first = 0;
			}

			t = (clock() - start) / (float)1000;
			sprintf(temp, "%.3f pkt: %d Receiver < Sender\n", t, rcv_pkt);
			printf(temp);
			
			percent = rand() % 10000 / 100.f;
			if (percent < (loss_prob * 100) )
			{
				sprintf(temp, "%.3f pkt: %d | Dropped\n", t, rcv_pkt);
				printf(temp);
			}

			else
			{
				if (pkt + 1 == rcv_pkt)
				{
					sprintf(send_buf, "%d", rcv_pkt);
					sendto(sock, send_buf, strlen(send_buf) + 1, 0, (struct sockaddr*)&send_addr, sizeof(send_addr));

					t = (clock() - start) / (float)1000;
					sprintf(temp, "%.3f ACK: %d Receiver > Sender\n", t, rcv_pkt);
					printf(temp);
					pkt++;
				}

				else if (pkt + 1 < rcv_pkt)
				{
					sprintf(send_buf, "%d", pkt);
					sendto(sock, send_buf, strlen(send_buf)+1, 0, (struct sockaddr*)&send_addr, sizeof(send_addr));

					t = (clock() - start) / (float)1000;
					sprintf(temp, "%.3f ACK: %d Receiver > Sender\n", t, pkt);
					printf(temp);
				}
			
			}
	
		}
	}
}

