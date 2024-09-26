#include "serverM.h"



ServerM::~ServerM() {    //Desctructor to close sockets when done.
    if (sockUDP >= 0) close(sockUDP);
    if (sockTCP >= 0) close(sockTCP);
}

ServerM::ServerM() {

}

//Create and bind the UDP socket on serverM
void ServerM::createUDP() {  //Creates the socket and binds to given port
    //Create socket
    if ((sockUDP = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {   //Learned from Beej
        std::cerr << "Error: Failed to create socket" << std::endl;
        exit(1);
    }

    //Set serveraddrUDP on the fixed port number PORT_UDP.
    memset(&serveraddrUDP, 0, sizeof(serveraddrUDP));
    serveraddrUDP.sin_family = AF_INET; // IPv4 //Learned from Beej
    serveraddrUDP.sin_port = htons(PORT_UDP);   //Learned from Beej
    inet_pton(AF_INET, LOOPBACK_ADDR, &serveraddrUDP.sin_addr); //Learned from Beej

    if (bind(sockUDP, (const struct sockaddr *)&serveraddrUDP, sizeof(serveraddrUDP)) < 0 )    //Bind socket. //Learned from Beej
    {
        std::cerr << "Error: Failed to bind socket for UDP sock" << std::endl;
        exit(1);
    }

}

//Create TCP socket and bind
void ServerM::createTCP() {
    sockTCP = socket(PF_INET, SOCK_STREAM, 0);  //Learned from Beej
    if( sockTCP ==-1) {
        std::cerr << "FAILED TO CREATE TCP SOCKET" <<  std::endl;
        exit(1);
    }

    memset(&serveraddrTCP, 0, sizeof serveraddrTCP);    //Learned from Beej
    serveraddrTCP.sin_family = AF_INET;
    serveraddrTCP.sin_port = htons(PORT_TCP);
    inet_pton(AF_INET, LOOPBACK_ADDR, &serveraddrTCP.sin_addr);

    if(bind(sockTCP, (const struct sockaddr *) &serveraddrTCP, sizeof (serveraddrTCP)) == -1) { //Learned from Beej
        std::cerr << "Error binding TCP socket on server side" << std::endl;
        exit(1);
    }
}

//Initializes the member list on serverM. Stores in a hashmap of username ,password pairs.
void ServerM::initMembers() {
    std::ifstream file ("member.txt");
    std::string word;
    int counter = 0;
    std::string key;
    std::string val;
    //Read in file line by line, store
    while(file>>word){
        if(counter % 2 == 0) {  //First word, add as key
            key = word.substr(0, word.length()-1); //Add the room number as key. Must remove the letter at the start
            std::transform(key.begin(), key.end(), key.begin(),
                           [](unsigned char c) { return std::tolower(c); });
        } else {
            val = word;
            loginInfo[key] = val;
        }
        counter++;
    }
}

//Configure the addresses of backend servers
void ServerM::configure() {

    //Configure serverD address
    memset(&serverD, 0, sizeof(serverD));
    serverD.sin_family = AF_INET;   //Learned from Beej
    serverD.sin_port = htons(42896); //Assign 42000 + XXX   //Learned from Beej
    inet_pton(AF_INET, LOOPBACK_ADDR, &serverD.sin_addr); //Convert address to binary. "127.0.0.1" is loopback address for testing purposes.    //Learned from Beej

    //Configure serverS address
    memset(&serverS, 0, sizeof(serverS));
    serverS.sin_family = AF_INET;   //Learned from Beej
    serverS.sin_port = htons(41896);    //Assign 41000 + XXX    //Learned from Beej
    inet_pton(AF_INET, LOOPBACK_ADDR, &serverS.sin_addr);   //Learned from Beej

    //Configure serverU address
    memset(&serverU, 0, sizeof(serverU));
    serverU.sin_family = AF_INET;   //Learned from Beej
    serverU.sin_port = htons(43896);    //Assign 43000 + XXX    //Learned from Beej
    inet_pton(AF_INET, LOOPBACK_ADDR, &serverU.sin_addr);   //Learned from Beej

}

//Receives the room lists from backend servers.
// Remains in while loop until confirms reception from all 3 servers S, D, U
void ServerM::receiveRoomLists() {
    //Booleans to check if room lists have been received
    bool receiveS = false;
    bool receiveD = false;
    bool receiveU = false;

    char buffer[1024];    //Receive buffer
    struct sockaddr_in srcAddr;    //Struct to store the srcAddr.
    socklen_t srcAddrLen = sizeof(srcAddr);

    while(receiveS == false || receiveD == false || receiveU == false) {
        memset(&buffer, 0, sizeof(buffer));    //Clear the buffer
        int receivedBytes = recvfrom(sockUDP, buffer, sizeof(buffer), 0, (struct sockaddr*)&srcAddr, &srcAddrLen);  //Learned from Beej
        if (receivedBytes < 0) {
            std::cout << "Error receiving from client" << std::endl;
        }

        //Compute number of ints received
        int numInts = receivedBytes / sizeof(uint32_t);

        ////Identify the sender by comparing port and IP:
        //if from serverS, change receiveD. Loop through message and add the room number to the appropriate hashSet
        if(srcAddr.sin_addr.s_addr == serverD.sin_addr.s_addr && srcAddr.sin_port == serverD.sin_port){
            std::cout << "The main server has received the room status from Server D using UDP over port " << PORT_UDP << "." <<std::endl;
            receiveD = true;
            for(int i = 0; i < numInts; i++){
                //Casts the char in buffer to uint32_t
                //uint32_t reads the input as the starting byte of a 4 byte (32bit) sequence
                //Converts each 4 byte sequence back into the original room number ints sent
                uint32_t* roomNum = reinterpret_cast<uint32_t*>(buffer + i * sizeof(uint32_t));
                doubleRooms.insert(ntohl(*roomNum));   //Convert from network byte order to host byte order
            }

        }
        //if from serverS, change receiveS. Loop through message and add the room number to the appropriate hashSet
        if(srcAddr.sin_addr.s_addr == serverS.sin_addr.s_addr && srcAddr.sin_port == serverS.sin_port){
            std::cout << "The main server has received the room status from Server S using UDP over port " << PORT_UDP << "." <<std::endl;
            receiveS = true;
            for(int i = 0; i < numInts; i++){
                //Casts the char in buffer to uint32_t
                //uint32_t reads the input as the starting byte of a 4 byte (32bit) sequence
                //Converts each 4 byte sequence back into the original room number ints sent
                uint32_t* roomNum = reinterpret_cast<uint32_t*>(buffer + i * sizeof(uint32_t));
                singleRooms.insert(ntohl(*roomNum));   //Convert from network byte order to host byte order
            }
        }

        //If from serverU, change receiveU. Loop through message and add the room number to the appropriate hashSet
        if(srcAddr.sin_addr.s_addr == serverU.sin_addr.s_addr && srcAddr.sin_port == serverU.sin_port){
            std::cout << "The main server has received the room status from Server U using UDP over port " << PORT_UDP << "." <<std::endl;
            receiveU = true;
            for(int i = 0; i < numInts; i++){
                //Casts the char in buffer to uint32_t
                //uint32_t reads the input as the starting byte of a 4 byte (32bit) sequence
                //Converts each 4 byte sequence back into the original room number ints sent
                uint32_t* roomNum = reinterpret_cast<uint32_t*>(buffer + i * sizeof(uint32_t));
                suiteRooms.insert(ntohl(*roomNum));   //Convert from network byte order to host byte order
            }
        }
    }
}

void ServerM::handleClients() {

    struct sockaddr_storage client_addr; // Client's address information
    socklen_t sin_size;
    int new_fd = -1;    //Set sockfd to invalid before calling accept.

    if(listen(sockTCP, BACKLOG) != 0) {     //Listen for connections    //Learned from Beej
        std::cerr << "Failed to listen() on TCP socket" << std::endl;
        exit(1);
    }
    while(1) {
        sin_size = sizeof client_addr;
        new_fd = accept(sockTCP, (struct sockaddr*) &client_addr, &sin_size);   //Accept connection. Store client address info  //Learned from Beej
        if( new_fd == -1) {
            std::cerr << "Failed to accept at TCP socket" <<std::endl;
        }

        if (!fork()) { // Create child process
            close(sockTCP); // Child doesn't need the listener
            while(true) {
                char buffer[1024] = { 0 };
                int bytes_received = recv(new_fd, buffer, sizeof(buffer), 0);    //Receive username & password  //Learned from Beej
                if(bytes_received < 0) {
                    std::cout << "Error receiving login" << std::endl;
                    exit(1);
                }
                //Receive & verify login info from client
                std::string login = std::string(buffer);
                //Parse string by the ' ' delimeter. First word is the username, second word is password (stored as key, val)
                const std::string username = login.substr(0, login.find(','));
                const std::string password = login.substr(login.find(',') + 1, login.length());
                if(password.empty()) {  //If empty password, log as guest user
                    handleGuest(username, new_fd);  //Handle guests in private method. Guests are always accepted so dont need a loop/verification check here.
                    break;
                }
                //If not guest, try to verify member login
                const int loginStatus = verifyLogin(username, password);
                if (loginStatus == BAD_PASSWORD || loginStatus == BAD_USERNAME) {   //If the user is not verified, send status and go back to receiving login info
                    sendVerificationStatus(loginStatus, new_fd);
                    continue;
                }
                handleMember(username, password, new_fd);   //Handle verified member in private method
            }

            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }
}

//Sends verification status of the login attempt to the client
void ServerM::sendVerificationStatus(uint32_t status, const int client_fd) {
    const uint32_t message = htonl(status);  //Convert to network byte order
    if (send(client_fd, &message, sizeof(uint32_t), 0) == -1) { //Learned from Beej
        std::cerr << "Failed to send verification status to client" << std::endl;
        exit(1);
    }
}

//Handle guest queries. Forwards to backend server. Receives response and forwards to client using private method handleGuestResponse().
void ServerM::handleGuest(const std::string &username, const int client_fd) {
    std::cout << "The main server received the guest request for " << username << " using TCP over port "
    << PORT_TCP << ". The main server accepts " << username << " as a guest." << std::endl;
    sendVerificationStatus(GUEST, client_fd);
    std::cout << "The main server sent the guest response to the client." << std::endl;

    while(true) {
        RoomRequest request = recvRoomRequest(client_fd);
        if(request.requestType == 'R') {    //If guest tries to make reservation request, send an error response.
            std::cout << username << " cannot make a reservation." << std::endl;

            uint32_t message = htonl(ERROR);
            if(send(client_fd, &message, sizeof(message), 0) == -1) {
                std::cout << "Failed to send error message to client" << std::endl;
            }
            std::cout << "The main server sent the error message to the client." << std::endl;

        } else {    //Else, forward query to backend server. Receive response from backend, forward to client.
            if(forwardQuery(request, client_fd) == -1) {    //If user entered an invalid roomtype, send error back to client. Skip backend server
                int32_t response = htonl(NO_MATCH);     //Convert message back to network byte order
                //Send response to client
                int bytesSent = send(client_fd, &response, sizeof(response), 0);
                if(bytesSent == -1){
                    std::cerr << "Error sending query response to client" << std::endl;
                }
                continue;
            }
            handleGuestResponse(request, client_fd);    //Receive response from backendserver and send to client
        }

    }
}
//RESPONSE HANDLING::
//
void ServerM::handleMember(const std::string &username, const std::string &password, const int client_fd) {
    std::cout << "The main server received the authentication for " << username
    <<" using TCP over port " << PORT_TCP << "." << std::endl;
    const int loginStatus = verifyLogin(username, password);    //Attempt to verify login. Returns an int indicating verification status
    sendVerificationStatus(loginStatus, client_fd); //Send login verification status to client
    std::cout << "The main server sent the authentication result to the client." << std::endl;

    while(true) {
        RoomRequest request = recvRoomRequest(client_fd);
        //Identify message type. Unique prints for each
        if(request.requestType == 'R') {    //Reservation request
            std::cout << "The main server has received the reservation request on Room "<< request.roomType
            << request.roomNumber << " from " << username << " using TCP over port " << PORT_TCP << "." << std::endl;
        } else if(request.requestType == 'A') { //Availability request
            std::cout << "The main server has received the availability request on Room "<< request.roomType
            << request.roomNumber << " from " << username << " using TCP over port " << PORT_TCP << "." << std::endl;
        }
        //If user entered an invalid roomtype (such as: L123) , send NO_MATCH result back to client. Skip backend server
        if(forwardQuery(request, client_fd) == -1) {
            int32_t response = htonl(NO_MATCH);     //Convert message back to network byte order
            //Send response to client
            int bytesSent = send(client_fd, &response, sizeof(response), 0);    //Learned from Beej
            if(bytesSent == -1){std::cerr << "Error sending query response to client" << std::endl;
            }
            continue;
        }
            handleMemberResponse(request, client_fd);    //Receive response from backendserver and send to client
    }
}

//Handles the query response from the backend server for member requests. Forwards the result to the client.
//Prints messages based on query results
void ServerM::handleMemberResponse(RoomRequest request, const int client_fd) {
    //Receive query response from backend server
    int32_t response;
    //Store source address.
    struct sockaddr_in srcAddr;
    socklen_t srcAddrLen = sizeof(srcAddr);

    //Receive response and store it in the response variable
    int receivedBytes = recvfrom(sockUDP, &response, sizeof(response), 0, (struct sockaddr*)&srcAddr, &srcAddrLen); //Learned from Beej
    if(receivedBytes == -1) {
        std::cout << "Failed to receive room request response from backend server" << std::endl;
    }

    response = ntohl(response); //Convert response to host byte order
    char sender = identifySender(srcAddr);  //Identify which server sent the response

    if(response == SUCCESS) {   //If the response is a success
        if(request.requestType == 'R') {    //Unique print for successful reservation
            std::cout << "The main server received the response and the updated room status from Server " << sender
            << " using UDP over port " << PORT_UDP << "." <<std::endl;
            std::cout << "The room status of Room " << request.roomType << request.roomNumber << " has been updated." << std::endl;
        }
        if(request.requestType == 'A') {    //Unique print for successful availability request
            std::cout << "The main server received the response from Server " << sender <<
                " using UDP over port " << PORT_UDP << "." << std::endl;
        }
    }else if(response == NO_MATCH || response == UNAVAILABLE) { //Print for unsuccessful query
        std::cout << "The main server received the response from Server " << sender << " using UDP over port " << PORT_UDP << "." << std::endl;
    }


    response = htonl(response);    //Convert message back to network byte order
    //Send response to client
    int bytesSent = send(client_fd, &response, sizeof(response), 0);    //Learned from Beej
    if(bytesSent == -1){
        std::cerr << "Error sending query response to client" << std::endl;
    }
    if(request.requestType == 'R') {    //Print for reservation query response
        std::cout << "The main server sent the reservation result to the client." << std::endl;
    }
    if(request.requestType == 'A') {    //Print for availability query response
        std::cout << "The main server sent the availability information to the client." << std::endl;
    }
}


//Receive the response from backend server for guest clients and forward to client server.
//ServerM only forwards availability requests to backend servers for guest clients.
void ServerM::handleGuestResponse(RoomRequest request, const int client_fd) {
    //Receive query response from backend server
    int32_t response;
    //Store source address. Can be used to identify sender.
    struct sockaddr_in srcAddr;
    socklen_t srcAddrLen = sizeof(srcAddr);

    //Receive response and store it in the response variable
    int receivedBytes = recvfrom(sockUDP, &response, sizeof(response), 0, (struct sockaddr*)&srcAddr, &srcAddrLen); //Learned from Beej
    if(receivedBytes == -1) {
        std::cout << "Failed to receive room request response from backend server" << std::endl;
    }

    response = ntohl(response); //Convert response to host byte order   //Learned from Beej
    char sender = identifySender(srcAddr);  //Identify which server sent the response   //Learned from Beej

    if(response == SUCCESS || response == NO_MATCH || response == UNAVAILABLE) {    //If it is a known response message
        std::cout << "The main server received the response from Server " << sender << " using UDP over port " << PORT_UDP << "." << std::endl;
    }else{  //If unrecognized response
        std::cerr << "Unrecognized response from server  " << sender << std::endl;  //If message doesnt match any of the response types
    }

    //Send response to the client server
    response = htonl(response);     //Convert message back to network byte order
    int bytesSent = send(client_fd, &response, sizeof(response), 0);    //Send to client
    if(bytesSent == -1){
        std::cerr << "Error sending query response to client" << std::endl;
    }
    std::cout << "The main server sent the availability information to the client." << std::endl;

}

//UTILITY METHOD:
//

RoomRequest ServerM::recvRoomRequest(const int client_fd) {
    RoomRequest request{};
    //Receiving the request in format of a struct RoomRequest
    ssize_t bytesRcv = recv(client_fd, &request, sizeof(request), 0);   //Learned from Beej
    if(bytesRcv < 0) {
        std::cout << "Error receiving room code from client" << std::endl;
    }

    request.roomNumber = ntohl(request.roomNumber);
    //std::cout << "Received room code: " <<request.roomType << request.roomNumber << " request type is :  " << request.requestType << std::endl;
    return request;
}

//Forward query over UDP. Identifies appropriate server and sends the request. Forwards request as a RoomRequest struct
int ServerM::forwardQuery(RoomRequest request, const int client_fd) {
    sockaddr_in *destination;

    //Identify which server to forward to
    if(request.roomType=='S') {
        destination = &serverS;
    }else if(request.roomType == 'D') {
        destination = &serverD;
    }else if(request.roomType == 'U') {
        destination = &serverU;
    }else {
        return -1;
    }

    //Need to send room number and request type to server via the RoomRequest struct
    //Send room request to backend server
    request.roomNumber = htonl(request.roomNumber); //Convert to network byte order
    int bytesSent = sendto(sockUDP, &request, sizeof(request), 0, (const sockaddr*) destination, sizeof(sockaddr_in));  //Learned from Beej
    if(bytesSent == -1) {
        std::cout << "Failed to room request to backend server" << std::endl;
    }
    std::cout << "The main server sent a request to Server " << request.roomType << "." << std::endl;
    return 0;
}

//Identify which backend server sent a message
char ServerM::identifySender(sockaddr_in& srcAddr) {

    if(srcAddr.sin_addr.s_addr == serverD.sin_addr.s_addr && srcAddr.sin_port == serverD.sin_port) {
        return 'D';
    }
    if(srcAddr.sin_addr.s_addr == serverS.sin_addr.s_addr && srcAddr.sin_port == serverS.sin_port) {
        return 'S';
    }
    if(srcAddr.sin_addr.s_addr == serverU.sin_addr.s_addr && srcAddr.sin_port == serverU.sin_port) {
        return 'U';
    }
    return 'X'; //If cant identify the sender
}


//AUTHENTICATION:
//

//Verify login for member users. Return int representing loginStatus to be sent to client server.
int ServerM::verifyLogin(const std::string &username, const std::string &password) {
    //Check for existence of the key in the map
    const auto iter = loginInfo.find(username);
    if(iter == loginInfo.end()) {   //Cannot find username
        return BAD_USERNAME;
    }
    if(iter -> second == password) {    //Found username and password match
        return MEMBER;
    }
    return BAD_PASSWORD;    //Username matches but password does not
}

//DEBUGGING:
//
void ServerM::printRooms() {
    std::cout << "Double Rooms Availability:" << std::endl;
    for (const auto& roomNum: doubleRooms) {
        std::cout << "Room " << roomNum << std::endl;
    }

    std::cout << "Suite Rooms Availability:" << std::endl;
    for (const auto& roomNum: suiteRooms) {
        std::cout << "Room " << roomNum << std::endl;
    }

    std::cout << "Single Rooms Availability:" << std::endl;
    for (const auto& roomNum: singleRooms) {
        std::cout << "Room " << roomNum << std::endl;
    }
}

int main(){
    ServerM server;
    server.initMembers();
    server.createUDP();
    server.createTCP();
    std::cout << "The main server is up and running." <<std::endl;

    server.configure();
    server.receiveRoomLists();
    server.handleClients();
    return 0;
}
