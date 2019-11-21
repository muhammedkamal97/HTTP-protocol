//
// Created by muhammed on 10/31/19.
//

#ifndef SIMPLE_CLIENT_CLIENT_H
#define SIMPLE_CLIENT_CLIENT_H

#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include<string>
#include <unistd.h>
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<netdb.h> //hostent
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>

using namespace std;

struct request{
    string method;
    string hostname;
    int port;
    string file_path;
    string body;
};


class client {
public:
    bool connectToServer(string hostname,int port);
    vector<request> parseCommandFile(string file_name);
    void closeConnection();
    bool processCommand(request req,int index);
    void sendRequests(string file_name);
private:
    request commandParser(string command);
    string handleGET(string request);
    string handlePOST(string request);
    int server_socket;

};


#endif //SIMPLE_CLIENT_CLIENT_H
