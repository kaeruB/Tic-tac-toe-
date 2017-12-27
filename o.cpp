//client = o

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <signal.h> 
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define MAX_BUF_LEN 10
#define DEFAULT_PORT "27015"
using namespace std;

bool end = false;
int res;
SOCKET ConnectSocket = INVALID_SOCKET;
int xClient = 0;
int yClient = 0; 
bool stop = false;

void endConnection (){
	int res;
	char receiveBuf[MAX_BUF_LEN];
	
    if (shutdown(ConnectSocket, SD_SEND) == SOCKET_ERROR) {
        cout << "shutdown error:  " << WSAGetLastError() << endl;
        closesocket(ConnectSocket);
        WSACleanup();
        exit(1);
    }
	
	do {
        res = recv(ConnectSocket, receiveBuf, MAX_BUF_LEN, 0);
		if (res == 0)
            cout << "Zatrzymanie polaczenia\n";
        else if (res < 0)
            cout << "recv error: " << WSAGetLastError() << endl;
    } while( res > 0 );

    closesocket(ConnectSocket);
    WSACleanup();	
	exit(0);
}

void sigHandler(int sig){
	if (sig == SIGINT){
		cout << "Otrzymano SIGINT, konczymy." << endl;
		endConnection();
	}
}

void printBoard(char board[]) {
	system("CLS");
	cout << endl;
	for(int i = 1; i < 10; i++) {
		cout << " " << board[i] << " ";
		if(i % 3)
		  cout << "|";
		else if(i != 9)
		  cout << "\n---+---+---\n";
		else cout << endl;
	}
	cout << endl;
}


void getClientXY(string &xClientStr, string &yClientStr, string &xyClientStr) {	
	cout << "Wprowadz wspolrzedne: " << endl;
	cout << "x: " << endl;
	cin >> xClientStr;
	if (xClientStr == "x"){
		endConnection();
		return;
	}
	cout << "y: " << endl;
	cin >> yClientStr;
	if (yClientStr == "x")
		endConnection();
	xyClientStr = xClientStr + yClientStr;	

	xClient = atoi(xClientStr.c_str());
	yClient = atoi(yClientStr.c_str());
}

void sendMove(){
	string xClientStr, yClientStr, xyClientStr; 		//wspolrzedne do wyslania
	//pobranie wspolrzednych
	if (stop != true) {
		getClientXY(xClientStr, yClientStr, xyClientStr);
		
		//przygotowanie do wyslania
		char *sendbuf = new char[xyClientStr.size() + 1];
		sendbuf = new char[xyClientStr.size() + 1];
		copy(xyClientStr.begin(), xyClientStr.end(), sendbuf);
		sendbuf[xyClientStr.size()] = '\0';
		
		//wysylanie wiadomosci
		res = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
		delete[] sendbuf;
		if (res == SOCKET_ERROR) {
			cout << "send failed with error: " <<  WSAGetLastError() << endl;
			closesocket(ConnectSocket);
			WSACleanup();
			exit(1);
		}	
	}
}
/*
void showHistory(){
	char answer;
	const char *getHistory = "t";
	const char *doNotGetHistory = "n";
	
	cout << "Pokazac historie?" << endl;
	cout << "Tak - t, nie - n" << endl;
	cin >> answer;
	if (answer == 't'){
		cout << "Pokazac" << endl;
		res = send(ConnectSocket, getHistory, (int)strlen(getHistory), 0 );
		if (res == SOCKET_ERROR) {
			cout << "send failed with error: " <<  WSAGetLastError() << endl;
			closesocket(ConnectSocket);
			WSACleanup();
			exit(1);
		}	
	}
	else {
		cout << "nie pokazac" << endl;
		res = send(ConnectSocket, doNotGetHistory, (int)strlen(doNotGetHistory), 0 );
		if (res == SOCKET_ERROR) {
			cout << "send failed with error: " <<  WSAGetLastError() << endl;
			closesocket(ConnectSocket);
			WSACleanup();
			exit(1);
		}	
		
	}		
}
*/

void receiveAnswer(char board[]) {
	char stateBuf[MAX_BUF_LEN];
	bool receiveAnswerAgain = true;
	int xServer, yServer, xyServer;						//wspolrzedne serwera	
	
	while (receiveAnswerAgain == true && stop != true){
		res = recv(ConnectSocket, stateBuf, MAX_BUF_LEN, 0);
		if (res > 0){
			string message(stateBuf);		//jak dostanie e to ma sie zakonczyc
			if (message == "e"){
				cout << "Otrzymano polecenie zakonczenia." << endl;
				receiveAnswerAgain = false;		//koniec, nie trzeba wiecej wiadomosci od serwera
				end = true;
			}
			else if (message == "w") {
				cout << "Wprowadzono niepoprawne wspolrzedne" << endl;
				sendMove();
				receiveAnswerAgain = true;		//bo serwer ma powiedziec czy wspolrzedne sa ok
			}
			else if (message == "k"){
				board[xClient + (yClient - 1) * 3] = 'o';
				printBoard(board);
				receiveAnswerAgain = true;	//bo od serwera chcemy ruch
			}
			else if (message == "o"){
				cout << "wygralas :|\n";
				receiveAnswerAgain = false;
				stop = true;
				//showHistory();
				endConnection();
			}
			
			//serwer wyslal wspolrzedne
			else{
				receiveAnswerAgain = false;		//odebralismy wspolrzedne do zaznaczenia, konczymy while'a
				
				xyServer = atoi(stateBuf);
				if (xyServer > 100){
					cout << "Wygrana serwera - x" << endl;
					receiveAnswerAgain = false;
					stop = true;
					//showHistory();
					endConnection();
				}
				xServer = xyServer / 10;
				yServer = xyServer % 10;
				board[xServer + (yServer - 1) * 3] = 'x';
				printBoard(board);
			}

		}
		else if (res == 0)
			cout << "Zatrzymanie polaczenia 1";
		else
			cout << "recv error 1: " << WSAGetLastError() << endl;

	}
}



int __cdecl main(int argc, char **argv)
{
	char board[10];		
	int xServer, yServer, xyServer;						//wspolrzedne serwera	
	WSADATA wsaData;
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;
	
	for (int i = 1; i < 10; i++)
		board[i] = ' ';
	printBoard(board);
	
	
	if (argc != 2) {
        cout << "Nalezy podac ip serwera z ktorym nawiazujemy polaczenie." << endl;
        return 1;
    }

	//inicjalizacja Winsock
    res = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (res != 0) {
        cout << "WSAStartup error: " << res << endl;
        return 1;
    }
	
	//ZeroMemory zapelnia wskazany obszar pamieci zerami
	ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    res = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (res != 0) {
        cout << "getaddrinfo error: " << res << endl;
        WSACleanup();
        return 1;
    }
	
	for(ptr = result; ptr != NULL ;ptr = ptr->ai_next) {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            cout << "socket failed with error: " << WSAGetLastError() << endl;
            WSACleanup();
            return 1;
        }

        res = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (res == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
        cout << "Nie udalo sie polaczyc z serwerem." << endl;
        WSACleanup();
        return 1;
    }
	signal(SIGINT, sigHandler);
	
	cout << "Aby zakonczyc nacisnij x" << endl;
	cout << "Mozna wpisywac wartosci 1, 2 lub 3 jako x i y." << endl;
	
	do {
		sendMove();
		receiveAnswer(board);		
	} while (!end);
	
	endConnection();	
	
	return 0;
}