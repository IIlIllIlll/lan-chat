#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>

unsigned int lenofstr(const char* str){
    unsigned int len = 0;
    const char* p = str;
    for(; *p != 0; p++, len++);
    return len;
}

void err(const char* msg){
    unsigned int len = lenofstr(msg); 
    write(2, msg, len);
}

void ntostr(int n, char* buffer, int buffersize){
    int x = 0;
    int i = 0;
    for(int j = buffersize - 1; i<buffersize -1 && j > -1; j--){
        int k = 10*j;
        if(n/k > 0){
            buffer[i] = n/k - x + 48;
            x = n/k*10;
            i++;
        }
    }
    buffer[i] = 0;
}

int create_tcp_socket_ipv4(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        write(2, "socket() failed\n", 17);
    return sockfd;
}

int bind_addr_to_tcp_socket_ipv4(int& sockfd, int port){
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if(bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0){
        write(2, "bind() failed\n", 15);
        return -1;
    }
    return 0;
}

int listen_socket(int& sockfd, int max_connections){
    if(listen(sockfd, max_connections) < 0){
        write(2, "listen() failed\n", 17);
        return -1;
    }
    return 0;
}

int connect_to_tcp_socket_ipv4_sever(int& destination_server_sockfd, const char* ip, int port){
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, ip, &addr.sin_addr) < 1){
        write(1, "Invalid ip presentation\n", 24);
        return -1;
    }
    addr.sin_port = htons(port);
    if(connect(destination_server_sockfd, (sockaddr*)&addr, sizeof(addr)) < 0){
        write(2, "connect() failed\n" ,18);
        return -1;
    }
    return 0;
}

// accept()
int get_client_socketfd(int& server_sockfd, sockaddr_in* client_info_buffer, unsigned int* client_addr_len){
    int clientfd = accept(server_sockfd, (sockaddr*)client_info_buffer, client_addr_len);
    if(clientfd < 0)
        write(2, "accept() failed\n", 17);
    return clientfd;
}

int send_data_to(int& sockfd, const void* buffer, int len){
    int n = send(sockfd, buffer, len, 0);
    if(n < 0)
        write(2, "send() failed\n", 15);
    return n;
}

int recv_data_from(int& sockfd, void* recieve_buffer, int buffer_size){
    int n = recv(sockfd, recieve_buffer, buffer_size, 0);
    if(n < 0)
        write(2, "recv() failed\n", 15);
    return n;
}

// send the data in send_buffer to sockfd, put recieved data in recieve buffer from sockfd
int send_and_recv(int sockfd, void* send_buffer, int send_len, void* recv_buffer, int recv_buffer_size){
    if(send(sockfd, send_buffer, send_len, 0) < 0){
        write(2, "send() failed\n", 15);
        return -1;
    }

    if(recv(sockfd, recv_buffer, recv_buffer_size, 0) < 0){
        write(2, "recv() failed\n", 15);
        return -1;
    }

    return 0;
}

// send the data in buffer to sockfd, clear the buffer and use it for recieving data from sockfd
int send_n_recv(int& sockfd, void* buffer, int buffer_size, int send_len){
    if(send(sockfd, buffer, send_len, 0) < 0){
        write(2, "send() failed\n", 15);
        return -1;
    }

    memset(buffer, 0, buffer_size);

    if(recv(sockfd, buffer, buffer_size, 0) < 0){
        write(2, "recv() failed\n", 15);
        return -1;
    }

    return 0;
}

void close_socket(int& sockfd){
    if(sockfd != -1){
        close(sockfd);
        sockfd = -1;
    }
}

int quick_setup_server_tcp_ipv4(int& server_fd_empty, int& client_fd_empty, int port, int max_connection){
    server_fd_empty = create_tcp_socket_ipv4();
    if(server_fd_empty < 0){
        return -1;
    }
    if(bind_addr_to_tcp_socket_ipv4(server_fd_empty, port) < 0){
        close(server_fd_empty);
        err("failed to bind port ");
        char port_info[6] = {0};
        ntostr(port, port_info, 6);
        int len = lenofstr(port_info);
        write(2, port_info, len);
        write(2, "\n", 1);
        return -1;
    }
    if(listen(server_fd_empty, max_connection) < 0){
        close(server_fd_empty);
        return -1;
    }
	write(1, "Server started. Waiting for connection\n", 39);
    client_fd_empty = get_client_socketfd(server_fd_empty, 0, 0);
    if(client_fd_empty < 0){
        close(server_fd_empty);
        return -1;
    }
    return 0;
}

int quick_client_setup_and_connect_tcp_ipv4(int& dest_server_fd_empty, const char* server_ip, int server_port){
    dest_server_fd_empty = create_tcp_socket_ipv4();
    if(dest_server_fd_empty < 0){
        return -1;
    }
    if(connect_to_tcp_socket_ipv4_sever(dest_server_fd_empty, server_ip, server_port) < 0){
        close(dest_server_fd_empty);
        return -1;
    }
    return 0;
}

int quick_setup_server_tcp_ipv4_with_client_info(int& server_fd_empty, int& client_fd_empty, int port, sockaddr_in& client_info, int max_connection){
    unsigned int x = 0;
    server_fd_empty = create_tcp_socket_ipv4();
    if(server_fd_empty < 0){
        return -1;
    }
    if(bind_addr_to_tcp_socket_ipv4(server_fd_empty, port) < 0){
        close(server_fd_empty);
        err("failed to bind port ");
        char port_info[6] = {0};
        ntostr(port, port_info, 6);
        int len = lenofstr(port_info);
        write(2, port_info, len);
        write(2, "\n", 1);
        return -1;
    }
    if(listen(server_fd_empty, max_connection) < 0){
        close(server_fd_empty);
        return -1;
    }
	write(1, "Server started. Waiting for connection\n", 40);
    client_fd_empty = get_client_socketfd(server_fd_empty, &client_info, &x);
    if(client_fd_empty < 0){
        close(server_fd_empty);
        return -1;
    }
    return 0;
}
