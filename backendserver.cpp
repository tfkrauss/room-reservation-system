#include "backendserver.h"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>

//Constructor requires an upload file with the room codes and availability, port to bind the socket, and a one char descriptor ('S', 'D, etc.)
backendserver::backendserver(const std::string& fileName, int port, char serverDescriptor)
: sockfd(-1), serverType(serverDescriptor) {
    initialize(fileName);
    bootUp(port);
}

//Destructor
backendserver::~backendserver() {
    if (sockfd >= 0) {
        close(sockfd); // Ensure we close the socket when done  //learned from Beej
    }
}


//Read in room availability data from a file in the format : "X000, 1"
//where X000 is the room number, and 1 is the availability spaced with a comma
void backendserver::initialize(const std::string& fileName){
    std::ifstream file(fileName);   //Read in file
    std::string word;   //var for current word
    int counter = 0;    //Counter used to indicate which word is key versus val, since key and val alternate
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << fileName << std::endl;
        return;
    }
    //Add try catch block to check for a valid file, etc.
    //Read file & add to hashmap
    int key;
    while(file>>word){
        if(counter % 2 == 0) {  //First word, add as key
            key = std::stoi(word.substr(1)); //Add the room number as key. Must remove the letter at the start
        } else {
            int val = std::stoi(word);
            roomAvailability[key] = val;
        }
        counter++;
    }

    file.close();

    //Initialize serverM with hardcoded address
    memset(&serverM, 0, sizeof(serverM));
    serverM.sin_family    = AF_INET; // IPv4    //learned from Beej
    serverM.sin_port = htons(44896);    //learned from Beej
    inet_pton(AF_INET, "127.0.0.1", &serverM.sin_addr); //learned from Beej

}

void backendserver::bootUp(int port) {  //Creates the socket and binds to given port
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Error: Failed to create socket" << std::endl;
        exit(1);
    }

    //Set serveraddr
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family    = AF_INET; // IPv4
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port);

    if ( bind(sockfd, (const struct sockaddr *)&serveraddr,
        sizeof(serveraddr)) < 0 )
    {
        std::cerr << "Error: Failed to bind socket" << std::endl;
        exit(1);
    }
    std::cout << "The server " << serverType << " is up and running using UDP on port " << port << "." << std::endl;
}

//Send list of rooms to serverM
void backendserver::sendRooms() {
    std::vector<uint32_t> rooms;    //Use uint32_t to gurantee 32 bit size of each int
    for (const auto& entry : roomAvailability) { //Loop through map entries and add keys to buffer
        uint32_t room = htonl(entry.first); //Convert host byte order to network byte order
        rooms.push_back(room);
    }
    //rooms.data points to start of the vector
    int bytesSent = sendto(sockfd, rooms.data(), rooms.size()*sizeof(uint32_t), 0, (const sockaddr*)& serverM, sizeof(serverM)) == -1;  //learned from Beej
    if (bytesSent == -1){
        std::cerr << "Failed to send room list to serverM" << std::endl;
    }
    std::cout << "The server "<< serverType << " has sent the room status to the main server." << std::endl;
}

void backendserver::handleQueries() {
    struct sockaddr_in srcAddr; //Since can only receive from serverM, dont really need this?
    socklen_t srcAddrLen = sizeof(srcAddr);



    //CODE TO RECEIVE THE ROOM REQUEST AND SEND BACK THE AVAILABILITY STATUS
    while(true) {
        //Receive room request
        RoomRequest request;
        int receivedBytes = recvfrom(sockfd, &request, sizeof(request), 0, (struct sockaddr*)&srcAddr, &srcAddrLen);    //learned from Beej
        if(receivedBytes == -1) {
            std::cerr << "Failed to receive room request" << std::endl;
            exit(1);
        }
        request.roomNumber = ntohl(request.roomNumber); //learned from Beej

        if(request.requestType == 'R') {
            std::cout << "The server "<< serverType << " received a reservation request from the main server." << std::endl;
            reserveReq(request);
        } else if(request.requestType == 'A'){
            std::cout << "The server "<< serverType << " received an availability request from the main server." << std::endl;
            availabilityReq(request);
        }

    }
}

//Handle reserve requests
void backendserver::reserveReq(RoomRequest request) {

    uint32_t response;

    const int roomNum = request.roomNumber;

    //Check room availability status
    ////If room is not in the map
    auto roomIter = roomAvailability.find(roomNum);
    if (roomIter == roomAvailability.end()) {
        std::cout << "Cannot make a reservation. Not able to find the room layout." << std::endl;
        response = NO_MATCH;
    } else {
        // Room exists in the map, safe to access
        if (roomIter->second > 0) {
            roomIter->second--;    // Decrement the availability of the room
            std::cout << "Successful reservation. The count of Room " << roomNum << " is now "
                      << roomIter->second << "." << std::endl;
            response = SUCCESS;
        } else {
            std::cout << "Cannot make a reservation. Room " << roomNum << " is not available." << std::endl;
            response = UNAVAILABLE;
        }
    }

    response = htonl(response); //Convert to network byte order
    //Send response to serverM
    ssize_t bytesSent = sendto(sockfd, &response, sizeof(int32_t), 0, (const sockaddr*)& serverM, sizeof(serverM)) == -1;   //learned from Beej
    if (bytesSent == -1){
        std::cerr << "Failed to send reserve response to serverM" << std::endl;
    }
    response = ntohl(response); //Convert to host byte order for comparison
    if(response == SUCCESS) {
        std::cout << "The server "<< serverType << " finished sending the response and the updated room status to the main server." << std::endl;
    }
    if(response == UNAVAILABLE) {
        std::cout << "The server "<< serverType << " finished sending the response to the main server." << std::endl;
    }
}


//Handle reserve requests
//Currently same as reservationReq, just does not decrement.
void backendserver::availabilityReq(RoomRequest request) {

    int32_t response;

    const int roomNum = request.roomNumber;
    //If room is not in the map
    if(roomAvailability.find(roomNum) == roomAvailability.end()) {
        std::cout << "Not able to find the room layout." << std::endl;
        response = NO_MATCH;
    } else {
            //Check room availability status
        const int availability = roomAvailability.at(roomNum);
        if(availability > 0) {
            std::cout << "Room " << request.roomType << roomNum << " is available." << std::endl;
            response = SUCCESS;
        } else {
            std::cout << "Room " << roomNum << " is not available." << std::endl;
            response = UNAVAILABLE;
        }
    }



    response = htonl(response); //Convert to network byte order
    //Send response to serverM
    int bytesSent = sendto(sockfd, &response, sizeof(int32_t), 0, (const sockaddr*)& serverM, sizeof(serverM)) == -1;   //learned from Beej
    if (bytesSent == -1){
        std::cerr << "Failed to send reserve response to serverM" << std::endl;
    }
    std::cout << "The server "<< serverType << " finished sending the response to the main server." << std::endl;
}
