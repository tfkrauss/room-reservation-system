#ifndef CLIENT_H
#define CLIENT_H

#include <cstdint>
#include <algorithm> // for std::transform
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <unordered_set>
//For assignment:
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




#define LOOPBACK_ADDR "127.0.0.1"
#define SERVERM_PORT 45896

// Login verification statusses
constexpr int GUEST = 0;
constexpr int MEMBER = 1;
constexpr int BAD_USERNAME = -1;
constexpr int BAD_PASSWORD = -2;

// Query responses
constexpr uint32_t ERROR = 2;
constexpr uint32_t NO_MATCH = 3;
constexpr uint32_t BAD_ROOMTYPE = 4;
constexpr uint32_t UNAVAILABLE = 0;
constexpr uint32_t SUCCESS = 1;

struct RoomRequest {
    char requestType;   // A for availability, R for reservation
    char roomType;      // S, D, U
    uint32_t roomNumber;
};

class client {
public:
    client();
    ~client();

    void createClient();
    void configure();
    void connectServer();
    void setLocalPort();
    void login();
    void handleReqs();
    void availabilityReq(RoomRequest request);
    void reservationReq(RoomRequest request);

private:
    int sockfd;
    int localPort;
    struct sockaddr_in localAddr;
    struct sockaddr_in clientAddr;
    struct sockaddr_in serverM;
    std::string username;
    std::string password;
    static std::string encrypt(std::string str);
    int loginStatus;

    void sendVerificationStatus(uint32_t status, int client_fd);
    RoomRequest recvRoomRequest(int client_fd);
    int forwardQuery(RoomRequest request, int client_fd);
    void handleMemberResponse(RoomRequest request, int client_fd);
    void handleGuestResponse(RoomRequest request, int client_fd);
    int verifyLogin(const std::string& key, const std::string& val);
    char identifySender(struct sockaddr_in& srcAddr);
};

#endif // CLIENT_H