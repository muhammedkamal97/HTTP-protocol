//
// Created by muhammed on 10/29/19.
//





#include "server.h"
#include <pthread.h>


server::server() {
    file_extension_map["jpg"] = "image/jpeg";
    file_extension_map["png"] = "image/png";
    file_extension_map["html"] = "text/html";
    file_extension_map["txt"] = "txt/plain";
    file_extension_map[""] = "txt/plain";
}





bool server::bindOnSocket(char* portNumber) {
    struct addrinfo hints, *res;

    memset(&hints,0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;


    int status;
    if ((status = getaddrinfo(NULL, portNumber, &hints, &res)) != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    socket_fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if(socket_fd == -1){
        perror("server: socket");
    }

    if(status = bind(socket_fd,res->ai_addr,res->ai_addrlen) == -1){
        perror("bind");
        cout<< "can't bind "<<gai_strerror(status)<<"\n";
        return false;
    }
    freeaddrinfo(res);
    cout<<"binding successfull on socket: "<<socket_fd<<"\n";
    return true;
}

void server::start_listening(int queueSize) {
    this->queueSize = queueSize;
    if(listen(socket_fd,queueSize) == -1){
        perror("listen");
    }
    cout<< "start listening successful\n";
}

int server::acceptConnection() {
    struct sockaddr_in client_addr;
    int size_addr = sizeof(struct sockaddr_in);
    new_socket_fd = accept(socket_fd, (struct sockaddr*) &client_addr, (socklen_t*) &size_addr);
    if(new_socket_fd == -1){
        perror("accept");
        return -1;
    }
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&(client_addr.sin_addr),str,INET_ADDRSTRLEN);
    cout << "connection from " << str<< " port "
         << ntohs(client_addr.sin_port) << endl;
    return new_socket_fd;
}


//start server 
void server::startServer() {
    pthread_t connections[queueSize];
    client_num = 0;
    wrapper arg;
    arg.p = &connected_clients;
    arg.max = queueSize;
    //start the interupter thread.
    pthread_t interputer;
    pthread_create(&interputer,NULL,interupt,&arg);


    while(1){
        struct sockaddr_in client_addr;
        socklen_t client_len;

        client_attr new_client;
        //block until new connection accepted.
        new_client.socket_fd = acceptConnection();
        if(new_client.socket_fd == -1){
            continue;
        }
        printf("new connection on socket: %d\n",new_client.socket_fd);
        new_client.time = clock();
        new_client.s = this;
        connected_clients.push_back(new_client);
        int creation = pthread_create(&connections[client_num++],NULL,thread_handler,&new_client);
        if(creation != 0)
            printf("creation of new thread faild");

        if(client_num >= queueSize){
            for(int i = 0;i < queueSize;i++){
                pthread_join(connections[i],NULL);
            }
            client_num = 0;
        }

    }
}

void *thread_handler(void *arg) {
    //extract args
    client_attr *client = (client_attr*) arg;
    int sock = client->socket_fd;
    server *sr = (server* )client->s;
    //loop of requests from client
    while(1){
        char buffer[2048];
        memset(buffer,0,sizeof buffer);
        int recieved;
        client->time = clock();
        recieved = recv(sock,buffer,sizeof buffer,0);
        if(recieved == 0 || recieved < 3){
            //recieve wrong format request.
            close(sock);
            break;
        }

        string req = string(buffer, buffer+recieved);
        cout<<"log : socket "<<sock<<" \n"<<buffer<<endl;
        vector<string> vec;
        int content_length = 0;
        int body_id = 0;
        try {
            vec = sr->requestParser(req, &content_length, &body_id);
        }catch (int e){
            cout<<"log : socket "<<sock<<" "<<"error parsing"<<endl;
            break;
        }
        try{
            if(vec[0] == "POST"){
                string body = vec[vec.size()-1];
                char request_body[content_length];
                memset(request_body,0,sizeof request_body);
                for(int i = body_id;i < recieved;i++){
                    request_body[i-body_id] = buffer[i];
                }

                if(content_length > recieved){
                    int length = recieved-body_id;
                    recv(sock,request_body+length,content_length-length,0);
                }
                sr->handleRequest(vec,sock,request_body,content_length);
            }else if(vec[0] == "GET"){
                sr->handleRequest(vec,sock,NULL,NULL);
            }
        }catch (int e){
            break;
        }

    }

}


vector<string> server::requestParser(string req,int *body_length,int* body_id) {
    vector<string> req_vec;
    boost::algorithm::split(req_vec,req, boost::is_any_of("\r\n"));
    vector<string> result;

    for(string str : req_vec){
        if(str != "") {
            result.push_back(str);
        }
    }
    vector<string> first_line;
    boost::algorithm::split(first_line,result[0],boost::is_any_of(" "));

    if(first_line[0] == "POST"){
        vector<string> second_line;
        boost::algorithm::split(second_line,result[1],boost::is_any_of(" "));
        *body_length = atoi(second_line[second_line.size()-1].c_str());
        string body = "";
        for(int i = 2;i<result.size();i++){
            body += result[i];
        }
        *body_id = result[0].size()+result[1].size()+6;
        first_line.push_back(body);

    }

    return first_line;
}


void server::handleRequest(vector<string> request,int sock,char* buffer,int recived) {
    if(request.size() < 2){
        return;
    }
    if(request[0] == "GET"){
        handleGET(request[1],sock);
    }else if(request[0] == "POST"){
        handlePOST(request[1],sock,buffer,recived);
    }
}

void server::handlePOST(string file_name, int sock, char *buffer,int recived) {
    if(file_name == "/"){
        return;
    }

    string file = file_name.substr(1,file_name.size());
    FILE * fp = fopen(file.c_str(),"w");

    fwrite(buffer, sizeof(char),recived,fp);
    //printf("hey post buffer");
    send(sock,"HTTP/1.1 200 OK\r\n",17,0);
    fflush(fp);
    fclose(fp);
}


void server::handleGET(string file, int sock) {
    if(!file_exist(file)){
        send(sock,"HTTP/1.1 404 Not Found\r\n\r\n",26,0);
    }else{
        string data_header = get_header_of_file(file);
        send(sock,data_header.c_str(),data_header.size(),0);
        vector<char> d = readfile(file);
        send(sock,&d[0],d.size(),0);
    }
}


bool server::file_exist(string file_name) {
    if(file_name == ""){
        return false;
    }

    if(file_name == "/"){
        ifstream f("index.html");
        return f.good();
    }
    ifstream f(file_name.substr(1,file_name.size()).c_str());
    return f.good();
}

string server::get_header_of_file(string file_name) {

    string file;
    // remove the slash
    if(file_name == "/"){
        file = "index.html";
    }else{
        file = file_name.substr(1,file_name.size());
    }


    //get the file extension
    int i = file.size()-1;
    while(file.c_str()[i] != '.') i--;
    string file_extension = file.substr(i+1,file.size());

    //get file length
    ifstream in_file(file.c_str(), std::ios::binary);
    in_file.seekg(0, std::ios::end);
    int file_size = in_file.tellg();

    string data_header = "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Type: "+
            file_extension_map[file_extension]+"\r\nContent-Length: "+to_string(file_size)+
            "\r\n\r\n";
    return data_header;
}




vector<char> server::readfile(string file_name) {
    string file;
    // remove the slash
    if(file_name == "/"){
        file = "index.html";
    }else{
        file = file_name.substr(1,file_name.size());
    }

    //get file length
    ifstream in_file;
    in_file.open(file, ios::binary | std::ios::in);
    in_file.seekg(0, std::ios::end);
    int file_size = in_file.tellg();

    vector<char> d;
    d.resize(file_size);
    char *file_data = new char[file_size];
    in_file.seekg(0, ios::beg);
    in_file.read(&d[0],file_size);
    //FILE *fp = fopen(file.c_str(),"rb");
    //fread(&d[0],1,file_size,fp);
    //string data = file_data;
    return d;
}


//intrupt thread that monitering the clients and close connection with
//idle clients.
void* interupt(void* arg){
    wrapper *w = (wrapper*)arg;
    vector<client_attr>* vec = (vector<client_attr>*)w->p;
    int max = w->max;
    int defaultTimeOut = 10;
    while(1){
        for(int i = 0;i < vec->size();i++){
            if((clock() - vec->at(i).time)/CLOCKS_PER_SEC > (defaultTimeOut - (vec->size()/max)*3)){
                close(vec->at(i).socket_fd);
                cout<<"timeout for : "<< vec->at(i).socket_fd<<endl;
                vec->erase(vec->begin() + i);
            }
        }
    }

}