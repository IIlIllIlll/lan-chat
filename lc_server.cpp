#include"simpleSocket.h"
#include"inputBuffer.h"
#include<stdio.h>
#include<string.h>
#include<sys/epoll.h>


const int max_conn = 3;

int server_fd = -1;
int client_fd = -1;
int epfd = -1;

int main(int argc, char* argv[]){
    if(argc == 1){
        printf("please provide port you want to use\nexample:\n    lc_server 16555 <optional>nick_name\n");
        return -1;
    }

    const int port = strtonum(argv[1], 5);
    if(port < 0 || port > 65535){
        printf("Bad port argument, please check\n");
        return -1;
    }

    char nick_name[27] = "Unknown";
    char nick_name_client[27] = {0};
    int nick_name_len = 8;
    char c_ip[16] = {0};

    if(argc > 2){
        nick_name_len = strlen(argv[2]);
        if(nick_name_len > 26){
            printf("error: nick name too long. should be no more than 26 characters(8 wide characters)\n");
            return -1;
        }
        memset(nick_name, 0, 27);
        snprintf(nick_name, 26, "%s", argv[2]);
    } else {
        printf("no nick name set. will use Unknown as default\n");
    }

	printf("Starting server...\n");
    sockaddr_in client_info = {0};
    if(quick_setup_server_tcp_ipv4_with_client_info(server_fd, client_fd, port, client_info, max_conn) < 0){
        return -1;
    }
    if(inet_ntop(AF_INET, &client_info.sin_addr, c_ip, 16) == NULL){
        printf("WARN: Failed to parse client's ip\n");
    }
    printf("%s connected.\n", c_ip);
    printf("Proccessing nick name...\n");


    if(recv_data_from(client_fd, nick_name_client, 26) <= 0){
        printf("Failed to recieve nickname, use Unknown as default\n");
        snprintf(nick_name_client, 8, "Unknown");
    }

    if(send_data_to(client_fd, nick_name, nick_name_len) < 0){
        printf("Failed to send your nick name to the other\n");
    }

    printf("the other's nickname is %s.\n", nick_name_client);

    epfd = epoll_create1(0);

    if(epfd == -1){
        printf("failed to create epoll\n");
        close(server_fd);
        close(client_fd);
        return -1;
    }

    epoll_event stdin_event = {0};
    stdin_event.data.fd = fileno(stdin);
    stdin_event.events = EPOLLIN;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &stdin_event) < 0){
        printf("failed to epoll stdin\n");
        close(server_fd);
        close(client_fd);
        return -1;
    }

    epoll_event client_event = {0};
    client_event.data.fd = client_fd;
    client_event.events = EPOLLIN;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_event) < 0){
        printf("failed to epoll client\n");
        close(server_fd);
        close(client_fd);
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
            if(events_buffer[i].data.fd == client_fd){
				if(recv_data_from(client_fd, recv_buffer, 200) == 0){
					printf("%s(%s) disconnected, exiting\n", nick_name_client, c_ip);
					goto exit;
				}

				printf("\033[2K\r> %s(%s) says:\n%s\n\n", nick_name_client, c_ip, recv_buffer);
				memset(recv_buffer, 0, 200);
                if(start_typing && len != -1){
                    write(1, send_buffer, len);
                }
            }

			if(events_buffer[i].data.fd == 0){
                start_typing = 1;
				len = get_line(send_buffer, 200, finished_typing);
                if(finished_typing){
                    printf("\033[A\033[2K\r- %s (me):\n%s\n\n\n", nick_name, send_buffer);
                    if(send_data_to(client_fd, send_buffer, len) < 0){
                        printf("%s(%s) disconnected, exiting\n", nick_name_client, c_ip);
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
    close(client_fd);
}
