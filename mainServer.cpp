#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<iostream>
#include<WS2tcpip.h>
#include<vector>
#include<thread>
#include<mutex>
#include <algorithm>
#include<map>
#pragma comment (lib , "ws2_32.lib")
using namespace std;

map <SOCKET , string> clients;

//mtx = mutex
mutex mtx;


void Broadcast(const string& message, SOCKET sender) {
	lock_guard<mutex> lock(mtx);
	string finalMsg;

	if (sender == INVALID_SOCKET) {
		finalMsg = message;
	}
	else {
		string senderName = clients[sender];
		finalMsg = senderName + ": " + message;
	}

	for (auto const& [sock, name] : clients) {
		if (sock != sender) {
			send(sock, finalMsg.c_str(), (int)(finalMsg.size() + 1), 0);
		}
	}
	
}




// clientTH = client Thread
void clientTh(SOCKET clientSock) {

	char buffer[4096];
	string username;

	ZeroMemory(buffer, 4096);
	int bytesReceived = recv(clientSock, buffer, 4096, 0);

	if (bytesReceived > 0) {
		username = string(buffer, 0, bytesReceived);

		mtx.lock();
		clients[clientSock] = username;
		mtx.unlock();

		string joinMsg = "[Server]: " + username + " has joined the chat!";
		cout << joinMsg << endl;
		Broadcast(joinMsg, INVALID_SOCKET);
	}
	else {
		closesocket(clientSock);
		return;
	}

	while (true) {
		ZeroMemory(buffer, 4096);

		int received = recv(clientSock, buffer, 4096, 0);
		// تعديل بسيط عشان لو رجع -1 كدا ف Error لكن لو رجع 0 كده الclient فصل هنتعامل مع الاتنين مش واحدة بس
		if (received <= 0) {
			string leaveMsg = "[Server]: " + username + " has left the chat.";
			cout << leaveMsg << endl;
			Broadcast(leaveMsg, INVALID_SOCKET);
			break;
		}


		string message(buffer, 0, received);
		cout << "Broadcasting : " << message << endl;

		Broadcast(message, clientSock);

	}
	closesocket(clientSock);



	lock_guard<mutex> lock(mtx);
	clients.erase(clientSock);



}
int main() {
	WSADATA wsaData;
	int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != 0) {
		cerr << "WSAStartup failed! Error code: " << res << endl;
		return 0;
	}
	cout << "WSAStartup success! Network library is ready" << endl;

	//Ssock = server socket
	SOCKET Ssock = socket(AF_INET, SOCK_STREAM, 0);
	if (Ssock == INVALID_SOCKET) {
		cerr << "Can't creat a socket" << endl;
		return 0;
	}
	cout << "Socket creating successfully" << endl;

	sockaddr_in ser;
	ser.sin_family = AF_INET;
	ser.sin_addr.s_addr = INADDR_ANY;
	ser.sin_port = htons(2005);
	if (bind(Ssock, (sockaddr*)&ser, sizeof(ser)) == SOCKET_ERROR) {
		cerr << "Bind falling";
		closesocket(Ssock);
		WSACleanup();
		return 0;
	}
	cout << "Bind success .. ";


	if (listen(Ssock, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "listening falling";
		closesocket(Ssock);
		WSACleanup();
		return 0;
	}
	cout << "Listening success… waiting for clients to connect..." << endl;

	while (true) {
		sockaddr_in clint;
		//clientS = client size
		int clientS = sizeof(clint);
		//Csock = client socket
		SOCKET Csock = accept(Ssock, (sockaddr*)&clint, &clientS);
		if (Csock != INVALID_SOCKET) {
			char host[NI_MAXHOST];
			char service[NI_MAXSERV];

			ZeroMemory(host, NI_MAXHOST);
			ZeroMemory(service, NI_MAXSERV);

			if (getnameinfo((sockaddr*)&clint, sizeof(clint), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
				cout << "Client connected: " << host << " on port " << service << endl;
			}
			else {
				cout << "Client connected: " << inet_ntoa(clint.sin_addr)
					<< " on port " << ntohs(clint.sin_port) << endl;
			}


			cout << "New client connected .. !" << '\n';

			//TH = thread
			thread TH(clientTh, Csock);

			TH.detach();
		}

	}

	WSACleanup();
}
