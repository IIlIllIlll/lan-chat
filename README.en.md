

This project is a simple local area network (LAN) chat tool based on TCP, containing both client and server programs, written in C++. It allows clients to connect to the server and perform basic text communication.

## Features

- TCP communication supporting IPv4.
- Efficient I/O event handling using epoll.
- Basic terminal input processing capabilities.
- Separated text input and output section.

## File Structure

- `inputBuffer.h`: Provides terminal input handling functions.
- `lc_client.cpp`: The client program used to connect to the server and send/receive messages.
- `lc_server.cpp`: The server program that listens for client connections and manages communication.
- `simpleSocket.h`: Contains utility functions for creating and managing TCP sockets.

## Compilation and Execution

### Compilation

Ensure that a C++ compiler is installed (g++ for example). Use the following commands to compile:

```bash
g++ lc_server.cpp -o lc_server
g++ lc_client.cpp -o lc_client
```

### Running the Server

```bash
./lc_server [port_to_listen] [<optional>nickname]
```

### Running the Client

```bash
./lc_client [destination_ip] [destination_port] [<optional>nickname]
```

The client will attempt to connect to the server.

## Usage Example

1. Start the server on one machine first.
2. Start the client on another machine and ensure both are on the same network.
3. Once connected, the client can input and send messages to the server via the terminal.

## Notes

- Tested in Linux only, dont support Windows yet (will support in future).
- If you sure destination_ip and port are both correct, but failed to connect, check server machine's firewall setting.
- This project is a simple example and does not implement advanced error handling or encryption features.