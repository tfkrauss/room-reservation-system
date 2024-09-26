//
// Created by student on 4/19/24.
//

#include "client.h"

client::client() : sockfd(-1), localPort(0), loginStatus(-1) {  //Initialize vars
    // Initialize any needed values
}

client::~client() {
    if (sockfd >= 0) {
        close(sockfd); // Ensure we close the socket when done  //learned from Beej
    }
}

void client::createClient() {
    //Create TCP socket @ client
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //learned from Beej
    if (sockfd == -1) {
        std::cerr << "error creating socket at client " << std::endl;
        exit(1);
    }
}

void client::configure() {
    //Initialize serverM with hardcoded address
    memset(&serverM, 0, sizeof(serverM));
    serverM.sin_family    = AF_INET; // IPv4
    serverM.sin_port = htons(SERVERM_PORT);//learned from Beej
    inet_pton(AF_INET, "127.0.0.1", &serverM.sin_addr);//learned from Beej
}


void client::connectServer() {

    //Connect to serverM
    if (connect(sockfd, (struct sockaddr*) &serverM, sizeof(serverM)) == -1) {  //learned from Beej
        std::cerr << "Failed to connect to serverM" << std::endl;
        exit(1);
    }
    std::cout << "Client is up and running." << std::endl;


}
//Retrieve the local port/address and store in this->localPort and this->localAddr
void client::setLocalPort() {
    socklen_t addrLen = sizeof(localAddr);
    //Get local address
    if (getsockname(sockfd, (struct sockaddr*)&localAddr, &addrLen) == -1) {
        std::cerr << "Error getting local address: " << strerror(errno) << std::endl;
        return;
    }
    //Set localPort to the port of local address. Convert to host byte order
    localPort = ntohs(localAddr.sin_port);
}

//Encryption method for username and password
std::string client::encrypt(std::string str) {
    int offset = 3;
    std::string encrypted;
    encrypted.reserve(str.size());  //Reserve space for the new string
    for(auto i=0u ; i < str.size(); i++) {
        char ch = str[i];   //Current char
        char new_ch;    //New offset char
        if(std::isalpha(ch)) {  //If is alphabetical: add offset and get remainder using 26
            char base = std::isupper(ch) ? 'A' : 'a';   //Determine base depending on case of char
            new_ch = (ch - base + offset) % 26 + base;  //Reset to 0-25, add offset and use remainder if exceeds bounds. Readd base to get proper ASCII value
            encrypted.push_back(new_ch);
        }else if(std::isdigit(ch)) {    //If numerical char: add offset and get remainder using 10
            new_ch = (ch - '0' + offset) % 10 + '0';
            encrypted.push_back(new_ch);
        }else { //If neither, do not offset
            encrypted.push_back(ch);
        }
    }
    return encrypted;
}

//Take in user login info and send to serverM for verification
void client::login() {
    while(true) {
        //Prompt user to enter username and password
        std::cout << "Please enter the username: ";
        std::getline(std::cin, username);
        //Convert username inputs to lowercase. Eliminates case sensitivity.
        std::transform(username.begin(), username.end(), username.begin(), [](unsigned char c) { return std::tolower(c); });
        const std::string userEncrypt = encrypt(username); //Seperate variable so we can print the unencrypted username in GUI. Should we make it lowercase or let the server shoot an error message
        std::cout << "Please enter the password: ";
        std::getline(std::cin, password);   //Use get line to successfully read empty input for guests
        if(password.empty()) {
            loginStatus = GUEST;
        } else {
            loginStatus = MEMBER;
        }
        password = encrypt(password);   //Encrypt password to send to serverM
        //Send login info to serverM for verification
        const std::string message = userEncrypt + "," + password;
        if (send(sockfd, message.c_str(), message.size(), 0) == -1) {
            std::cerr << "Failed to send LOGIN INFO to serverM" << std::endl;
            exit(1);
        }

        //Print message for sending login to serverM
        if(loginStatus == MEMBER) {
            std::cout << username << " sent an authentication request to the main server." << std::endl;
        } else if (loginStatus == GUEST) {
            std::cout << username << " sent a guest request to the main server using TCP over port " << SERVERM_PORT << std::endl;
        }

        //Receive login status
        int verification;
        int bytes_received = recv(sockfd, &verification, sizeof(uint32_t), 0);  //learned from Beej
        if(bytes_received == -1) {
            std::cout << "Error receiving login" << std::endl;
            exit(1);
        } else if (bytes_received != sizeof(uint32_t)) {
            std::cerr << "Improper data received" << std::endl;
        }

        //Handle output for different verification status
        verification = ntohl(verification); //Convert to host byte order    //learned from Beej
        if(verification == BAD_USERNAME) {
            std::cout << "Failed login: Username does not exist." << std::endl;
            continue;
        }
        if(verification == BAD_PASSWORD) {
            std::cout << "Failed login: Password does not match." << std::endl;
            continue;
        }
        if(verification == GUEST) {
            std::cout << "Welcome guest " << username << "!" << std::endl;
            break;
        }
        if(verification == MEMBER) {
            std::cout << "Welcome member " << username << "!" << std::endl;
            break;
        }
        break;
    }

}

void client::handleReqs() {
    //Loop infinitely until user hits ctr+c
    while(true) {
        //Works assuming room code input is always correct
        std::cout << "Please enter the room code: ";
        std::string room;
        std::cin >> room;
        struct RoomRequest roomReq{};
        //Convert input into room code to be sent via the RoomRequest struct. Check for valid input as well.
        if (room.length() > 1) {
            char roomType = room.at(0);
            std::string roomNumberStr = room.substr(1);

            // Check if roomNumberStr contains only digits
            if (std::all_of(roomNumberStr.begin(), roomNumberStr.end(), ::isdigit)) {
                roomReq.roomType = roomType;
                roomReq.roomNumber = htonl(std::stoi(roomNumberStr));
                // Now proceed with the request
            } else {
                std::cout << "Invalid room number. Please enter a valid room code.\n";
                continue; // Skip processing and prompt for input again
            }
        } else {
            std::cout << "Invalid room code format. Please enter a valid room code.\n";
            continue; // Skip processing and prompt for input again
        }

        //Prompt requests
        while(true) {
            std::cout << "Would you like to search for the availability or make a reservation?"
            "(Enter “Availability” to search for the availability or "
            "Enter “Reservation” to make a reservation ): ";
            std::string reqType;
            std::cin >> reqType;
            if (reqType == "Availability") {
                roomReq.requestType = 'A';
            }else if (reqType == "Reservation") {
                roomReq.requestType = 'R';
            }else {
                std::cout <<"Error: Unrecognized request" << std::endl;
                continue;
            }

            //std::cout << "Room requested: " << roomReq.roomType << ntohl(roomReq.roomNumber) << std::endl;   //TEST PRINT

            if (send(sockfd, &roomReq, sizeof(roomReq), 0) == -1) {
                std::cerr << "Failed to send room code for request" << std::endl;
                exit(1);
            }

            //Print request type sent to main server
            if(roomReq.requestType == 'A') {
                std::cout << username << " sent an availability request to the main server." << std::endl;
                availabilityReq(roomReq);
            } else if(roomReq.requestType == 'R') {
                std::cout << username << " sent a reservation request to the main server." << std::endl;
                reservationReq(roomReq);
            }
            break;
        }
    }



}

void client::availabilityReq(RoomRequest request) {
    uint32_t queryResponse;

    //Receive the request response and store in queryResponse
    int bytes_received = recv(sockfd, &queryResponse, sizeof(int32_t), 0);  //learned from Beej
    if (bytes_received == -1) {
        std::cerr << "Failed to receive query response from serverM" << std::endl;
    }

    queryResponse = ntohl(queryResponse);   //Convert to host byte order    //learned from Beej
    if(queryResponse == SUCCESS) {
        std::cout << "The client received the response from the main server using TCP over port " << localPort <<
                     ". \nThe requested room is available.\n" << std::endl;
    } else if(queryResponse == UNAVAILABLE) {
        std::cout << "The client received the response from the main server using TCP over port " << localPort <<
                     ". \nThe requested room is not available.\n" << std::endl;
    }else if(queryResponse == NO_MATCH) {
        std::cout << "The client received the response from the main server using TCP over port " << localPort <<
                     ". \nNot able to find the room layout.\n" << std::endl;
    }else {
        std::cerr << "Unrecognized response from serverM" << std::endl;
    }

}

void client::reservationReq(RoomRequest request) {
    uint32_t queryResponse;
    //Receive the request response and store in queryResponse
    int bytes_received = recv(sockfd, &queryResponse, sizeof(int32_t), 0);  //learned from Beej
    if (bytes_received == -1) {
        std::cerr << "Failed to receive query response from serverM" << std::endl;
    }

    queryResponse = ntohl(queryResponse);
    if(queryResponse == SUCCESS) {
        std::cout << "The client received the response from the main server using TCP over port "
        << localPort << " \nCongratulation! The reservation for Room "
        << request.roomType << ntohl(request.roomNumber) << " has been made.\n" << std::endl;   //learned from Beej
    }else if(queryResponse == UNAVAILABLE) {
        std::cout << "The client received the response from the main server using TCP over port " << localPort <<
                     ". Sorry! The requested room is not available.\n" << std::endl;
    }else if(queryResponse == NO_MATCH) {
        std::cout << "The client received the response from the main server using TCP over port " << localPort <<
                     ". Oops! Not able to find the room.\n" << std::endl;;
    }else if(queryResponse == ERROR) {
        std::cout << "Permission denied: Guest cannot make a reservation." << std::endl;
    } else {
        std::cerr << "Unrecognized response from serverM" << std::endl;
    }

}

int main() {
    client c{};

    c.configure();
    c.createClient();
    c.connectServer();
    c.setLocalPort();
    c.login();
    c.handleReqs();
    while(true) {  //While loop to handle client requests

        break;
    }

    return 0;
}

