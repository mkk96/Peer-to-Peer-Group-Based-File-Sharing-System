#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h> 
#include<sys/types.h> 
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<iostream>
#include<vector>
#include<pthread.h>
#include<fstream>
#include<fcntl.h>
#include<unordered_map>
#include<algorithm>
int count1=0;
#define MAXDATASIZE 100
using namespace std;

class torFile{
    public:
         string fileName;
         string peerAddr;
};

class Peer{
        public:
             string id;
             string password;
             string IP;
             string port;
            bool loggedIn = false;
            Peer( string id, string password, string ip, string port){
                this->id = id;
                this->password = password;
                this->IP = ip;
                this->port = port;
            }
             vector< pair< string, string>> groupJoinRequest; 
};

class PeerInfo{
    public:
         string user_id;
         string filePathInPeer;
};

class FileMetaData{
    public:
         string fileName;
         string fileSize;
         vector< string> hashOfChunks;
         vector<PeerInfo> clientsHavingThisFile;
};

class Group{
    private:
        pthread_mutex_t lockGroupFilesShared;
    public:
         string id;
         string ownwer;
         vector< string> members;
         unordered_map< string,FileMetaData*> filesShared;
        void updatePeerListInGroup( string fileName,PeerInfo peer){
            pthread_mutex_lock(&lockGroupFilesShared);
            this->filesShared[fileName]->clientsHavingThisFile.push_back(peer);
            pthread_mutex_unlock(&lockGroupFilesShared);
        };

        bool peerPresentInList( string fileName, string user_id){
            if(filesShared.find(fileName) == filesShared.end()){
                 cout<<"filename is not present in group "<<id<< endl;
                return false;
            }
            else{
                for(auto p : filesShared[fileName]->clientsHavingThisFile){
                    if(p.user_id == user_id)
                        return true;
                }
                return false;
            }
        }
};

 unordered_map< string,Peer*> peerMap;

 unordered_map< string,Group*> groupMap;

pthread_mutex_t lockPeerMap;
pthread_mutex_t lockGroupMap;

 string delim = ">>=";

 vector< string> getTokens( string input){
     vector< string> result;
    
    size_t pos = 0;
     string token;
    while((pos = input.find(delim))!= string::npos)
    {
        count1++;
        token = input.substr(0,pos);
        result.push_back(token);
        input.erase(0,pos+delim.length());
    }
     count1++;
    result.push_back(input);
    return result;
}

int makeServer( string ip, string port){
    int sock_fd;
    struct addrinfo hints, *res;

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET; // ipv4
    count1++;
    hints.ai_socktype = SOCK_STREAM; // for tcp

    if(getaddrinfo(ip.c_str(),port.c_str(),&hints,&res) != 0 ){
        printf("socket failed \n");
        count1++;
        exit(1);
        
    }
    sock_fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if(sock_fd <= 0){
        printf("socket failed \n");
        count1++;
        exit(1);
    }

    if(bind(sock_fd,res->ai_addr,res->ai_addrlen) == -1){
         cout<<"sock_fd is :"<<sock_fd<< endl;
         cout<<"ai_addr is "<<res->ai_addr<< endl;
        printf("bind failed \n");
        exit(1);
        count1++;
    }

    freeaddrinfo(res);

    if(listen(sock_fd,10)==-1){
        printf("listen failed \n");
        count1++;
        exit(1);
    }

    return sock_fd;
}

int dummySend(int new_fd)
{
    count1++;
    int dummySize = 10;
    char buf[dummySize] ={0};
    if(send(new_fd,buf,dummySize,0) == -1)
    {
        count1++;
        printf("sendind user_id failed \n");
        close(new_fd);
        exit(1);
    }
    count1++;
    return 0;
}

int dummyRecv(int sock_fd){
    int dummySize = 10;
    count1++;
    char buf[dummySize] = {0}; 
    int numbytes;
    count1++;
    if((numbytes = recv(sock_fd,buf,dummySize,0))==-1){ 
        printf("error reading data");
        exit(1);
    }
    return 0;
}

 string getStringFromSocket(int new_fd){
    char buf[MAXDATASIZE] = {0};
    int numbytes;
    count1++;
    if((numbytes = recv(new_fd,buf,MAXDATASIZE-1,0))==-1){
         cout<<"Error recieving string"<< endl;
        exit(1);
    }
    else if(numbytes == 0){
        return "connectionClosedBySender";
    }
     string recvString(buf);
    fflush(stdout);
    dummySend(new_fd);
    return recvString;
}

int acceptTorFileFromPeer(int new_fd){

     string peerAddr = getStringFromSocket(new_fd);
     cout<<"Recieving file from : "<<peerAddr<< endl;

     string fileName = getStringFromSocket(new_fd);
     cout<<"file name :"<<fileName<< endl;

     string filePath = getStringFromSocket(new_fd);
     cout<<"file path :"<<filePath<< endl;

     string count1HashBlocksStr = getStringFromSocket(new_fd);
     cout<<"Number of blocks :"<<count1HashBlocksStr<< endl;
    int count1HashBlocks =  stoi(count1HashBlocksStr);

     vector< pair<int, string>> hashBlocks;
    for(int i=0;i<count1HashBlocks;i++){
         string blockSizeStr = getStringFromSocket(new_fd);
        int blockSize =  stoi(blockSizeStr);

         string hash = getStringFromSocket(new_fd);
        hashBlocks.push_back({blockSize,hash});
    }

     cout<<"hash blocks recvd size is: "<<hashBlocks.size()<< endl;
    return 0;
}

 string updatePeerMap(Peer* peer, string user_id){
    pthread_mutex_lock(&lockPeerMap);
     string status;
    if(peerMap.find(user_id) == peerMap.end())
    {
        count1++;
        peerMap[user_id] = peer;
        status = "user created";
    }
    else
    {
        count1++;
        status ="userid exists";
        free(peer);
    }
    pthread_mutex_unlock(&lockPeerMap);
     cout<<"Number of users :"<<peerMap.size()<< endl;
    return status;
}

 string updateGroupMap(Group* group, string group_id){
    pthread_mutex_lock(&lockGroupMap);
     string status;
    if(groupMap.find(group_id) == groupMap.end())
    {
        count1++;
        groupMap[group_id] = group;
        status = "group created";
    }
    else
    {
        count1++;
        status ="group exists";
        free(group);
    }
    pthread_mutex_unlock(&lockGroupMap);
     cout<<"Number of groups :"<<groupMap.size()<< endl;
    return status;
}

int updatePeerMapLoginStatus( string user_id,bool b)
{
    pthread_mutex_lock(&lockPeerMap);
    peerMap[user_id]->loggedIn = b;
    pthread_mutex_unlock(&lockPeerMap);
    return 0;
}

int addGroupJoinReqToPeer( string owner, string user_id, string group_id){
    pthread_mutex_lock(&lockPeerMap);
    peerMap[owner]->groupJoinRequest.push_back({group_id,user_id});
    pthread_mutex_unlock(&lockPeerMap);
     cout<<"Added group join req to "<<owner<<" to allow "<<user_id<<" to join group "<<group_id<< endl;
    return 0;
}

int removeGroupJoinReq( string owner, string group_id, string user_id){
    pthread_mutex_lock(&lockPeerMap);
    auto& v = peerMap[owner]->groupJoinRequest;
    auto position =  find(v.begin(),v.end(), make_pair(group_id,user_id));
    if(position != v.end()){
        v.erase(position);
    }
    pthread_mutex_unlock(&lockPeerMap);
     cout<<"removed join req "<< endl;
    return 0;
}

int addMemberToGroup( string user_id, string group_id){
    pthread_mutex_lock(&lockGroupMap);
    auto group = groupMap[group_id];
    group->members.push_back(user_id);
    pthread_mutex_unlock(&lockGroupMap);
     cout<<"Members in group :"<<group_id<< endl;
    for(auto m:group->members)
         cout<<m<< endl;
    return 0;
}

bool isHashEqual( vector< string> h1, vector< string> h2)
{
    if(h1.size() != h2.size())
    {
        count1++;
        return false;
    }
    else
    {
        count1++;
        for(int i=0;i<h1.size();i++)
        {
            if(h1[i] != h2[i])
            {
                count1++;
                return false;
            }
        }
        return true;
    }
    count1++;
}

void upload_file_exitHelper(int new_fd){
     string sizePlusNoOfHashes = getStringFromSocket(new_fd);
     vector< string> tokens = getTokens(sizePlusNoOfHashes);
     string size = tokens[0];
    int noOfBlocks =  stoi(tokens[1]);

     vector< string> ignore;
    for(int i=0;i<noOfBlocks;i++){
        ignore.push_back(getStringFromSocket(new_fd));
    }

}
 string upload_file(int new_fd, string filePath, string group_id, string user_id){
     string status;
    if(user_id=="")
    {
        count1++;
        status = "First login to upload file";
        upload_file_exitHelper(new_fd);
    }
    else
    {
        count1++;
        if(groupMap.find(group_id) == groupMap.end())
        {
            status = "Group does not exist with group_id :";
            status.append(group_id);
            count1++;
            upload_file_exitHelper(new_fd);
        }
        else{
            Group* group = groupMap[group_id];
            auto x= find(group->members.begin(),group->members.end(),user_id);
            if(x==group->members.end())
            {
                count1++;
                status = "User does not belong to group ";status.append(group_id);
                upload_file_exitHelper(new_fd);
            }
            else{
                 string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);
                 string sizePlusNoOfHashes = getStringFromSocket(new_fd);
                 vector< string> tokens = getTokens(sizePlusNoOfHashes);
                 string size = tokens[0];
                int noOfBlocks =  stoi(tokens[1]);

                 vector< string> hashOfChunks;
                for(int i=0;i<noOfBlocks;i++){
                    hashOfChunks.push_back(getStringFromSocket(new_fd));
                     cout<<hashOfChunks[i]<< endl;
                }
                auto peer = PeerInfo();
                count1++;
                peer.filePathInPeer = filePath;
                peer.user_id = user_id;
                auto fileMetaData = new FileMetaData();
                count1++;
                fileMetaData->fileSize = size;
                fileMetaData->hashOfChunks = hashOfChunks;
                fileMetaData->clientsHavingThisFile.push_back(peer);
                count1++;
                fileMetaData->fileName = fileName;
                auto y = group->filesShared.find(fileName);
                if(y==group->filesShared.end())
                { 
                    count1++;
                    group->filesShared[fileName] = fileMetaData;
                    status = "File shared"; 
                }
                else
                {
                    count1++;
                    auto hashSaved = group->filesShared[fileName]->hashOfChunks;
                    if(!isHashEqual(hashSaved,hashOfChunks)){
                        status = "There exists a file with same name but diff hash(content). Change name for uploading";
                    }
                    else
                    {
                        count1++;
                        auto peersSharedThisFile = group->filesShared[fileName]->clientsHavingThisFile;
                        if(group->peerPresentInList(fileName,user_id)){ 
                            status =fileName;
                            count1++;
                            status.append("Already shared by this peer");
                            count1++;
                        }
                        else{
                            group->updatePeerListInGroup(fileName,peer);
                            if(true){}
                            status = "File shared";
                        }
                    }
                }
            }
        }
    }

    if(true&&send(new_fd,status.c_str(),status.size(),0) == -1)
    {
        count1++;
        printf("sending status signal failed in create group \n");
        close(new_fd);
        count1++;
        exit(1);
    }

    dummyRecv(new_fd);
     cout<<"end of upload file"<< endl;
    return status;
}

 string create_group(int new_fd, string user_id, string group_id){

    Group* group = new Group();
    group->id = group_id;
    group->ownwer = user_id;
    group->members.push_back(user_id);

     string status;
    if(user_id==""){
        status = "First login to create group";
        count1++;
        free(group);
    }
    else{
        count1++;
        status = updateGroupMap(group,group_id);
    }

    if(send(new_fd,status.c_str(),status.size(),0) == -1){
        count1++;
        printf("sending status signal failed in create group \n");
        close(new_fd);
        exit(1);
        count1++;
    }

    dummyRecv(new_fd);
     cout<<"end of create_group"<< endl;
    return status;
}

 string accept_request(int new_fd, string currUser, string group_id, string user_id){
     string status="";
    if(user_id=="")
    {
        count1++;
        status = "First login to list requests";
    }
    else
    {
        if(groupMap.find(group_id) == groupMap.end())
        {
            count1++;
            status = "Group does not exist with group_id :";
            status.append(group_id);
            removeGroupJoinReq(currUser,group_id,user_id);
            count1++;
        }
        else if(peerMap.find(user_id) == peerMap.end())
        {
            status = "User does not exist with user_id :";
            status.append(user_id);
            count1++;
            removeGroupJoinReq(currUser,group_id,user_id);
        }
        else{
            Group* group = groupMap[group_id];
            if(group->ownwer != currUser)
            {
                count1++;
                status="Only owner of group can accept request: Permission denied";
            }
            else
            {
                Group* group = groupMap[group_id];
                auto x= find(group->members.begin(),group->members.end(),user_id);
                count1++;
                if(x!=group->members.end())
                {
                    status = "User is already a member of group :";status.append(group_id);
                    removeGroupJoinReq(currUser,group_id,user_id);
                    count1++;
                }
                else
                {
                    addMemberToGroup(user_id,group_id);
                    status = user_id;
                    count1++;
                    status.append(" Added to group : ");status.append(group_id);
                    removeGroupJoinReq(currUser,group_id,user_id);
                }
            }
        }
    }
count1++;
    if(send(new_fd,status.c_str(),status.size(),0) == -1){
        printf("sending status signal failed in create group \n");
        close(new_fd);
        count1++;
        exit(1);
    }

    dummyRecv(new_fd);
     cout<<"end of accept request "<< endl;
    return status;
}

 string list_requests(int new_fd, string user_id, string group_id){
     string status="";
    if(user_id=="")
    {
        status = "First login to list requests";
        count1++;
    }
    else{
        if(groupMap.find(group_id) == groupMap.end())
        {
            status = "Group does not exist with group_id :";
            count1++;
            status.append(group_id);
        }
        else
        {
            Group* group = groupMap[group_id];
            if(group->ownwer != user_id)
            {
                status="Only owner can list request: Permission denied";
                count1++;
            }
            else
            {
                if(peerMap[user_id]->groupJoinRequest.size() == 0)
                {
                    status="No pending request";
                }
                else
                {
                    count1++;
                    status.append("Group").append("    ").append("User ID").append("\n");
                    for(auto r:peerMap[user_id]->groupJoinRequest)
                    {
                        count1++;
                        status.append(r.first).append("    ").append(r.second).append("\n");
                    }
                }
            }
        }
    }
    count1++;
    if(send(new_fd,status.c_str(),status.size(),0) == -1)
    {
        count1++;
        printf("sending status signal failed in list request \n");
        close(new_fd);
        exit(1);
        count1++;
    }

    dummyRecv(new_fd);
     cout<<"end of list request"<< endl;
    return status;
}

void getPeersWithFile_exitHelper(int new_fd){
     string peerListSizeStr = "0";
    peerListSizeStr.append(delim).append("0");
    count1++;
    if(send(new_fd,peerListSizeStr.c_str(),peerListSizeStr.size(),0) == -1)
    {
        printf("sending no of peers in get peers with file failed \n");
        close(new_fd);
        count1++;
        exit(1);
    }
    dummyRecv(new_fd);
}

 string getPeersWithFile(int new_fd, string group_id, string fileName, string currUser){
     string status;
     string noOfFilesStr = "0";
    if(currUser=="")
    {
        count1++;
        status = "First login to list requests";
        getPeersWithFile_exitHelper(new_fd);
    }
    else if (groupMap.find(group_id) == groupMap.end())
    {
        count1++;
            status = "Group does not exists ";
            getPeersWithFile_exitHelper(new_fd);
        }
    else{
        auto group = groupMap[group_id];
        count1++;
        auto x= find(group->members.begin(),group->members.end(),currUser);
        if(x==group->members.end()){
            count1++;
            status = "User does not belong to group ";status.append(group_id);
            getPeersWithFile_exitHelper(new_fd);
        }
        else{
            count1++;
            if(group->filesShared.find(fileName) == group->filesShared.end()){
                status = fileName;
                status.append(" is not shared in the group ").append(group_id);
                count1++;
                getPeersWithFile_exitHelper(new_fd);
            }
            else{
                 string peerListSizeStr = group->filesShared[fileName]->fileSize;
                count1++;
                peerListSizeStr.append(delim).append( to_string(group->filesShared[fileName]->clientsHavingThisFile.size()));
                status = "sending no of peers :";status.append( to_string(group->filesShared[fileName]->clientsHavingThisFile.size()));
                count1++;
                if(send(new_fd,peerListSizeStr.c_str(),peerListSizeStr.size(),0) == -1){
                    count1++;
                    printf("sending no of peers in get peers with file failed \n");
                    close(new_fd);
                    count1++;
                    exit(1);
                }
                dummyRecv(new_fd);

                for(auto peer: group->filesShared[fileName]->clientsHavingThisFile){
                     cout<<"Sending user id "<<peer.user_id<< endl;
                    count1++;
                    if(send(new_fd,peer.user_id.c_str(),peer.user_id.size(),0) == -1){
                        printf("sending user id in get peers with files failed \n");
                        close(new_fd);
                        count1++;
                        exit(1);
                    }
                    dummyRecv(new_fd);

                     string address = peerMap[peer.user_id]->IP;
                    address.append(":").append(peerMap[peer.user_id]->port);
                    count1++;
                     cout<<"Sending address in peer "<<address<< endl;

                    if(send(new_fd,address.c_str(),address.size(),0) == -1){
                        count1++;
                        printf("sending address in get peers with file failed \n");
                        close(new_fd);
                        count1++;
                        exit(1);
                    }
                    dummyRecv(new_fd);

                     cout<<"Sending file path "<<peer.filePathInPeer<< endl;

                    if(send(new_fd,peer.filePathInPeer.c_str(),peer.filePathInPeer.size(),0) == -1)
                    {
                        count1++;
                        printf("sending file path in get peers with files failed \n");
                        close(new_fd);
                        exit(1);
                    }
                    count1++;
                    dummyRecv(new_fd);
                    
                }
            }
        }
    }
count1++;
    if(send(new_fd,status.c_str(),status.size(),0) == -1){
        printf("sending status in get peers with files  \n");
        count1++;
        close(new_fd);
        exit(1);
    }
    dummyRecv(new_fd);    
    
     cout<<"end of get peers with files"<< endl;
    return status;
}

 string list_files(int new_fd, string group_id){
     string status;
     string noOfFilesStr = "0";
    if(groupMap.find(group_id) == groupMap.end()){
        count1++;
        status = "Group does not exist with group_id :";
        status.append(group_id);

        if(send(new_fd,noOfFilesStr.c_str(),noOfFilesStr.size(),0) == -1){
            count1++;
            printf("sending no of files in list files failed \n");
            close(new_fd);
            exit(1);
        }
        count1++;
        dummyRecv(new_fd);
    }
    else{
        auto group = groupMap[group_id];
        count1++;
        int noOfFiles = group->filesShared.size();
        noOfFilesStr =  to_string(noOfFiles);
         cout<<"Sending no of files :"<<noOfFilesStr<< endl;
        count1++;
        status = "No of files sent is :";status.append(noOfFilesStr);
        if(send(new_fd,noOfFilesStr.c_str(),noOfFilesStr.size(),0) == -1){
            count1++;
            printf("sending no of files in list files failed \n");
            close(new_fd);
            count1++;
            exit(1);
        }
        dummyRecv(new_fd);

        for(auto f : group->filesShared){
             cout<<"Sending file name :"<<f.first<< endl;
            if(send(new_fd,f.first.c_str(),f.first.size(),0) == -1){
                count1++;
                printf("sending file names in list files failed \n");
                close(new_fd);
                count1++;
                exit(1);
            }
            dummyRecv(new_fd);
        }
    }

    if(send(new_fd,status.c_str(),status.size(),0) == -1){
        count1++;
        printf("sending status in list group \n");
        close(new_fd);
        exit(1);
    }
    dummyRecv(new_fd);

     cout<<"end of list files"<< endl;
    return status;
}

 string list_groups(int new_fd){
     string status;
    if(groupMap.size() == 0)
    {
        status = "No groups created yet ";
    }
    else
    {
        count1++;
        for(auto g : groupMap){
            status.append(g.first).append("\n");
        }
        count1++;
    }
    if(send(new_fd,status.c_str(),status.size(),0) == -1){
        printf("sending status in list group \n");
        close(new_fd);
        exit(1);
    }

    dummyRecv(new_fd);
     cout<<"end of list group"<< endl;
    return status;
}

 string join_group(int new_fd, string user_id, string group_id){
     string status;
    if(user_id==""){
        count1++;
        status = "First login to join group";
    }
    else{
        if(groupMap.find(group_id) == groupMap.end()){
            count1++;
            status = "Group does not exist with group_id :";
            status.append(group_id);
        }
        else{
            count1++;
            Group* group = groupMap[group_id];
            auto x= find(group->members.begin(),group->members.end(),user_id);
            if(x!=group->members.end()){ 
                count1++;
                status = "User is already a member of group :";status.append(group_id);
            }
            else{
                count1++;
                addGroupJoinReqToPeer(group->ownwer,user_id,group_id);
                status = "Requested group owner to join group : ";status.append(group_id);
            }
        }
        count1++;
    }

    if(send(new_fd,status.c_str(),status.size(),0) == -1){
        count1++;
        printf("sending status in join group \n");
        close(new_fd);
        exit(1);
    }

    dummyRecv(new_fd);
     cout<<"end of join_group"<< endl;
    return status;
}
string leave_group(int new_fd, string user_id, string group_id){
     string status;
    if(user_id==""){
        count1++;
        status = "First login to leave group";
    }
    else{
        if(groupMap.find(group_id) == groupMap.end()){
            count1++;
            status = "Group does not exist with group_id :";
            status.append(group_id); 
        }
        else{
            count1++;
            Group* group = groupMap[group_id];
            auto x= find(group->members.begin(),group->members.end(),user_id);
            if(x==group->members.end()){ 
                count1++;
                status = "User is not a member of group :";status.append(group_id);
            }
            else if(group->ownwer==user_id)
            {
                cout<<"User is owner of group so cannot leave group" <<endl;
                status = "User is owner of group so cannot leave group ";status.append(group_id);
            }
            else{
                count1++;
                pthread_mutex_lock(&lockGroupMap);
                group->members.erase(x);
                pthread_mutex_unlock(&lockGroupMap);
				 cout<<"Leave group User: "<<user_id<<" from Group: "<<group_id<< endl;
                status = "Leave Request Sucessfully ";status.append(group_id);
            }
        }
        count1++;
    }

    if(send(new_fd,status.c_str(),status.size(),0) == -1){
        count1++;
        printf("sending status in leave group \n");
        close(new_fd);
        exit(1);
    }

    dummyRecv(new_fd);
     cout<<"end of leave_group"<< endl;
    return status;
}
int create_user(int new_fd, string user_id, string passwd, string IP, string port){
    Peer* peer = new Peer(user_id,passwd,IP,port);

     string status = updatePeerMap(peer,user_id);
     cout<<"status :"<<status<< endl;

    if(send(new_fd,status.c_str(),status.size(),0) == -1){
        printf("sending success signal failed in create user \n");
        close(new_fd);
        exit(1);
    }

    dummyRecv(new_fd);
     cout<<"end of create_user"<< endl;
    return 0;
}

int logout(int new_fd, string user_id){
    updatePeerMapLoginStatus(user_id,false);
     string retStrPeer = "Logged out ";
    if(send(new_fd,retStrPeer.c_str(),retStrPeer.size(),0) == -1)
    {
        count1++;
        printf("sending status signal failed in login \n");
        close(new_fd);
        exit(1);
    }

    dummyRecv(new_fd);
    return 0;
}

 string login(int new_fd, string user_id, string passwd, string& currUser){
     string retStrPeer = "";
    if(peerMap.find(user_id) == peerMap.end()){
        count1++;
        retStrPeer = "User does not exists create one";
    }
    else if(peerMap[user_id]->password != passwd){
        count1++;
        retStrPeer = "Wrong Password";
    }
    else{
        retStrPeer = "login success";
        count1++;
        updatePeerMapLoginStatus(user_id,true);
        currUser = user_id;
    }

    if(send(new_fd,retStrPeer.c_str(),retStrPeer.size(),0) == -1){
        count1++;
        printf("sending status signal failed in login \n");
        close(new_fd);
        exit(1);
    }
    count1++;
    dummyRecv(new_fd);
    return retStrPeer;
} 


void* serviceToPeer(void* i){
    int new_fd = *((int *)i);
    count1++;
    free(i); 
     string currUser = "";

    while(1){
         string stringFromPeer = getStringFromSocket(new_fd);
         vector< string> tokens = getTokens(stringFromPeer);
         string command = tokens[0];
         cout<<"command is "<<command<< endl;
        
        if(command == "upload_file"){
            if(tokens.size() != 3){
                 cout<<"wrong format for upload_file "<< endl;
            }
            else{
                 cout<<"file_upload called "<< endl;
                 string status = upload_file(new_fd,tokens[1],tokens[2],currUser);
                 cout<<status<< endl;
            }
        }

        else if(command == "create_user"){
            if(tokens.size() != 5){
                 cout<<"wrong format for create user "<< endl;
            }
            else{
                 cout<<"create user called "<< endl;
                create_user(new_fd,tokens[1],tokens[2],tokens[3],tokens[4]);
                count1++;
            }
        }

        else if(command == "login"){
            if(tokens.size() != 3){
                 cout<<"wrong format for login "<< endl;
            }
            else{
                if(currUser != ""){
                     cout<<"Already logged in as :"<<currUser;
                }
                else{
                     string status = login(new_fd,tokens[1],tokens[2],currUser);
                     cout<<status<< endl;
                }
            }
        }

        else if(command == "logout"){
            if(tokens.size() != 1){
                 cout<<"wrong format for logout "<< endl;
            }
            else{
            if(currUser == "")
                 cout<<"Didn't login to logout :"<<currUser;
            else{
                int status = logout(new_fd,currUser);
                if(status == 0){
                     cout<<"Logged out as : "<<currUser<< endl;
                    currUser = "";
                    }
                }
            }
        }

        else if(command == "create_group"){
             string status;
            if(tokens.size() != 2){
                 cout<<"wrong format for create_group "<< endl;
            }
            else{
                
                 cout<<"create group called "<< endl;
                status = create_group(new_fd,currUser,tokens[1]);
            }
             cout<<status<< endl;
        }

        else if(command == "join_group"){
            if(tokens.size() != 2 ){
                 cout<<"wrong format for join group "<< endl;
            }
             cout<<join_group(new_fd,currUser,tokens[1])<< endl;
        }
        else if(command == "leave_group"){
            if(tokens.size() != 2 ){
                 cout<<"wrong format for leave group "<< endl;
            }
             cout<<leave_group(new_fd,currUser,tokens[1])<< endl;
        }
        else if(command == "list_groups"){
            if(tokens.size() != 1 ){
                 cout<<"wrong format for list group "<< endl;
            }
             cout<<list_groups(new_fd)<< endl;
        }

        else if(command == "list_files"){
            if(tokens.size() != 2 ){
                 cout<<"wrong format for list files "<< endl;
            }
             cout<<list_files(new_fd,tokens[1])<< endl;
        }

        else if(command == "getPeersWithFile"){
            if(tokens.size() != 3 ){
                 cout<<"wrong format for getPeersWithFils "<< endl;
            }
             cout<<getPeersWithFile(new_fd,tokens[1],tokens[2],currUser)<< endl;
        }

        else if(command == "list_requests"){
            if(tokens.size() != 2 ){
                 cout<<"wrong format for lists requests "<< endl;
            }
             cout<<list_requests(new_fd,currUser,tokens[1])<< endl;
        }

        else if(command == "accept_request"){
            if(tokens.size() != 2 ){
                 cout<<"wrong format for accept requests "<< endl;
            }
             cout<<accept_request(new_fd,currUser,tokens[1],tokens[2])<< endl;
        }

        else if(command == "connectionClosedBySender"){
             cout<<"Connection closed by a peer"<< endl;
            break;
        }

        else{
             cout<<"Not a valid command"<< endl;
             string ignore;
             getline( cin,ignore);
        }
    }
    close(new_fd);
}

int main(int argc,char *argv[]){
    if(argc != 2){
        printf("./tracker tracker_info.txt");
        exit(1);
    }

     string tracker1,tracker2;
     ifstream MyReadFile(argv[1]);

    getline(MyReadFile,tracker1);
     string tracker1Port = tracker1.substr(tracker1.find_last_of(":") + 1);
     string tracker1IP = tracker1.substr(0,tracker1.find_last_of(":"));

    getline(MyReadFile,tracker2);
     string tracker2Port = tracker2.substr(tracker1.find_last_of(":") + 1);
     string tracker2IP = tracker2.substr(0,tracker1.find_last_of(":"));

    int sock_fd = makeServer(tracker1IP,tracker1Port);

    int new_fd;
    count1++;
    while(1){
        struct sockaddr_storage client_address;
        socklen_t sin_size = sizeof client_address;
        count1++;
        new_fd = accept(sock_fd,(struct sockaddr *)&client_address,&sin_size);
        if(new_fd == -1){
            perror("accept");
            count1++;
            continue;
        }
        char s[INET6_ADDRSTRLEN];
        count1++;
        inet_ntop(client_address.ss_family,&((struct sockaddr_in *)&client_address)->sin_addr,s,sizeof s);
        printf("Server got connection from %s\n",s);
        count1++;
        int* fd =(int *)malloc(sizeof(*fd));
        if(fd == NULL){
            count1++;
             cout<<"Malloc failed for creating thread "<< endl;
            close(new_fd);
            exit(1);
        }

        *fd = new_fd;
        pthread_t threadForServiceToPeer;
        pthread_create(&threadForServiceToPeer,NULL,serviceToPeer,fd);
    }
    close(new_fd);
}
