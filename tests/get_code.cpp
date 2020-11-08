#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <unistd.h> 
#include <string> 
#include <cstring>
int main() {
    int sock = 0; 
    struct sockaddr_in serv_addr; 
    char buffer[14] = {0}; 
    const std::string host = "kelj0.com";
    std::string ip;
    const char* request = "GET / HTTP/1.1\r\nHost:kelj0.com\r\n\r\n";
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(80); // TODO add https support
    if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <=0 ) { 
        printf("\nInvalid address or you entered hostname and not IP! I'll convert it\n");
        std::string ip = inet_ntoa(**(in_addr**)gethostbyname(host.c_str())->h_addr_list);
        if(inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) { 
            printf("Failed to convert %s to IP\n", host.c_str());
        }else{
            printf("Coverted %s to %s\n", host.c_str(), ip.c_str());
        }
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 
    send(sock, request, strlen(request), 0); 
    int i = 0; 
    read(sock, buffer, 14);
    for(;buffer[i]!='\r';++i);
    
    std::string str_code;
    for(int j = 9; j<i;++j){
        str_code.push_back(buffer[j]);
    };
    int code = stoi(str_code);
    printf("Status code => %d\n", code); 

    return 0;
}
