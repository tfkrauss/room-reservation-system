#include "backendserver.h"

int main(){
    backendserver server = backendserver("double.txt", 42896, 'D');
    server.sendRooms(); // Send room list to serverM
    server.handleQueries(); //Handle all queries forwarded

    return 0;
}