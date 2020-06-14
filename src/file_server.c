#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 1024
void file_list(FILE* pip, char* path, char* path_temp); // 현 폴더에 존재하는 파일/디렉토리 표시
void error_handling(char *message);

int main(int argc, char *argv[])
{
	int serv_sd, clnt_sd;
	FILE *fp;
	char buf[BUF_SIZE];
	char message[BUF_SIZE] = {0, };

	FILE *pip;
	int read_cnt = 0, f_size = 0;
	// pipe의 결과를 문자열로 저장, pipe 출력 임시 저장 문자열
	char path[BUF_SIZE] = {0, }, path_temp[BUF_SIZE] = {0, };

	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;

	if(argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sd = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	bind(serv_sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
	listen(serv_sd, 5);

	clnt_adr_sz = sizeof(clnt_adr);
	clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

	*(int *) message = 1;

	do {

		switch ((int) message[0]) {

			case 1: /* init */

				*(int *) path = 1; // opcode 1: init
				file_list(pip, path, path_temp);
				write(clnt_sd, path, BUF_SIZE);

				path[0] = '\0';
				path_temp[0] = '\0';
				break;

			case 2: /* Reaction */

				printf("Request file: %s", message+1);
				message[strlen(message)-1] = '\0'; // 문자열 리턴 제거

				if (access(message+1, F_OK) == 0) {
					// 사용자로부터 받은 입력: 파일 명
					// 파일이 존재한다면 다운로드 실행

					*(int *) path = 3; // opcode 3: success

					fp = fopen(message+1, "rb");
					fseek(fp, 0, SEEK_END);
					f_size = ftell(fp);
					fseek(fp, 0, 0); // 파일 포인터 초기로

					*(int *) (path+1) = f_size;

					write(clnt_sd, path, BUF_SIZE);
					read(clnt_sd, buf, BUF_SIZE);

					if (!strcmp(buf, "Already")) {
						// 클라이언트가 이미 파일이 존재하다고 보내옴
						// 다시 파일목록을 보내주고 다운로드를 실행하지 않음

						*(int *) path = 2;
						file_list(pip, path, path_temp);
						write(clnt_sd, path, BUF_SIZE);

						path[0] = '\0';
						path_temp[0] = '\0';
						continue; // 다운로드 pass
					}

					printf("File Size = %d\n", f_size);
					printf("Message from client: %s \n", buf); // 클라이언트가 준비됨
					buf[0] = '\0';

					while (1)
					{
						// 다운로드 실행 1Byte씩 전송
						read_cnt = fread((void*)buf, 1, BUF_SIZE, fp);
						if(read_cnt < BUF_SIZE)
						{
							write(clnt_sd, buf, read_cnt);
							break;
						}
						write(clnt_sd, buf, BUF_SIZE);
					}

					// Half Close
					shutdown(clnt_sd, SHUT_WR);
					read(clnt_sd, buf, BUF_SIZE);
					printf("Message from client: %s \n", buf); // 클라이언트의 메세지 (성공)

					fclose(fp);
					close(clnt_sd); close(serv_sd);
				}
				else {
					// Error
					*(int *) path = 2; // opcode 2: Validation error
					file_list(pip, path, path_temp);
					write(clnt_sd, path, BUF_SIZE);

					path[0] = '\0';
					path_temp[0] = '\0';
				}

				break;
		}
		message[0] = '\n';
	} while((read_cnt = read(clnt_sd, message, BUF_SIZE)) != 0);

	return 0;
}

void file_list(FILE* pip, char* path, char* path_temp)
{
	/* 파일 목록을 보여주는 함수 */
	/* 리눅스, 유닉스 파이프 이용해 리턴된 결과를 저장 */
	pip = popen("ls -al", "r");

	while (fgets(path_temp, sizeof(path_temp), pip) != NULL) {
		strcat(path+1, path_temp);
	}

	pclose(pip);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
