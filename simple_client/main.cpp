#include <iostream>
#include "client.h"

int main() {
    client *cl = new client();
    cl->sendRequests("commands.txt");
    return 0;
}