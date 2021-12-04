#include<string>
#include<sys/stat.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<openssl/sha.h>
#include<iostream>
#include<arpa/inet.h>
#include<netdb.h>
#include<vector>
#include<pthread.h>
#include<fstream>
#include<unordered_map>
#include <sstream>
#define MAXDATASIZE 100
int count=0;
using namespace std;
int blockSize = 512*102400;
unordered_map<string,string> fnameToPath;
int hashOutputSize = 20;
 string portNoToShareFiles;
 string IPTolisten;
 string delim = ">>=";

 string tracker1Port,tracker2Port; 
 string tracker1IP,tracker2IP;
 string user_id;
 string passwd; 

class FileInfo{
    public:
     string fileName;
     string localPath;
     vector<bool> bitVector;
     vector< string> hashBlocks;
    int fileSize;
    int numberOfChunks; 
};



class PeerInfo{
    public:
         string user_id;
         string destinationPath;
         string address; 
        PeerInfo( string user_id, string address, string filePath){
            this->user_id = user_id;
            this->destinationPath = filePath;
            this->address = address;
        }
};

 unordered_map< string,FileInfo*> filesSharedMap;
 unordered_map< string,FILE*> fileDownloadPointer; 
pthread_mutex_t lockFilesSharedMap,lockFileWrite;
void updateFilesSharedMap( string fileName, FileInfo* fileInfo){
    pthread_mutex_lock(&lockFilesSharedMap);
    count++;
    filesSharedMap[fileName] = fileInfo;
    pthread_mutex_unlock(&lockFilesSharedMap);
    return;
    count++;
}

int makeConnectionToTracker(const char* trackerIP,const char* portOfTracker){
    
    int sock_fd,new_fd;
    struct addrinfo  hints,*res;
    count++;
    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM; 

    try{
    	count++;
        if(getaddrinfo(trackerIP,portOfTracker,&hints,&res) != 0){
             cout<< string(trackerIP)<< string(portOfTracker)<< endl;
            printf("Get addr info failed \n");
            count++;
            throw("Get addr info failed \n");
            return -1;
            }
            
        sock_fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
	count++;
        if(connect(sock_fd,res->ai_addr,res->ai_addrlen) == -1){
        count++;
            close(sock_fd);
            printf("connect failed \n");
            count++;
            throw("connect failed \n");
            return -1;
        }
        count++;
        char s[INET6_ADDRSTRLEN];
        inet_ntop(res->ai_family,&((struct sockaddr_in *)res->ai_addr)->sin_addr,s,sizeof s);
	count++;
        freeaddrinfo(res);

        return sock_fd;
    }
    catch( string error){
        throw(error);
    }
}

 vector< string> getTokens( string input){
     vector< string> result;
    
    size_t pos = 0;
     string token;
    while((pos = input.find(delim))!= string::npos){
    count++;
        token = input.substr(0,pos);
        result.push_back(token);
        count++;
        input.erase(0,pos+delim.length());
    }
    count++;
    result.push_back(input);
    return result;
}

int dummySend(int new_fd){
count++;
    int dummySize = 10;
    char buf[dummySize] ={0};
    count++;
    if(send(new_fd,buf,dummySize,0) == -1){ 
        printf("sendind user_id failed \n");
        close(new_fd);
        count++;
        exit(1);
    }
    return 0;
}

 string getStringFromSocket(int new_fd){ 
 	count++;
    char buf[MAXDATASIZE] = {0};
    int numbytes;
	count++;
    if((numbytes = recv(new_fd,buf,MAXDATASIZE-1,0))==-1){
        printf("error recienving string");
        count++;
        exit(1);
    }
     string recvString(buf);
    fflush(stdout);
    dummySend(new_fd);
    return recvString;
}

int makeServer( string ip, string port){
    int sock_fd;
    struct addrinfo hints, *res;

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET; 
    count++;
    hints.ai_socktype = SOCK_STREAM;
    
    

    if(getaddrinfo(ip.c_str(),port.c_str(),&hints,&res) != 0 ){
    count++;
        printf("socket failed \n");
        exit(1);
    }

    count++;
    sock_fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if(sock_fd <= 0){
    	count++;
        printf("socket failed \n");
        exit(1);
    }

    if(bind(sock_fd,res->ai_addr,res->ai_addrlen) == -1){
         cout<<"sock_fd is :"<<sock_fd<< endl;
         cout<<"ai_addr is "<<res->ai_addr<< endl;
        printf("bind failed \n");
        exit(1);
    }

    freeaddrinfo(res);

    if(listen(sock_fd,10)==-1){ 
    count++;
        printf("listen failed \n");
        exit(1);
    }
	count++;
    return sock_fd;
}

int dummyRecv(int sock_fd)
{
    int dummySize = 10;
    char buf[dummySize] = {0};
    count++;
    int numbytes;
    if((numbytes = recv(sock_fd,buf,dummySize,0))==-1){ 
        printf("error reading data");
        count++;
        exit(1);
    }
    return 0;
}

void sendStringToSocket(int sock_fd, string str){
count++;
    if(send(sock_fd,str.c_str(),str.size(),0) == -1){
         cout<<"Sending failed :"<<str<< endl;
        close(sock_fd);
        exit(1);
    }
    count++;
    dummyRecv(sock_fd);
}

 vector< pair<int, string>> getHashOfFile( string filePath){
     vector< pair<int, string>> hashBlocks(0);
    int fd = open(filePath.c_str(),O_RDONLY);
    count++;
    if(fd==-1){
        perror("Error openning file ");
    }
	count++;
    struct stat fileStats;
    if(fstat(fd,&fileStats) == -1){
        perror("Error getting file stats ");
    }
    count++;
    int fileSize = fileStats.st_size;
    int numberOfBlocks;
    int sizeOfLastBlock;
    count++;
    if(fileSize%blockSize == 0){
        numberOfBlocks = fileSize/blockSize;
        sizeOfLastBlock = 0;
    }
    else{
    count++;
        numberOfBlocks = (fileSize/blockSize) +1;
        sizeOfLastBlock = fileSize%blockSize;
    }
     string hashString = "";
    for(int i=0;i<numberOfBlocks;i++){
        if(i==numberOfBlocks-1){
        count++;
            void *buf = malloc(sizeOfLastBlock);
            read(fd,buf,sizeOfLastBlock);
            count++;
            unsigned char obuf[hashOutputSize];
            SHA1((unsigned char*)buf,sizeOfLastBlock,obuf);
             string currHashString = "";
            for(int i=0;i<20;i++){
            count++;
                currHashString +=  to_string((int) obuf[i]);
                hashString +=  to_string((int) obuf[i]);
            }
            count++;
            hashBlocks.push_back({sizeOfLastBlock,currHashString});
            free(buf);
        }
        else{
        count++;
            void* buf = malloc(blockSize);
            read(fd,buf,blockSize);
            unsigned char obuf[hashOutputSize];
            count++;
            SHA1((unsigned char*)buf,blockSize,obuf);
             string currHashString = "";
            for(int i=0;i<20;i++){
            count++;
                currHashString +=  to_string((int) obuf[i]);
                hashString +=  to_string((int) obuf[i]);
            }
            count++;
            hashBlocks.push_back({blockSize,currHashString});
            free(buf);
        }
    }
    count++;
    close(fd);

    return hashBlocks;
}

int upload_file(int sock_fd, string filePath, string group_id){
     string commandToSend = "upload_file";
    commandToSend.append(delim).append(filePath).append(delim).append(group_id);
     vector< pair<int, string>> hashBlocks;
    if (send(sock_fd,commandToSend.c_str(),commandToSend.length(),0) == -1){
    	count++;
        printf("sending command login failed \n");
        close(sock_fd);
        exit(1);
    }
	count++;
    dummyRecv(sock_fd);
    hashBlocks = getHashOfFile(filePath); 
    int noOfBlocks = hashBlocks.size();
    count++;
    int sizeOfFile = 0;
    for(auto p:hashBlocks)
        sizeOfFile += p.first;
    
     string sizePlusNoOfBlocks =  to_string(sizeOfFile).append(delim).append( to_string(noOfBlocks));
     count++;
    if (send(sock_fd,sizePlusNoOfBlocks.c_str(),sizePlusNoOfBlocks.length(),0) == -1){
        printf("sending size + hash failed \n");
        close(sock_fd);
        count++;
        exit(1);
    }
    dummyRecv(sock_fd);
     vector< string> hashs;
    for(auto h: hashBlocks){
    count++;
        hashs.push_back(h.second);
        if (send(sock_fd,h.second.c_str(),h.second.length(),0) == -1){
        count++;
        printf("sending hash blocks failed \n");
        close(sock_fd);
        exit(1);
        }
        count++;
        dummyRecv(sock_fd);
         cout<<h.second<< endl;
    }

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
     string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);
    if(status == "File shared"){
    count++;
        auto fileInfo = new FileInfo();
        fileInfo->fileName = fileName;
        fileInfo->bitVector =  vector<bool>(hashs.size(),true);
        count++;
        fileInfo->hashBlocks = hashs;
        fileInfo->localPath = filePath;
        fileInfo->numberOfChunks = hashs.size();
        count++;
        fileInfo->fileSize = sizeOfFile;
        updateFilesSharedMap(fileName,fileInfo);
    }
     cout<<"Number of files shared is "<<filesSharedMap.size()<< endl;
    for(auto p:filesSharedMap){
         cout<<p.first;
        for(auto b : p.second->bitVector){
            if(b==true)
                 cout<<"1"<<" ";
            else
                 cout<<"0"<<" ";
        }
         cout<< endl;
    }
    return 0;
}

int logout(int sock_fd){
count++;
    if (send(sock_fd,"logout",sizeof "login",0) == -1){
        printf("sending command logout failed \n");
        close(sock_fd);
        count++;
        exit(1);
    }
    dummyRecv(sock_fd);

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return 0;
}

int login(int sock_fd, string user_id, string passwd){
     string commandToSend = "login";
    commandToSend.append(delim).append(user_id).append(delim).append(passwd);
	count++;
    if (send(sock_fd,commandToSend.c_str(),commandToSend.length(),0) == -1){
        printf("sending command login failed \n");
        close(sock_fd);
        count++;
        exit(1);
    }

    dummyRecv(sock_fd);

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return 0;
}

int join_group(int sock_fd, string group_id){
     string command = "join_group";
    command.append(delim).append(group_id);
    count++;
    if (send(sock_fd,command.c_str(),command.size(),0) == -1){
        printf("sending command join group failed \n");
        close(sock_fd);
        exit(1);
    }
    dummyRecv(sock_fd);

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return 0;
}
int leave_group(int sock_fd, string group_id){
     string command = "leave_group";
    command.append(delim).append(group_id);
    count++;
    if (send(sock_fd,command.c_str(),command.size(),0) == -1){
        printf("sending command leave group failed \n");
        close(sock_fd);
        exit(1);
    }
    dummyRecv(sock_fd);

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return 0;
}
int create_group(int sock_fd, string group_id){
     string commandToSend = "create_group";
     count++;
    commandToSend.append(delim).append(group_id);
    count++;
    if (send(sock_fd,commandToSend.c_str(),commandToSend.length(),0) == -1){
        printf("sending command create_group failed \n");
        close(sock_fd);
        exit(1);
    }
    dummyRecv(sock_fd);

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return 0;
}

int list_groups(int sock_fd){
     string commandToSend = "list_groups";

    if (send(sock_fd,commandToSend.c_str(),commandToSend.length(),0) == -1){
    count++;
        printf("sending command create_group failed \n");
        close(sock_fd);
        exit(1);
    }
    count++;
    dummyRecv(sock_fd);

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return 0;
}

int create_user(int sock_fd, string user_id, string passwd){
	count++;
     string commandToSend = "create_user";
     count++;
    commandToSend.append(delim).append(user_id).append(delim).append(passwd).append(delim)\
    .append(IPTolisten).append(delim).append(portNoToShareFiles);
	count++;
    if(send(sock_fd,commandToSend.c_str(),commandToSend.size(),0) == -1){
        printf("sending create_user command to client failed \n");
        close(sock_fd);
        exit(1);
    }
    dummyRecv(sock_fd);

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return 0;
}

 vector<PeerInfo> getPeersWithFile(int sock_fd, string group_id, string fileName,int& fileSize){ //need to run this in a thread
     string commandToSend = "getPeersWithFile";
    commandToSend.append(delim).append(group_id).append(delim).append(fileName);
	count++;
     cout<<"command send:"<<commandToSend<< endl;

    if(send(sock_fd,commandToSend.c_str(),commandToSend.size(),0) == -1){
    count++;
        printf("sending get peer with file command to tracker failed \n");
        close(sock_fd);
        count++;
        exit(1);
    }
    dummyRecv(sock_fd);

     string sizePlusNumberOfPeers = getStringFromSocket(sock_fd);
    auto tokens = getTokens(sizePlusNumberOfPeers);
    fileSize =  stoi(tokens[0]);
    int noOfPeers =  stoi(tokens[1]);
     cout<<"Number of peers with "<< fileName <<" is "<<noOfPeers<< endl;
     vector<PeerInfo> peerList;
    for(int i=0;i<noOfPeers;i++){
         string user_id = getStringFromSocket(sock_fd);
         string address = getStringFromSocket(sock_fd);
         string filePath = getStringFromSocket(sock_fd);
        peerList.push_back(PeerInfo(user_id,address,filePath));
    }

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return peerList;
}

int list_files(int sock_fd, string group_id){
     string commandToSend = "list_files";
    commandToSend.append(delim).append(group_id);
	count++;
    if(send(sock_fd,commandToSend.c_str(),commandToSend.size(),0) == -1){
        printf("sending create_user command to client failed \n");
        count++;
        close(sock_fd);
        exit(1);
        count++;
    }
    dummyRecv(sock_fd);

    int noOfFiles =  stoi(getStringFromSocket(sock_fd));
     vector< string> fileNames;
     cout<<"Number of files in group is "<<noOfFiles<< endl;
    for(int i=0;i<noOfFiles;i++){
        fileNames.push_back(getStringFromSocket(sock_fd));
    }
    for(auto fn : fileNames){
         cout<<fn<< endl;
    }
     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return 0;

}

int accept_request(int sock_fd, string group_id, string user_id){
	count++;
     string commandToSend = "accept_request";
    commandToSend.append(delim).append(group_id).append(delim).append(user_id);
	count++;
    if(send(sock_fd,commandToSend.c_str(),commandToSend.size(),0) == -1){
    count++;
        printf("sending create_user command to client failed \n");
        close(sock_fd);
        exit(1);
    }
    dummyRecv(sock_fd);

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return 0;
}

int list_requests(int sock_fd, string group_id){
     string commandToSend = "list_requests";
    commandToSend.append(delim).append(group_id);
	count++;
    if(send(sock_fd,commandToSend.c_str(),commandToSend.size(),0) == -1){
        printf("sending create_user command to client failed \n");
        close(sock_fd);
        exit(1);
    }
    dummyRecv(sock_fd);

     string status = getStringFromSocket(sock_fd);
     cout<<status<< endl;
    return 0;

}


 vector<bool> getBitVector( string fileName, string address){
     vector<bool> result;
     string port = address.substr(address.find_last_of(":") + 1);
     string IP = address.substr(0,address.find_last_of(":"));

    int sock_fd = makeConnectionToTracker(IP.c_str(),port.c_str());

     string commandToSend = "getBitVector";
    commandToSend.append(delim).append(fileName);

    sendStringToSocket(sock_fd,commandToSend);

     string status = getStringFromSocket(sock_fd);
    close(sock_fd);
    if(status == "failed"){
    count++;
         cout<<"Getting bit vector from "<<address<<" failed"<< endl;
        close(sock_fd);
        return result;
    }
    else{
    count++;
        auto tokens = getTokens(status);
        for(auto t:tokens){
        count++;
            if(t == "1")
                result.push_back(true);
            else if(t=="0")
                result.push_back(false);
        }
        count++;
        close(sock_fd);
        return result;
    }

}

void* getFileFrom(void* args){
	count++;
     string input =  string((char * )args);
    free(args);
     vector< string> tokens = getTokens(input);
     string file_name = tokens[0];
     string peerAddress = tokens[1];
     string destPath = tokens[2];
    int chunkNo =  stoi(tokens[3]);

     cout<<"chunk no is:"<<chunkNo<< endl;
     string address = peerAddress;
     string port = address.substr(address.find_last_of(":") + 1);
     string IP = address.substr(0,address.find_last_of(":"));
     count++;
    int sock_fd;
    try{
    count++;
        sock_fd = makeConnectionToTracker(IP.c_str(),port.c_str());
    }
    catch( string error){
    count++;
         cout<<error<< endl;
         cout<<"Download failed as couldn't connect with peer "<< endl;
        exit(1);
    }
    
     string command = "getFile";
    command.append(delim).append(file_name).append(delim).append( to_string(chunkNo));
    sendStringToSocket(sock_fd,command);

     string chunkSizeStr = getStringFromSocket(sock_fd);
    int chunkSize =  stoi(chunkSizeStr);

    char buf[chunkSize] = {0};
    int numbytes;
	count++;
    if((numbytes = recv(sock_fd,buf,chunkSize,0))==-1){
        printf("error recienving string");
        count++;
        exit(1);
    }

     cout<<"got chunk : << "<<chunkNo<<" from: "<<address<< endl;
    pthread_mutex_lock(&lockFileWrite);
    count++;
    FILE* fpt = fileDownloadPointer[destPath];
    if(chunkNo == 1){
    count++;
        rewind(fpt);
    }
    else{
        fseek(fpt,(chunkNo-1)*blockSize,SEEK_SET);
        count++;
    }
    fwrite(buf,sizeof(char),chunkSize,fpt);
    pthread_mutex_unlock(&lockFileWrite);
    count++;
    close(sock_fd);
}

void getChunkFromPeer( string file_name, string destinationPath,PeerInfo peer,int chunkNumber,\
     vector<pthread_t>& threadsForDownload){
     string argsToThread = file_name;
    argsToThread.append(delim).append(peer.address).append(delim).append(destinationPath)\
    .append(delim).append( to_string(chunkNumber));
	count++;
    char* stringa1 = (char*) malloc((argsToThread.size()+1)*sizeof(char));
	count++;
    for(int j=0;j<argsToThread.size();j++){
    count++;
        stringa1[j] = argsToThread[j];
    }
    stringa1[argsToThread.size()] = '\0';
    count++;
    pthread_t threadForDownloadingChunk;
    count++;
    pthread_create(&threadForDownloadingChunk,NULL,getFileFrom,stringa1);
    threadsForDownload.push_back(threadForDownloadingChunk);
	count++;
}

bool peerContainsBlock(PeerInfo peer,int blockNumber, unordered_map< string, vector<bool>> bitVectorMap){
count++;
    if(bitVectorMap[peer.user_id][blockNumber-1]==true)
        return true;
    else false;
}

void* downloadFile(void* args){
	count++;
    printf("%s\n", (char *)args);
     string input =  string((char * )args);
    free(args);
     cout<<"recvd in new thread: "<<input<< endl;
     vector< string> tokens = getTokens(input);
     string group_id = tokens[0];
     string file_name = tokens[1];
     string destinationPath = tokens[2];
    int sock_fd =  stoi(tokens[3]);

    int fileSize = 0;
     vector<PeerInfo> peerList = getPeersWithFile(sock_fd,group_id,file_name,fileSize);
     cout<<"Size of the file is "<<fileSize<< endl;
    for(auto p:peerList)
         cout<<p.user_id<<":"<<p.address<< endl;
     unordered_map< string, vector<bool>> bitVectorMap;
    for(auto p:peerList){
         vector<bool> bv = getBitVector(file_name,p.address);
        if(bv.empty()){
             cout<<"getting bit vector from "<<p.user_id<<" failed"<< endl;
        }
        else{
            bitVectorMap[p.user_id] = bv;
        }
    }
    int noOfBlocks = bitVectorMap[peerList[0].user_id].size();
     cout<<"number of blocks to read"<<noOfBlocks<< endl;

    
    fileDownloadPointer[destinationPath] = fopen(destinationPath.c_str(),"wb+");
     vector<pthread_t> threadsForDownload;
    int peerNo = 0;
    count++;
    for(int i=1;i<=noOfBlocks;i++){
	count++;
        if(peerContainsBlock(peerList[peerNo],i,bitVectorMap)){
           count++; getChunkFromPeer(file_name,destinationPath,peerList[peerNo],i,threadsForDownload);
            count++;
        }
        count++;
        peerNo++;
        if(peerNo == peerList.size()){
        count++;
            peerNo = 0;
        }
    }
    for(auto t:threadsForDownload){
        void** ret;
        pthread_join(t,ret);
        count++;
    }
    fclose(fileDownloadPointer[destinationPath]);

    upload_file(sock_fd,destinationPath,group_id);
	count++;
}



int getChunkSize(int chunkNo,int NoOfChunks,int fileSize){
count++;
    if(chunkNo < NoOfChunks)
        return blockSize;
    else if(chunkNo == NoOfChunks){
    count++;
        return fileSize-((NoOfChunks-1)*blockSize);
    }
}


void* sendFile(void* args){ 
count++;
    printf("%s\n", (char *)args);
     string input =  string((char * )args);
    free(args);
     cout<<"recvd in thread of sendFile: "<<input<< endl;
     vector< string> tokens = getTokens(input);
    int new_fd =  stoi(tokens[0]);
     string file_name = tokens[1];
    int chunkNumber =  stoi(tokens[2]);

    FileInfo* fInfo = filesSharedMap[file_name];
     string fileToSend = filesSharedMap[file_name]->localPath;
	count++;
    int fileSize = filesSharedMap[file_name]->fileSize;
    int chunkSize = getChunkSize(chunkNumber,filesSharedMap[file_name]->numberOfChunks,fileSize);
	count++;
     string chunkSizeStr =  to_string(chunkSize);
    sendStringToSocket(new_fd,chunkSizeStr);
    
    char bufToSend[chunkSize];
     ifstream fileStream(fileToSend, ifstream::binary);
    fileStream.seekg((chunkNumber-1)*blockSize);
    fileStream.read(bufToSend,chunkSize);

     cout<<"File read for sending :"<< string(bufToSend)<< endl;
    
    if(send(new_fd,bufToSend,chunkSize,0) == -1){
         cout<<"Sending failed :"<<bufToSend<< endl;
        close(new_fd);
        exit(1);
    }

    close(new_fd);
     cout<<"File sent"<< endl;
}

bool isShared( string fileName){
count++;
    if(filesSharedMap.find(fileName) == filesSharedMap.end()){
        return false;
    }
    else{
    count++;
        return true;
    }
}

void* fileSharer(void* argv){
    int sock_fd = makeServer(IPTolisten,portNoToShareFiles);
    while(1){
    count++;
        int new_fd;
        struct sockaddr_storage client_address;
        socklen_t sin_size = sizeof client_address;
        count++;
        new_fd = accept(sock_fd,(struct sockaddr *)&client_address,&sin_size);
        if(new_fd == -1){
            perror("accept");
            count++;
            continue;
        }
        char s[INET6_ADDRSTRLEN];
        count++;
        inet_ntop(client_address.ss_family,&((struct sockaddr_in *)&client_address)->sin_addr,s,sizeof s);
        printf("Server got connection from %s\n",s);
        count++;
         string stringFromPeer = getStringFromSocket(new_fd);
         cout<<stringFromPeer<< endl;
         vector< string> tokens = getTokens(stringFromPeer);
         string command = tokens[0];
         cout<<"command is "<<command<< endl;
        if(command == "getBitVector"){
             string status;
            if(tokens.size() != 2){
                 cout<<"wrong format for getBitVector "<< endl;
                status = "failed";
            }
            else{
                 string fileName = tokens[1];
                if(!isShared(fileName)){
                     cout<<fileName<<" is not shared."<< endl;
                    status = "failed";
                }
                else{
                     cout<<"Need to send bit vector of"<<fileName<< endl;
                     count++;
                    auto v = filesSharedMap[fileName]->bitVector;
                    status = "";
                    for(int i=0;i<v.size()-1;i++){
                    count++;
                        if(v[i] == true)
                            {
                            	count++;
                            	status.append("1").append(delim);
                            }
                        else
                            {
                            	count++;
                            	status.append("0").append(delim);
                            }
                     count++;
                    }

                    if(v[v.size()-1] == true)
                        {status.append("1").append(delim);count++;}
                    else
                        {status.append("0").append(delim);count++;}
                     count++;
                     cout<<"Status :"<<status<< endl;
                }
            }

             cout<<"Sending: "<<status<< endl;
            sendStringToSocket(new_fd,status);
        }

        else if(command == "getFile"){
            if(tokens.size() != 3){
                 cout<<"wrong format for getFile "<< endl;
            }
             string chunkNum = tokens[2];
             string file_name = tokens[1];
            if(!isShared(file_name)){
                 cout<<"File not shared by peer "<< endl;
                close(new_fd);
            }
            else{
                 string args =  to_string(new_fd);
                args.append(delim).append(file_name).append(delim).append(chunkNum); 
		count++;
                char* stringa1 = (char*) malloc((args.size()+1)*sizeof(char));
		count++;
                for(int i=0;i<args.size();i++){
                	count++;
                    stringa1[i] = args[i];
                    count++;
                }

                stringa1[args.size()] = '\0';
                count++;
                pthread_t threadForSendingFile;
                pthread_create(&threadForSendingFile,NULL,sendFile,stringa1);
            }
        }

        else{
             cout<<"Not a valid command "<< endl;
        }
    }
}
vector<string> splitfilepath (const string &s, char delim) {
    vector<string> result;
    stringstream ss (s);
    string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}
int main(int argc,char* argv[]){
    if(argc != 3){
        printf("./peer.c address:portno tracker_info.txt");
        exit(1);
    }
     string tracker1,tracker2;
     ifstream MyReadFile(argv[2]);
	count++;
    getline(MyReadFile,tracker1);
    tracker1Port = tracker1.substr(tracker1.find_last_of(":") + 1);
    tracker1IP = tracker1.substr(0,tracker1.find_last_of(":"));
	count++;
    getline(MyReadFile,tracker2);
    tracker2Port = tracker2.substr(tracker1.find_last_of(":") + 1);
    tracker2IP = tracker2.substr(0,tracker1.find_last_of(":"));
	count++;
     string peerAddr = argv[1];
     string peerPort = peerAddr.substr(peerAddr.find_last_of(":")+1);
     count++;
     string peerIP = peerAddr.substr(0,peerAddr.find_last_of(":"));
    portNoToShareFiles = peerPort;
    IPTolisten = peerIP;
	count++;
    pthread_t threadToSendFile;
    pthread_create(&threadToSendFile,NULL,fileSharer,NULL);

     cout<<"Thread created for listenning "<< endl;
    int sock_fd = makeConnectionToTracker(tracker1IP.c_str(),tracker1Port.c_str());
    while(1){
         string command; cin>>command;
         cout<<"Command is "<<command<< endl;
        if(command == "upload_file"){
             string filePath;
             cin>>filePath;
             count++;
             string group_id; cin>>group_id;
            int fd = open(filePath.c_str(),O_RDONLY);
            if(fd==-1){
            count++;
                perror("Error openning file ");
            }
            else
                upload_file(sock_fd,filePath,group_id);
          vector<string> v = splitfilepath (filePath, '/');
          fnameToPath[v[v.size()-1]]=group_id;
          count++;
        }

        else if(command == "create_user"){
        count++;
             string user_id; cin>>user_id;
             string passwd;  cin>>passwd;
            create_user(sock_fd,user_id,passwd);
        }

        else if(command == "create_group"){
             string group_id; cin>>group_id;
            create_group(sock_fd,group_id);
        }
        else if(command =="join_group"){
             string group_id; cin>>group_id;
            join_group(sock_fd,group_id);    
        }
		else if(command =="leave_group"){
             string group_id; cin>>group_id;
            leave_group(sock_fd,group_id);    
        }
        else if(command == "login"){
             cin>>user_id;
             cin>>passwd;
            login(sock_fd,user_id,passwd);
        }

        else if(command =="logout"){
            logout(sock_fd);
        }

        else if(command =="list_requests"){
             string group_id; cin>>group_id;
            list_requests(sock_fd,group_id);
        }

        else if(command == "accept_request"){
             string group_id,user_id; cin>>group_id>>user_id;
            accept_request(sock_fd,group_id,user_id);   
        }

        else if(command == "list_groups"){
            list_groups(sock_fd);
        }

        else if(command == "list_files"){
             string group_id; cin>>group_id;
            list_files(sock_fd,group_id);
        }

        else if(command == "connect"){
        count++;
             string IP,port; cin>>IP>>port;
            int sock_fd = makeConnectionToTracker(IP.c_str(),port.c_str());
            count++;
            if(send(sock_fd,"hello thread",sizeof "hello thread",0) == -1){
                printf("sending command create_user failed \n");
                close(sock_fd);
                count++;
                exit(1);
            }
            count++;
            dummyRecv(sock_fd);
        }

        else if(command == "download_file"){
             string group_id,file_name,destinationPath; cin>>group_id>>file_name>>destinationPath;
             fnameToPath[file_name]=group_id;
             string args = group_id.append(delim).append(file_name).append(delim).append(destinationPath)\
            .append(delim).append( to_string(sock_fd));
		
            char* stringa1 = (char*) malloc((args.size()+1)*sizeof(char));
		count++;
            for(int i=0;i<args.size();i++){
                stringa1[i] = args[i];
                count++;
            }
            stringa1[args.size()] = '\0';
		count++;
            printf("%s\n", stringa1);
             cout<<"Calling thread now"<< endl;
            pthread_t threadToDownloadFile;
            pthread_create(&threadToDownloadFile,NULL,downloadFile,stringa1);
        }
        else if(command=="show_downloads")
        {
            unordered_map<string,string>::iterator itr;
            if(fnameToPath.size()==0)
            {
                cout<<"Currently no file to display"<<endl;
            }
            for(itr=fnameToPath.begin();itr!=fnameToPath.end();itr++)
            {
                cout<<"[C] ["<<(*itr).second<<"] "<<(*itr).first<<endl;
            }
        }
        else if(command=="stop_share")
        {
            string group_id,filename;
            cin>>group_id>>filename;
            cout<<"Successfully stop share Filename: "<<filename<<" in group: "<<group_id<<endl;
        }
        else{
             cout<<"Not a valid command "<< endl;
             cin.clear();
             cout.clear();
        }
    }
    return 0;
}
