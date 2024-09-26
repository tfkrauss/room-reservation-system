#ifndef BACKENDSERVER_H
#define BACKENDSERVER_H

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
//Needed for assignment statement
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>


//Query responses to client
constexpr uint32_t ERROR = 2;   //Error if a guest attempts to reserve a room
constexpr uint32_t NO_MATCH = 3;  //If room is not in the map
constexpr uint32_t UNAVAILABLE = 0;  //If room is unavaiable
constexpr uint32_t SUCCESS = 1;      //If room is succesfully reserved for reservation. If room is available for availability query

class backendserver {

public:
    backendserver(const std::string& fileName, int port, char serverDescriptor);
    ~backendserver();

    void sendRooms();
    void handleQueries();

private:
    struct RoomRequest {
        char requestType;   // A for availability, R for reservation
        char roomType;      // S, D, U
        uint32_t roomNumber;
    };

    std::unordered_map<int, int> roomAvailability;
    int sockfd;
    char serverType;
    struct sockaddr_in serveraddr;
    struct sockaddr_in serverM;

    void initialize(const std::string& fileName);
    void bootUp(int port);
    void reserveReq(RoomRequest request);
    void availabilityReq(RoomRequest request);
};

#endif // BACKENDSERVER_H