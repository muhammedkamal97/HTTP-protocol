//
// Created by muhammed on 10/31/19.
//

#include <fstream>
#include "client.h"



void client::sendRequests(string file_name) {
    vector<request> commands = parseCommandFile(file_name);
    int index = 0;
    for(request req : commands){
        processCommand(req,index);
        index++;
    }
}


bool client::processCommand(request req,int index) {
    if(!connectToServer(req.hostname,req.port)){
        return false;
    }
    string request = "";
    if(req.method == "GET") {
        request += "GET ";
    }else if(req.method=="POST"){
        request += "POST ";
    }
    request += req.file_path;
    request += " HTTP/1.1\r\n";
    if(req.method == "POST"){
        request += "content-length: ";
        request += to_string(req.body.size());
        request += "\r\n\r\n";
        request += req.body;
    }else{
        request += "\r\n\r\n";
    }

    string response;
    if(req.method == "GET"){
        response = handleGET(request);
    }else{
        response = handlePOST(request);
    }
    string file_name = "response"+to_string(index)+".txt";
    ofstream f (file_name,ios::binary);
    f.write(response.c_str(),response.size());
    closeConnection();
}

string client::handleGET(string request) {
    send(server_socket,request.c_str(),request.size(),0);
    char buff[1024];
    ssize_t received = 0;
    memset(buff,0,sizeof buff);
    received = recv(server_socket,buff,sizeof buff,0);
    string response = buff;
    printf("%c\n",buff[1417]);
    printf("%d\n",received);
    if(received == 1024){
        memset(buff,0,sizeof buff);
        printf("recv\n");
        usleep(50000);
        while(recv(server_socket,buff,sizeof buff,0) == 1024){
            response = response.substr(0,response.size()-1);
            response += buff;
            usleep(50000);
            memset(buff,0,sizeof buff);
        }
        response += buff;
    }

    return response;
}


string client::handlePOST(string request) {
    cout<<request;
    send(server_socket,request.c_str(),request.size(),0);
    char buff[2048];
    int recved = 0;
    recv(server_socket,buff,sizeof buff,0);
    string response = buff;
    return buff;
}


bool client::connectToServer(string hostname, int port) {
    struct addrinfo hints,*res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(hostname.c_str(), to_string(port).c_str(), &hints, &res) == -1){
        return false;
    }

    server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(server_socket != -1){
        return connect(server_socket,res->ai_addr,res->ai_addrlen) != -1;
    }else{
        return false;
    }
}

vector<request> client::parseCommandFile(string file_name) {
    fstream f(file_name);
    string line;
    vector<request> commands;
    while(getline(f,line)){
        commands.push_back(commandParser(line));
    }
    f.close();
    return commands;
}

void client::closeConnection() {
    close(server_socket);
}

request client::commandParser(string command) {
    request req;
    vector<string> vec;
    boost::split(vec,command,boost::is_any_of(" "));
    req.method = vec[0];
    req.file_path = vec[1];
    req.hostname = vec[2];
    if(vec[0] == "POST"){
        if(vec.size() == 4){
            req.port = atoi(vec[3].c_str());
        }else{
            req.port = 80;
        }
        ifstream in_file;
        in_file.open(req.file_path.substr(1,req.file_path.size()), ios::binary | std::ios::in);
        in_file.seekg(0, std::ios::end);
        int file_size = in_file.tellg();
        vector<char> d;
        d.resize(file_size);
        char *file_data = new char[file_size];
        in_file.seekg(0, ios::beg);
        in_file.read(&d[0],file_size);
        req.body = string(d.begin(),d.end());
    }else{
        if(vec.size() == 4){
            req.port = atoi(vec[3].c_str());

        }else{
            req.port = 80;
        }
    }
    return req;
}
