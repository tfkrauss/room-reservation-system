#ifndef SERVER_M_H
#define SERVER_M_H

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm> // for std::transform

//For assignment
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



constexpr auto LOOPBACK_ADDR = "127.0.0.1";
constexpr int PORT_UDP = 44896;
constexpr int PORT_TCP = 45896;
constexpr int BACKLOG = 10;

//Login verification statuses returned from verifyLogin. Distinguish between guest and member as well
static constexpr int GUEST = 0;
static constexpr int MEMBER = 1;
static constexpr int BAD_USERNAME = -1;
static constexpr int BAD_PASSWORD = -2;

//Potential query responses to client
constexpr uint32_t ERROR = 2;   //Error if a guest attempts to reserve a room
constexpr uint32_t NO_MATCH = 3;  //If room is unavaiable
constexpr uint32_t UNAVAILABLE = 0;  //If room is unavaiable
constexpr uint32_t SUCCESS = 1;      //If room is succesfully reserved for reservation. If room is available for availability query

struct RoomRequest {
    char requestType;   // A for availability, R for reservation
    char roomType;      // S, D, U
    uint32_t roomNumber;
};

class ServerM {
public:
    ServerM();
    ~ServerM();

    void createUDP();
    void createTCP();
    void configure();
    void receiveRoomLists();
    void handleClients();
    void initMembers();
    void printRooms();

private:
    std::unordered_map<std::string, std::string> loginInfo;
    std::unordered_set<int> singleRooms, doubleRooms, suiteRooms;
    int sockUDP = -1, sockTCP = -1;
    struct sockaddr_in serveraddrUDP, serveraddrTCP;
    struct sockaddr_in serverD, serverU, serverS;

    void handleGuest(const std::string& username, int client_fd);
    void handleMember(const std::string& username, const std::string& password, int client_fd);
    void sendVerificationStatus(uint32_t status, int client_fd);
    RoomRequest recvRoomRequest(int client_fd);
    int forwardQuery(RoomRequest request, int client_fd);
    void handleMemberResponse(RoomRequest request, int client_fd);
    void handleGuestResponse(RoomRequest request, int client_fd);
    int verifyLogin(const std::string& key, const std::string& val);
    char identifySender(struct sockaddr_in& srcAddr);
};

#endif // SERVER_M_H