//dla planszy 3 xClient 3

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cstdlib>  
#include <signal.h>  
#include <fstream>
#include <ctime>

#pragma comment (lib, "Ws2_32.lib")
#define MAX_BUF_LEN 10
#define DEFAULT_PORT "27015"

using namespace std;
char board[10];
int res;
SOCKET clientSock = INVALID_SOCKET;

void saveResults(char winner){
	fstream fileHandler;
    time_t timeVal;
	string str;
	
	if (winner == 'x' || winner == 'o'){
		str = "-> wygrana ";
		str += winner;
		str += "\n";
	}
	else 
		str = "-> remis\n";
	
    fileHandler.open("results.txt", ios::app);
    if (fileHandler.good() != true)
        cout << "file error" << endl;
	
    time(&timeVal);
    fileHandler << ctime(&timeVal) << str << endl;	
	fileHandler.close();
}

void readResults(){
	string line;
    fstream fileToRead;
	
    fileToRead.open("results.txt", ios::in);
    if (fileToRead.good() != true)
        cout << "file error" << endl;
	
    while (getline(fileToRead, line))
        cout << line << endl;

    fileToRead.close();
}

void sigHandler(int sig){
	if (sig == SIGINT){
		cout << "Otrzymano SIGINT, konczymy." << endl;
		res = shutdown(clientSock, SD_SEND);
		if (res == SOCKET_ERROR) {
			cout << "shutdown error: " <<  WSAGetLastError();
			closesocket(clientSock);
			WSACleanup();
			exit(1);
		}
    
		// cleanup
		closesocket(clientSock);
		WSACleanup();
		exit(0);
	}
}

void showHistory(){
	char answer;
	
	cout << "Pokazac historie?" << endl;
	cout << "Tak - t, nie - n" << endl;
	cin >> answer;
	if (answer == 't')
		readResults();
}

void printBoard() {
	system("CLS");
	cout << endl;
	for(int i = 1; i < 10; i++) {
		cout << " " << board[i] << " ";
		if(i % 3)
		  cout << "|";
		else if(i != 9)
		  cout << "\n---+---+---\n";
		else 
			cout << "\n";
	}
	cout << endl;
}

bool isFreePlace(int x, int y){
	if (board[x + (y - 1) * 3] == ' ')
		return true;
	return false;	
}

bool isOnBoard(int x, int y){
	if ((x >= 1 && x <= 3) && (y >= 1 && y <= 3))
		return true;
	return false;
}

bool isOver(char ch, int x, int y){
	int xy = x + (y - 1) * 3;
	if (((board[1] == ch) && (board[5] == ch) && (board[9] == ch)) || ((board[3] == ch) && (board[5] == ch) && (board[7] == ch)))
		return true;
	
	for (int i = 1; i < 4; i++)
		if ((board[i] == ch) && (board[i+3] == ch) && (board[i+6] == ch))
			return true;
	for (int i = 1; i < 8; i+=3)
		if ((board[i] == ch) && (board[i+1] == ch) && (board[i+2] == ch))
			return true;

	return false;
}

void serverXYparse (int &xServer, int &yServer, string &xServerStr, string &yServerStr, string &xyServerStr){
	bool isFree = false;
	xServer = 0;
	yServer = 0;
	while (isOnBoard(xServer, yServer) != true || isFree != true ){	
		cout << "Wprowadz wspolrzedne: " << endl;
		cout << "x: " << endl;
		cin >> xServerStr;	
		if (xServerStr == "x")
			break;
		cout << "y: " << endl;
		cin >> yServerStr;
		if (yServerStr == "x")
			break;
		
		xServer = atoi(xServerStr.c_str());
		yServer = atoi(yServerStr.c_str());
		isFree = isFreePlace(xServer, yServer);
	}	
}

//0 - zle wspolrzedne, 1 - dobre
bool clientXYparse (int x, int y){
	if ((isOnBoard(x, y) == true) && (isFreePlace(x, y) == true))
		return true;
	return false;
}



int __cdecl main(void) 
{
	bool clientsXYaccepted = false;
	int freePlacesAveliable = 9;
	string xServerStr, yServerStr, xyServerStr;
	int xServer, yServer;
	char *sendbuf;
	
	int xClient, yClient, xyClient;								
	bool end = false;
	const char *endingMessage = "e"; 			//wiadomosc do klienta na zakonczenie
	const char *wrongArgsMessage = "w";			//wiadomosc do klienta, ze podal zle argumenty
	const char *okMessage = "k";				//jesli podane przez klienta wspolrzedne sa ok
	const char *winOMessage = "o";
	const char *argsMessage;
	
	WSADATA wsaData;
    int sendRes;
    SOCKET listenSock = INVALID_SOCKET;
    struct addrinfo *result = NULL;
    struct addrinfo hints;
    char recvbuf[MAX_BUF_LEN];
    int recvbuflen = MAX_BUF_LEN;
	
	for (int i = 1; i < 10; i++)
		board[i] = ' ';
	printBoard();
    
    //inicjalizowanie Winsock
    res = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (res != 0) {
        cout << "WSAStartup error: " <<  res << endl;
        return 1;
    }
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    res = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( res != 0 ) {
        cout << "getaddrinfo error: " << res << endl;
        WSACleanup();
        return 1;
    }

    //tworzenie
    listenSock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSock == INVALID_SOCKET) {
        cout << "socket error: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    //TCP listening socket
    res = bind(listenSock, result->ai_addr, (int)result->ai_addrlen);
    if (res == SOCKET_ERROR) {
        cout << "bind error: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    res = listen(listenSock, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        cout << "listen error: " << WSAGetLastError() << endl;
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    clientSock = accept(listenSock, NULL, NULL);
    if (clientSock == INVALID_SOCKET) {
        cout << "accept error: " << WSAGetLastError() << endl;
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }
    closesocket(listenSock);
	
	signal(SIGINT, sigHandler);
	
	cout << "Wprowadz wspolrzedne x i y = 1, 2 lub 3. Aby wyjsc z gry wprowadz x." << endl;
	cout << "Najpierw wspolrzedne wprowadza klient." << endl;

    do {
        res = recv(clientSock, recvbuf, recvbuflen, 0);
        if (res > 0) {
			while (clientsXYaccepted != true){
				if (res > 0){
					xyClient = atoi(recvbuf);
					xClient = xyClient / 10;
					yClient = xyClient % 10;
					
					//cout << "xClient: " << xClient << ", yClient: " << yClient << endl;
					if (clientXYparse(xClient, yClient) == true){
						clientsXYaccepted = true;
						board[xClient + (yClient - 1) * 3] = 'o';
						printBoard();
						
						//wysylamy ok do klienta - wpolrzedne zaakceptowane
						res = send(clientSock, okMessage, res, 0);
						if (res == SOCKET_ERROR) {
							cout << "send error: " << WSAGetLastError() << endl;
							closesocket(clientSock);
							WSACleanup();
							return 1;
						}

						//zmniejszamy liczbe dostepnych pol
						freePlacesAveliable--;						

						if (isOver('o', xClient, yClient) == true){
							res = send(clientSock, winOMessage, res, 0);
							if (res == SOCKET_ERROR) {
								cout << "send error: " << WSAGetLastError() << endl;
								closesocket(clientSock);
								WSACleanup();
								return 1;
							}	
							saveResults('o');
							cout << "Wygrana klienta - o\n" << endl;
							end = true;							
						}	
						else if (freePlacesAveliable == 0){
							saveResults('r');
							cout << "Remis" << endl;
							end = true;						
						}
					}
					else {
						//prosba o ponowne wyslanie wspolrzednych
						res = send(clientSock, wrongArgsMessage, res, 0);
						if (res == SOCKET_ERROR) {
							cout << "send error: " << WSAGetLastError() << endl;
							closesocket(clientSock);
							WSACleanup();
							return 1;
						}	
						res = recv(clientSock, recvbuf, recvbuflen, 0);
					}
				}
			}
			clientsXYaccepted = false;
			
			if (end == true){
				res = send(clientSock, endingMessage, res, 0);
				if (res == SOCKET_ERROR) {
					cout << "send error: " << WSAGetLastError() << endl;
					closesocket(clientSock);
					WSACleanup();
					return 1;
				}
            }
			else {
				serverXYparse (xServer, yServer, xServerStr, yServerStr, xyServerStr);	
		
				if (xServerStr == "x" || yServerStr == "x"){
					cout << "Zakonczenie polaczenia" << endl;
					res = send(clientSock, endingMessage, res, 0);
					if (res == SOCKET_ERROR) {
						cout << "send error: " << WSAGetLastError() << endl;
						closesocket(clientSock);
						WSACleanup();
						return 1;
					}										
					break;
				}
				board[xServer + (yServer - 1) * 3] = 'x';

				printBoard();					
				xyServerStr = xServerStr + yServerStr;
				
				if (isOver('x', xServer, yServer) == true){
					xyServerStr += '0';
					cout << "Wygrales" << endl;
					saveResults('x');
					end = true;
				}
								
				sendbuf = new char[xyServerStr.size() + 1];
				copy(xyServerStr.begin(), xyServerStr.end(), sendbuf);
				sendbuf[xyServerStr.size()] = '\0';
				
				//wyslanie zaznaczonych przez serwer wspolrzednych
				sendRes = send(clientSock, sendbuf, (int)strlen(sendbuf), 0);
				delete[] sendbuf;
				if (sendRes == SOCKET_ERROR) {
					cout << "send error: " << WSAGetLastError() << endl;
					closesocket(clientSock);
					WSACleanup();
					return 1;
				}	
				freePlacesAveliable--;
				if (freePlacesAveliable == 0){
					cout << "Remis" << endl;
					end = true;					
				}				
            }
        }
        else if (res == 0)
            cout << "Zakonczenie polaczenia" << endl;
        else  {
            cout << "recv error: " << WSAGetLastError() << endl;
            closesocket(clientSock);
            WSACleanup();
            return 1;
        }
    } while (res > 0);
	
	
	
	showHistory();

 
    res = shutdown(clientSock, SD_SEND);
    if (res == SOCKET_ERROR) {
        cout << "shutdown error: " <<  WSAGetLastError();
        closesocket(clientSock);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(clientSock);
    WSACleanup();

    return 0;
}
