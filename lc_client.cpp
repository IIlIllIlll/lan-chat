#include"simpleSocket.h"
#include"inputBuffer.h"
#include<stdio.h>
#include<string.h>
#include<sys/epoll.h>

int server_fd = -1;
int epfd = -1;

int main(int argc, char* argv[]){
    if(argc < 3){
        printf("please provide destination ip and port\nexample:\n    lc_client 127.0.0.1 16555 <optional>nick_name");
        return -1;
    }
    const int port = strtonum(argv[2], 5);
    if(port < 0 || port > 65535){
        printf("Bad port argument, please check\n");
        return -1;
    }
    char ip[16] = {0};
    snprintf(ip, 15, argv[1]);
    if(!is_valid_ipv4(ip)){
        printf("Bad ip format, please check.\n");
        return -1;
    }

    char nick_name[27] = "Unknown";
    char nick_name_server[27] = {0};
    int nick_name_len = 8;
    if(argc > 3){
        nick_name_len = strlen(argv[3]);
        if(nick_name_len > 26){
            printf("error: nick name too long. should be no more than 26 characters(13 wide characters)\n");
            return -1;
        }
        memset(nick_name, 0, 27);
        snprintf(nick_name, 26, "%s", argv[3]);
    } else {
        printf("no nick name set. will use Unknown as default\n");
    }

	printf("Trying to connect to %s:%d\n", ip, port);
    if(quick_client_setup_and_connect_tcp_ipv4(server_fd, ip, port) < 0){
		return -1;
	}
    if(send_data_to(server_fd, nick_name, nick_name_len) <= 0){
        printf("Failed to send your nick name to the other\n");
    }
    if(recv_data_from(server_fd, nick_name_server, 26) <= 0){
        printf("Failed to recieve nickname, use Unknown as default\n");
        snprintf(nick_name_server, 8, "Unknown");
    }

	printf("connected to %s(%s). loading...\n", nick_name_server, ip);


    epfd = epoll_create1(0);

    if(epfd == -1){
        printf("failed to create epoll\n");
        close(server_fd);
        return -1;
    }

    epoll_event stdin_event = {0};
    stdin_event.data.fd = 0;
    stdin_event.events = EPOLLIN;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &stdin_event) < 0){
        printf("failed to epoll stdin\n");
        close(server_fd);
        return -1;
    }

    epoll_event server_event = {0};
    server_event.data.fd = server_fd;
    server_event.events = EPOLLIN;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &server_event) < 0){
        printf("failed to epoll server\n");
        close(server_fd);
        return -1;
    }

    epoll_event events_buffer[2];

    char send_buffer[200] = {0};
    char recv_buffer[200] = {0};

	printf("Ready to chat!\nstarts below:\n\n");
    init_terminal();

    bool finished_typing = 0;
    bool start_typing = 0;
    int len = -1;
    while(1){
        int n = epoll_wait(epfd, events_buffer, 2 ,-1);
        if(n < 0){
            printf("epoll_wait error\n");
            goto exit;
        }

        for(int i = 0; i<n; i++){
            if(events_buffer[i].data.fd == server_fd){
				if(recv_data_from(server_fd, recv_buffer, 200) == 0){
					printf("%s disconnected, exiting\n", nick_name_server);
					goto exit;
				}

				printf("\033[2K\r> %s says:\n%s\n\n", nick_name_server, recv_buffer);
				memset(recv_buffer, 0, 200);
                if(start_typing && len != -1){
                    write(1, send_buffer, len);
                }
            }

			if(events_buffer[i].data.fd == 0){
                start_typing = 1;
				len = get_line(send_buffer, 200, finished_typing);
                if(finished_typing){
                    printf("\033[A\033[2K\r- %s (me):\n%s\n\n", nick_name, send_buffer);
                    if(send_data_to(server_fd, send_buffer, len) < 0){
                        printf("\n%s disconnected, exiting\n", nick_name_server);
                        goto exit;
                    }
                    memset(send_buffer, 0, 200);
                    start_typing = 0;
                    finished_typing = 0;
                }   
			}
        }
    }

exit:
    restore_terminal();
    close(server_fd);
}
