#include "backendserver.h"

int main(){
    backendserver server = backendserver("single.txt", 41896, 'S');
    server.sendRooms(); // Send room list to serverM
    server.handleQueries(); //Handle all queries forwarded

    return 0;
}