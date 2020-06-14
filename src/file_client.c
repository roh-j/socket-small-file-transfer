#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sd;
	FILE *fp;
	int i = 1, j = 0, k = 0;

	char buf[BUF_SIZE], path[BUF_SIZE], send[BUF_SIZE] = {0, };
	char f_path[30] = {0, }, f_down[30] = {0, };
	int read_cnt;
	int f_size, div_size; // 파일 크기, 1/10 크기 (진행 사항을 표시하기 위한 변수)

	struct sockaddr_in serv_adr;

	if(argc != 3) {
		printf("Usage: %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sd = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

	while (1) {
		read(sd, path, BUF_SIZE);

		if ((int) path[0] == 3) { // opcode 3: 유효성 검사 통과
			if (access(f_down, F_OK) != 0) {
				break;
			}
			else {
				write(sd, "Already", 10);
				continue;
			}
		}

		// 초기 설정, 파일 유효성 검사 실패시 서버에서 목록을 다시 보내줌.
		switch ((int) path[0]) {
			case 1: /* Init */
			case 2: /* Error */
				f_path[0] = '\0'; // 파일 경로 초기화
				*(int *) send = 2; // opcde 2: Reaction

				printf("%s\n", path+1); // 파일 리스트 표시

				if ((int) path[0] == 2)
					printf("Error : 파일명이 잘못되었거나 이미 파일이 존재합니다.\n");

				printf("Input : ");
				fgets(send+1, BUF_SIZE, stdin);
				strcpy(f_path, send+1);

				f_path[strlen(f_path)-1] = '\0';
				sprintf(f_down, "downloads/%s", f_path); // 파일 경로 저장

				write(sd, send, BUF_SIZE);
				send[0] = '\0';
				break;
		}
	}

	fp = fopen(f_down, "wb");
	printf("Path: %s\n", f_down);

	f_size = *(int *) (path+1); // 서버에서 받은 데이터 중 크기
	div_size = f_size / 10;

	printf("Data: %d\n", f_size);
	printf("Download Start!\n");

	write(sd, "Init", 10);

	while (i <= f_size)
	{
		read_cnt = read(sd, buf, 1);
		fwrite((void*)buf, 1, read_cnt, fp);

		// 퍼센트와 진행사항 표시
		if(i % div_size == 0)
		{
			printf("\n");
			for (j = 0; j < (i / div_size); j++)
			{
				printf("■ ");
			}
			for (k = 0; k < (10 - i / div_size); k++)
			{
				printf("□ ");
			}
			// 소수점 자리의 표현 오차로 인해 근사값이 나옴
			printf("%.2f %%\n", (float) i / (float) f_size * 100);
		}

		if (i == f_size)
			printf("\n■ ■ ■ ■ ■ ■ ■ ■ ■ ■ 100 %%\n"); // 오차로 인한 오해를 줄이기 위해 100 % 표시

		i++;
	}

	puts("\nReceived file data");
	write(sd, "Thank you", 10);
	fclose(fp);
	close(sd);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
