//
// Created by muhammed on 10/29/19.
//

#ifndef SIMPLE_SERVER_SERVER_H
#define SIMPLE_SERVER_SERVER_H
#include<iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <fstream>
#include <map>

using namespace std;

struct client_attr{
    int socket_fd;//socket to respond with to the client.
    clock_t time;//time of the client's last request.
    void *s;// thread function that handle client requests.
};

struct wrapper{
    void *p;
    int max;
};


class server {
public:
    server();
    bool bindOnSocket(char* portNumber);
    void start_listening(int queueSize);
    int acceptConnection();
    void startServer();
    vector<string> requestParser(string req,int *body_length,int* body_id);
    void handleRequest(vector<string> request,int sock,char* buffer,int recived);

private:
    int socket_fd;
    int new_socket_fd;
    int queueSize;
    vector<client_attr> connected_clients;
    int client_num;
    map<string,string> file_extension_map;
    string get_header_of_file(string file_name);
    bool file_exist(string file_name);
    vector<char> readfile(string file_name);
    void handleGET(string file,int sock);
    void handlePOST(string file_name,int sock,char* buffer,int recived);
};



void *thread_handler(void *arg);
void *interupt(void *arg);
#endif //SIMPLE_SERVER_SERVER_H
