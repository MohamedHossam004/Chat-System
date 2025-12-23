#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<iostream>
#include <string>
#include <thread>
#include<ws2tcpip.h>
#include <atomic>
#pragma comment (lib , "ws2_32.lib")

using namespace std;

atomic<bool> running(true);

void receiverTH(SOCKET clientSock) {
	char buffer[4096];

	while (running) {
		ZeroMemory(buffer, 4096);

		int received = recv(clientSock, buffer, 4096, 0);

		if (received <= 0) {
			cout << "\nDisconnected from server.\n" << endl;
			running = false;
			break;
		}


		string message(buffer, 0, received);
		cout << "\r" << message << endl;

		cout << "> " << flush;

	}
	closesocket(clientSock);
}
int main() {
	string ipadd = "127.0.0.1";
	int port = 2005;

	WSADATA wsaData;
	int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != 0) {
		cerr << "WSAStartup failed! Error code: " << res << endl;
		return 0;
	}
	cout << "WSAStartup success! Network library is ready" << endl;

	SOCKET Csock = socket(AF_INET, SOCK_STREAM, 0);
	if (Csock == INVALID_SOCKET) {
		cerr << "Can't create a socket" << endl;
		return 0;
	}
	cout << "Socket creating successfully" << endl;

	sockaddr_in serverHint;
	serverHint.sin_family = AF_INET;
	serverHint.sin_port = htons(port);
	inet_pton(AF_INET, ipadd.c_str(), &serverHint.sin_addr);

	int connRes = connect(Csock, (sockaddr*)&serverHint, sizeof(serverHint));
	if (connRes == SOCKET_ERROR) {
		cerr << "can't connect to server .. Error : " << WSAGetLastError() << endl;
		closesocket(Csock);
		WSACleanup();
		return 0;
	}


	cout << "Connected! please enter your name: ";
	string name;
	getline(cin, name);
	send(Csock, name.c_str(), (int)(name.size() + 1), 0);
	cout << "Welcome " << name << "...!" << '\n';

	thread TH(receiverTH, Csock);


	string userInput;
	while (true) {
		cout << "> ";
		getline(cin, userInput);

		if (userInput == "quit") {
			cout << "Exiting chat..." << endl;
			running = false;
			closesocket(Csock);
			break;
		}



		if (!userInput.empty()) {
			int res = send(Csock, userInput.c_str(), (int)(userInput.size() + 1), 0);
			if (res == SOCKET_ERROR) {
				cerr << "Send failed. Error #" << WSAGetLastError() << endl;
				running = false;
				closesocket(Csock);
				break;
			}
		}
	}

	if (TH.joinable()) {
		TH.join();
	}

	WSACleanup();
	return 0;
}
