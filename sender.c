#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define BUF_SIZE 1024

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>
#include <Windows.h>
#include<time.h>
#include<assert.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")

unsigned WINAPI thread_func(void* para);
void error_handle(char* message);
int input();
int acks[10000000], packets[10000000];
HANDLE hThread;
UINT threadID;

WSADATA wsa_data;
SOCKET send_soc, recv_soc;
SOCKADDR_IN send_addr, recv_addr;

char send_buf[BUF_SIZE], rcv_buf[BUF_SIZE];
int w_size, pkt_num;
float t_out;
int is_connected, nbyte;
int pkt = 0, tpkt = 0;

clock_t start, timer;

int main(void)
{
	int addrlen = sizeof(recv_addr);
	char temp[BUF_SIZE];
	float t;

	// 소켓 라이브러리 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		error_handle("WSAStartup() error!");

	// 소켓 생성
	send_soc = socket(PF_INET, SOCK_DGRAM, 0);
	if (send_soc == INVALID_SOCKET)
		error_handle("socket() error!");

	// 어드레스 설정
	memset(&send_addr, 0, sizeof(send_addr));
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	send_addr.sin_port = htons(12000);

	if (bind(send_soc, (SOCKADDR*)&send_addr, sizeof(send_addr)) == SOCKET_ERROR)
		error_handle("bind() error!");

	hThread = (HANDLE)_beginthreadex(NULL, 0, thread_func, 0, 0, &threadID);

	if (input())
		return 0;

	printf("waiting for connection.. \n");
	while (!is_connected);
	printf("connected!\n\n");

	start = clock();
	timer = clock();
	tpkt = 0;
	while (!acks[pkt_num - 1])
	{
		t = (clock() - timer) / (float)1000;
		if (t > t_out)
		{
			sprintf(temp, "\n%.3f pkt: %d | Timeout since %.3f\n\n", (clock() - start) / (float)1000, tpkt, ((clock() - start) / (float)1000) - t);
			printf(temp);

			pkt = tpkt;
			timer = clock();
		}

		if (pkt < tpkt + w_size && pkt < pkt_num)
		{
			t = (clock() - start) / (float)1000;
			sprintf(temp, "%.3f pkt: %d Sender > Receiver", t, pkt); 
			if (packets[pkt]) strcat(temp, "<retransmitted>");
			
			printf("%s\n", temp);

			sprintf(send_buf, "%d\n", pkt);
			sendto(send_soc, send_buf, strlen(send_buf) + 1, 0, (struct sockaddr*)&recv_addr, sizeof(recv_addr));
			packets[pkt] = 1;
			pkt++;
		}
	}

	t = (clock() - start) / (float)1000;
	sprintf(temp, "%.3f | %d pkt trasmission completed. Throughput: %f pkts/sec\n", t, pkt_num, pkt_num / t);
	printf(temp);
	
	closesocket(send_soc);
	WSACleanup();
	return 0;
}

int input()
{
	char choice;

	printf("window size :");
	scanf("%d", &w_size);
	printf("Timeout(sec) :");
	scanf("%f", &t_out);
	printf("how many packets to send? :");
	scanf("%d", &pkt_num);
	printf("Do you want to start?(y/n) :");
	scanf(" %c", &choice);

	if (choice == 'n')
	{
		printf("You've chosen quit..\n");
		return 1;
	}


	return 0;
}

void error_handle(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


UINT WINAPI thread_func(void* para)
{
	int addrlen = sizeof(recv_addr);
	float t;
	char temp[BUF_SIZE];
	while (1)
	{
		nbyte = recvfrom(send_soc, rcv_buf, BUF_SIZE, 0, (struct sockaddr *)&recv_addr, &addrlen);
		if (!strcmp(rcv_buf, "connect"))
		{
			memcpy(send_buf, "accept", 7);
			if (sendto(send_soc, send_buf, 7, 0, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) < 0)
				error_handle("sendto() error!");
			memcpy(rcv_buf, ".", 2);
			is_connected = 1;
			continue;
		}

		if (nbyte <= 0) continue;

		if (rcv_buf[nbyte - 1])  /* Do not printf unless it is terminated */
			printf("Received an unterminated string\n");
		else
		{
			int rcv_ack = atoi(rcv_buf);
			if (rcv_ack == -1 || acks[rcv_ack])
			{
				t = (clock() - start) / (float)1000;
				sprintf(temp, "%.3f ACK: %d Sender < Receiver\n", t, rcv_ack);
				printf(temp);
			}

			else
			{
				acks[rcv_ack] = 1;

				t = (clock() - start) / (float)1000;
				sprintf(temp, "%.3f ACK: %d Sender < Receiver\n", t, rcv_ack);
				printf(temp);
				timer = clock();
				tpkt++;
			}
		}
	}
}