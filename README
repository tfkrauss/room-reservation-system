# Multi-Server Room Management System

This project is a multi-server room management system implemented using UDP communication in C++. It demonstrates server-to-server communication, data handling, and a client-server architecture. The system involves a main server (`serverM`) and three backend servers (`serverD`, `serverU`, and `serverS`), each responsible for managing specific room types.

## Features

- **Distributed Server Architecture**: A main server (`serverM`) communicates with backend servers (`serverD`, `serverU`, and `serverS`) using UDP sockets.
- **Dynamic Room Management**: Each backend server manages the availability of specific room types:
  - `serverD`: Double rooms
  - `serverU`: Suite rooms
  - `serverS`: Single rooms
- **Data Synchronization**: Backend servers send their room data to the main server upon initialization.
- **Efficient Data Transmission**: Room availability data is transmitted in network byte order for cross-platform compatibility.
- **Scalable Design**: Modular code allows for the addition of more server types with minimal changes.


## How It Works

1. **Initialization**:
   - The main server (`serverM`) initializes and begins listening for incoming UDP messages.
   - Backend servers (`serverD`, `serverU`, `serverS`) initialize, read room data from predefined files, and send this data to the main server.

2. **Room Data Transmission**:
   - Room availability data is serialized into `uint32_t` format, converted to network byte order, and sent over UDP.

3. **Data Reception**:
   - The main server receives data from each backend server, identifies the sender using IP and port, and updates its corresponding room type storage.


## Future Improvements

- Implement secure communication using TLS or other encryption techniques.
- Extend the system to support dynamic client queries and booking features.
- Optimize data serialization for larger-scale room datasets.
