#include "backendserver.h"

int main(){ //Server U
    backendserver server = backendserver("suite.txt", 43896, 'U');
    server.sendRooms(); // Send room list to serverM
    server.handleQueries();

    return 0;
}