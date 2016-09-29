#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<iostream>
#include<WinSock2.h>
#define HAVE_REMOTE
#include<string>
#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<sstream>
#include<iomanip>
#include<ws2tcpip.h>
#include<thread>
#include<mutex>
using namespace std;
#define MAXBUF 65000
#define IN_PORT 8080
#define OUT_PORT 80
#pragma comment(lib, "ws2_32.lib")		//winsock2

string chTost(char *c);
string recv_Msg(SOCKET sock, bool is_Host);
void SetAddr(struct sockaddr_in *addr, ADDRESS_FAMILY sin_family, ULONG IP, int port);
void ReplaceAll(string& strSrc, const string& strFind, const string& strDest);
void Request_Msg(SOCKET csock, bool is_Host);
void Send_Msg(string tmpMsg, bool is_Host);
void GetHostAddr(string Msg);
void Error(char *err_ch);

struct addrinfo *p;
SOCKET recvServer, ssock, csock;
char *getIP = (char *)malloc(4);
int port = OUT_PORT;
std::mutex mtx;

int main() {
	WSADATA wsaData;
	int nErrorStatus;
	nErrorStatus = WSAStartup(0x0202, &wsaData);
	struct sockaddr_in server_addr;
	
		if ((ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
			Error("Socket");

		SetAddr(&server_addr, AF_INET, htonl(INADDR_ANY), IN_PORT);
		if (::bind(ssock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
			Error("Bind");
		if (listen(ssock, 10) < 0)
			Error("Listen");
		while (1) {
			if ((csock = accept(ssock, NULL, NULL)) != INVALID_SOCKET) 	cout << "Start Proxy Server\n";
			else Error("Accept");
			Request_Msg(csock, TRUE);
			Request_Msg(recvServer, FALSE);
			//	thread t1(Request_Msg, csock, TRUE);	//클라이언트 -> 서버
			//	thread t2(Request_Msg, csock, FALSE);	//서버 -> 클라이언트
			//	t2.join();
			//	t1.join();
		}
	free(getIP); closesocket(recvServer); closesocket(ssock); closesocket(csock); WSACleanup();
	return 0;
}


void Request_Msg(SOCKET csock, bool is_Host) {
	string ch;
	SOCKET sock = csock;

		if (is_Host == TRUE) {
			ch = recv_Msg(sock, is_Host);
			GetHostAddr(ch);
			Send_Msg(ch, is_Host);
		}
		else {
			ch = recv_Msg(recvServer, is_Host);
			Send_Msg(ch, is_Host);

	}
}
string recv_Msg(SOCKET sock, bool is_Host) {
	string ch;
	ch.clear();
	char buf[MAXBUF] = { 0, };
	memset(buf, 0x00, MAXBUF);
	mtx.lock();
	if (recv(sock, buf, MAXBUF, 0) > 0) {
		ch += buf;
		cout << ch << endl;
	}
	else  Error("Request_Msg Recieve");
	if (is_Host == FALSE) {
		if (ch.find("hacking") != string::npos)		ReplaceAll(ch, "hacking", "ABCDEF");
		if (send(csock, ch.c_str(), ch.length(), 0) == SOCKET_ERROR) Error("send ");
	}
	mtx.unlock();
	return ch;
}
void GetHostAddr(string Msg) {
	struct addrinfo addrInfo, *paddrInfo;
	SOCKADDR_IN *pSockAddr;
	string tmpMsg = Msg;
	string hostname = "";
	if (tmpMsg.find("Host: ") != string::npos) {
		hostname = tmpMsg.substr(tmpMsg.find("Host: ") + 6, tmpMsg.find("\r") - 1);
		hostname.erase(hostname.find("\r"), hostname.back());
		if (hostname.find(':') != string::npos) {
			port = ntohs(stoi(hostname.substr(hostname.find(':') + 1, hostname.back())));
			hostname = hostname.substr(0, hostname.find(':'));
		}
		ZeroMemory(&addrInfo, sizeof(addrInfo));
		addrInfo.ai_family = AF_INET;
		addrInfo.ai_socktype = SOCK_STREAM;
		addrInfo.ai_protocol = IPPROTO_TCP;
		if (getaddrinfo(hostname.c_str(), "0", &addrInfo, &paddrInfo) == 0) {
			pSockAddr = (SOCKADDR_IN *)paddrInfo->ai_addr;
			getIP = inet_ntoa(pSockAddr->sin_addr);
			cout << ">> IP : " << getIP << endl;
		}
		freeaddrinfo(paddrInfo);
	}
}
void Send_Msg(string tmpMsg, bool is_Host) {
	string msg = tmpMsg;
	int msgLen = msg.length();
	struct sockaddr_in hostaddr;
	if (is_Host == TRUE) {
		SetAddr(&hostaddr, AF_INET, inet_addr(getIP), OUT_PORT);
		if ((recvServer = socket(AF_INET, SOCK_STREAM, 0)) < 0) Error("Send_Msg Socket");
		if (connect(recvServer, (struct sockaddr *)&hostaddr, sizeof(hostaddr)) != INVALID_SOCKET) {
			if (send(recvServer, msg.c_str(), msgLen, 0) == SOCKET_ERROR) Error("Send ");
			else cout << ">> Packet Send!\n";
		}
		else Error("Connect");
	}
}
void SetAddr(struct sockaddr_in *addr, ADDRESS_FAMILY sin_family, ULONG IP, int port) {
	ZeroMemory(addr, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = IP;
	addr->sin_port = htons(port);
}
string chTost(char *c) {
	stringstream s;
	s << c;
	return s.str();
}
void ReplaceAll(string& strSrc, const string& strFind, const string& strDest){
	size_t a;
	while ((a = strSrc.find(strFind)) != string::npos) strSrc.replace(a, strFind.length(), strDest);
}
void Error(char *err_ch) {
	cout << err_ch << " Error : " << WSAGetLastError() << endl;
	exit(1);
}