#include <iostream>
#include "server.h"

int main(int argc, char *argv[]) {
    if(argc < 2){
        printf("not specified port number\n");
        return 0;
    }
    int listeners = 5;
    if(argc == 3){
        listeners = atoi(argv[2]);
    }

    server *s = new server();
    if(!s->bindOnSocket(argv[1]))
        exit(1);
    s->start_listening(listeners);
    s->startServer();
    return 0;
}