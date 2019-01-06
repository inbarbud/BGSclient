#include <stdlib.h>
#include "../include/connectionHandler.h"

//#include <iostream>
#include <thread>

using namespace std;



bool disconnected = false;
bool logoutReq = false;

void shortToBytes(short num, char* bytesArr)
{
    bytesArr[0] = ((num >> 8) & 0xFF);
    bytesArr[1] = (num & 0xFF);
}
short bytesToShort(char* bytesArr)
{
    short result = (short)((bytesArr[0] & 0xff) << 8);
    result += (short)(bytesArr[1] & 0xff);
    return result;
}

//TODO: parse func
void sendRegister(short opcode, string req, char buffer[],ConnectionHandler& c){
    shortToBytes(opcode,buffer);        //put opcode to buffer
    c.sendBytes(buffer,2);              //send
    //parse
    string username = req.substr(0,req.find(' '));
    string password = req.substr(req.find(' '));
    //send
    c.sendFrameAscii(username,'\0');
    c.sendFrameAscii(password,'\0');
}

void sendLogin(short opcode, string req, char buffer[],ConnectionHandler& c){
    shortToBytes(opcode,buffer);        //put opcode to buffer
    c.sendBytes(buffer,2);              //send
    //parse
    string username = req.substr(0,req.find(' '));
    string password = req.substr(req.find(' '));
    //send
    c.sendFrameAscii(username,'\0');
    c.sendFrameAscii(password,'\0');
}

void sendLogout(short opcode, char buffer[],ConnectionHandler& c){
    shortToBytes(opcode,buffer);        //put opcode to buffer
    c.sendBytes(buffer,2);              //send

    //req_length = 2;
}

void sendFollow(short opcode, string req, char buffer[],ConnectionHandler& c) {

    shortToBytes(opcode, buffer);        //put opcode to buffer
    c.sendBytes(buffer, 2);              //send
    //parse
    buffer[0] = req[0];               //followType
    c.sendBytes(buffer, 1);
    req = req.substr(2);
    int NumOfUsers = stoi(req);           //numOfUsers
    shortToBytes(NumOfUsers, buffer);
    c.sendBytes(buffer, 2);
    int k = req.find(' ');
    req = req.substr(k + 1);       //listOfUsers

    while (NumOfUsers > 0) {
        int i = req.find(' ');
        if (i != -1) {
            c.sendFrameAscii(req.substr(0, i), '\0');    //send a user
            NumOfUsers--;
            req = req.substr(i + 1);
        } else {
            c.sendFrameAscii(req, '\0');
            NumOfUsers--;
        }
    }
}

void sendPost(short opcode, string req, char buffer[],ConnectionHandler& c) {
    shortToBytes(opcode, buffer);        //put opcode to buffer
    c.sendBytes(buffer, 2);              //send
    //send
    c.sendFrameAscii(req, '\0');
}

void sendPM(short opcode, string req, char buffer[],ConnectionHandler& c) {
    shortToBytes(opcode,buffer);        //put opcode to buffer
    c.sendBytes(buffer,2);              //send
    //parse
    int i = req.find(' ');
    string username = req.substr(0,i);
    string content = req.substr(i+1);
    //send
    c.sendFrameAscii(username,'\0');
    c.sendFrameAscii(content,'\0');
}
void sendUserlist(short opcode, char buffer[],ConnectionHandler& c) {
    shortToBytes(opcode,buffer);        //put opcode to buffer
    c.sendBytes(buffer,2);              //send
}
void sendStat(short opcode, string req, char buffer[],ConnectionHandler& c) {
    shortToBytes(opcode, buffer);        //put opcode to buffer
    c.sendBytes(buffer, 2);              //send
    //send
    c.sendFrameAscii(req, '\0');
}

void sendAcknowledge(char buffer[],ConnectionHandler& c){
    cout << "ACK " ;
    c.getBytes(buffer,2);                            //read second opcode from buffer
    short ackType = bytesToShort(buffer);            //second opcode
    cout << ackType << " ";
    if (ackType==4 || ackType==7){                   //follow or userlist
        c.getBytes(buffer,2);
        short numOfUsers = bytesToShort(buffer);
        string numString = to_string(numOfUsers);
        cout << numString+" ";
        string user;
        while(numOfUsers>0)
        {
            c.getFrameAscii(user,'\0');
            cout << user.c_str()<<" ";
            user = "";
            numOfUsers--;
        }

    }
    else if(ackType==8){                            //stat
        //TODO:change
        c.getBytes(buffer,6); // read 3 couples of bytes;
        short NumPosts = bytesToShort(buffer);
        short NumFollowers= bytesToShort(buffer+2);
        short NumFollowing= bytesToShort(buffer+4);
        cout << NumPosts << " " <<NumFollowers << " " <<NumFollowing ;
    }
    else if(ackType==3){                            //logout
        disconnected=true;
    }
}



void sendNotification(char buffer[],ConnectionHandler& c){
    cout << "NOTIFICATION ";
    c.getBytes(buffer,1);                   //read messageType
    if (buffer[0]==0)
        cout << "PM ";
    else
        cout << "POST " ;
    string posting_user;
    c.getFrameAscii(posting_user,'\0');
    //TODO:?
    cout << posting_user.c_str() << " " ;
    string content;
    c.getFrameAscii(content,'\0');
    cout << content.c_str() << endl;

}

void sendError(char buffer[],ConnectionHandler& c){
    cout << "Error ";
    c.getBytes(buffer,2);
    short err_req = bytesToShort(buffer);
    if (err_req==3)                     //if logout request was errored
        logoutReq = false;
    cout << err_req  << endl;

}



class SocketListener{
private:
    ConnectionHandler &handler;
public:
    SocketListener(ConnectionHandler &connectionHandler) : handler(connectionHandler){}

    void run(){
        const short bufsize = 1024;
        char buf[bufsize];
        while(!disconnected){
            string answer;
            if (!handler.getLine(answer)) {
                cout << "Disconnected. Exiting...\n" << endl;
                disconnected = true;
                break;
            }
            short answerOpCode= bytesToShort(buf);
            //TODO:conditions?
            if(answerOpCode==10){
                sendAcknowledge(buf,handler);
            }
            else if(answerOpCode==9){
                sendNotification(buf,handler);
            }
            else if(answerOpCode==11){
                sendError(buf,handler);
            }
        }
    };

};


class KeyboardWriter {
private:
    ConnectionHandler &handler;
public:
    KeyboardWriter(ConnectionHandler &connectionHandler) : handler(connectionHandler) {}

    void run() {
        const short bufsize = 1024;
        char buf[bufsize];
        while (!disconnected) {
            string req;
            if (!logoutReq){
                getline(cin,req);
                //TODO:check if there is no ' '?
                string operation="";
                int temp = (int)req.find(' ');
                if(temp==-1)
                    operation=req;
                else
                    operation= req.substr(0,req.find(' '));

                if(operation.compare("REGISTER")==0){
                    sendRegister(1,req.substr(9),buf,handler);
                }
                else if(operation.compare("LOGIN")==0){
                    sendLogin(2,req.substr(6),buf,handler);
                }
                else if(operation.compare("LOGOUT")==0) {
                    sendLogout(3,buf,handler);
                    logoutReq = true;
                }
                else if(operation.compare("FOLLOW")==0) {
                    sendFollow(4,req.substr(7),buf,handler);
                }
                else if(operation.compare("POST")==0) {
                    sendPost(5,req.substr(5),buf,handler);
                }
                else if(operation.compare("PM")==0) {
                    sendPM(6,req.substr(3),buf,handler);
                }
                else if(operation.compare("USERLIST")==0) {
                    sendUserlist(7,buf,handler);
                }
                else if(operation.compare("STAT")==0) {
                    sendStat(8,req.substr(5),buf,handler);
                }
            }
        }
    };
};




/**
* This code assumes that the server replies the exact text the client sent it (as opposed to the practical session example)
*/
int main (int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " host port" << endl << endl;
        return -1;
    }

    ConnectionHandler connectionHandler(argv[1], atoi(argv[2]));                //(host, port)
    if (!connectionHandler.connect()) {
        cerr << "Cannot connect to " << argv[1] << ":" << atoi(argv[2]) << endl;
        return 1;
    }
    else{
        cout << "Connected" << endl;
    }

    KeyboardWriter keyboardWriter(connectionHandler);
    SocketListener socketListener(connectionHandler);

    thread t1 (&KeyboardWriter::run,&keyboardWriter);
    thread t2 (&SocketListener::run,&socketListener);


    t1.join();
    t2.join();
    return 0;
}





