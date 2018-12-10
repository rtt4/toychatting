#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 1024
void error_handling(char *message);
void read_routine(int sock, char *buf);
void write_routine(int sock, char *buf, char* name);

int main(int argc, char *argv[])
{
	int sock;
	char message[BUF_SIZE];
	int str_len;
	struct sockaddr_in serv_adr;

	// 멀티플렉싱을 위한 변수 추가
	struct timeval timeout;
	fd_set fds, cpy_fds;
	int fd_max, fd_num, i;

	if(argc!=4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}

	sock=socket(PF_INET, SOCK_STREAM, 0);
	if(sock==-1)
		error_handling("socket() error");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error!");
	// else
	// 	puts("Connected...........");

	// 멀티플렉싱을 위한 fd_set 초기화 및 관찰 대상 추가
	FD_ZERO(&fds);
	FD_SET(0, &fds);		//	채팅 메세지 입력 감지를 위해 stdin을 등록
	FD_SET(sock, &fds);	//	서버와의 연결 감지를 위해 sock 등록
	fd_max = sock;

	while(1)
	{
		// 원본 fd_set으로 초기화함과 동시에, timeout interval 또한 갱신.
		cpy_fds = fds;
		timeout.tv_sec = 5;
		timeout.tv_usec = 5000;

		if( (fd_num=select(fd_max+1, &cpy_fds, 0, 0, &timeout)) == -1)
			break;
		if(fd_num==0)
			continue;	// timeout 발생

		if(FD_ISSET(sock, &cpy_fds))		// 서버로부터 수신할 데이터가 존재
			read_routine(sock, message);
		if(FD_ISSET(0, &cpy_fds))				// 키보드 입력한 데이터가 있을 경우
			write_routine(sock, message, argv[3]);	// 사용자 이름도 같이 전달해서 메세지 구성
	}

	close(sock);
	return 0;
}

void read_routine(int sock, char *buf)
{
	int str_len=read(sock, buf, BUF_SIZE);
	if(str_len==0)
		return;

	buf[str_len]=0;
	printf("%s", buf);
}

void write_routine(int sock, char *buf, char *name)
{
	char newBuf[BUF_SIZE * 2];		// 이름 정보까지 더할 새로운 버퍼를 마련
	fgets(buf, BUF_SIZE, stdin);
	if(!strcmp(buf,"q\n") || !strcmp(buf,"Q\n"))
	{
		close(sock);
		return;
	}
	sprintf(newBuf, "[%s] : %s", name, buf);	// 새로운 메세지를 구성
	write(sock, newBuf, strlen(newBuf));
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
