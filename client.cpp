#include <stdio.h>
#include <winsock2.h>
#include <string>
#include <iostream>
#include <conio.h>

#pragma comment (lib,"Ws2_32.lib")
#pragma warning(disable:4996)

const int SizeOfMessage = 256;
char username[SizeOfMessage];
HANDLE Mutex;

// Получение данных от сервера
DWORD WINAPI Receive(LPVOID server)
{
	int retVal;
	char buf[SizeOfMessage];
	SOCKET server_socket = *(SOCKET*)server;
	while (true)
	{
		retVal = recv(server_socket, buf, sizeof(buf), 0);
		WaitForSingleObject(Mutex, INFINITE);
		if (retVal == SOCKET_ERROR)
		{
			printf("Unable to recv\n");
			system("pause");
			return SOCKET_ERROR;
		}
		if (strcmp(buf, "Disconnected") == 0) {
			ReleaseMutex(Mutex);
			break;
		}
		printf("%s", buf);
		ReleaseMutex(Mutex);
		memset(buf, 0, sizeof(buf));
	}
	return 1;
}

// Отправка данных на сервер
DWORD WINAPI Send(LPVOID server)
{
	int retVal;
	char buf[SizeOfMessage];
	SOCKET server_socket = *(SOCKET*)server;
	while (true)
	{
		// Ожидаем ввод ENTER
		while (_getch() != '\r');
		WaitForSingleObject(Mutex, INFINITE);
		fgets(buf, SizeOfMessage, stdin);
		ReleaseMutex(Mutex);
		if (buf[0] == '\n' || buf[0] == '\0')
			continue;
		retVal = send(server_socket, buf, sizeof(buf), 0);
		if (retVal == SOCKET_ERROR)
		{
			printf("Unable to send\n");
			system("pause");
			return SOCKET_ERROR;
		}
		if (strcmp(buf, "exit\n") == 0)
		{
			printf("Thank you for using the application");
			break;
		}
	}
	return 1;
}

int main()
{
	//setlocale(LC_ALL, "Russian");
	system("chcp 1251 > NUL");
	int retVal;

	// Создаем мьютекс
	Mutex = CreateMutex(NULL, FALSE, NULL);

	// Инициализация использования сокетов
	WSADATA ws;
	WSAStartup(MAKEWORD(2, 2), &ws);

	// Создание сокета клиента
	SOCKET client_socket;
	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_socket == INVALID_SOCKET) {
		printf("Unable to create socket\n");
		WSACleanup();
		system("pause");
		return SOCKET_ERROR;
	}

	// Считываем адрес и порт сервера
	char ip[50];
	int port;
	printf("Addres: ");
	scanf("%s", ip);
	printf("Port: ");
	scanf("%d", &port);

	// Формируем адрес сервера
	SOCKADDR_IN server_info;
	server_info.sin_family = AF_INET;
	server_info.sin_port = htons(port);
	server_info.sin_addr.S_un.S_addr = inet_addr(ip);

	// Соединяемся с сервером
	WaitForSingleObject(Mutex, INFINITE);
	retVal = connect(client_socket, (LPSOCKADDR)&server_info, sizeof(server_info));
	if (retVal == SOCKET_ERROR)
	{
		printf("Unable to connect\n");
		WSACleanup();
		system("pause");
		return 1;
	}
	char str[SizeOfMessage];
	retVal = recv(client_socket, str, sizeof(str), 0);
	if (retVal == SOCKET_ERROR)
	{
		printf("Unable to recv\n");
		system("pause");
		return SOCKET_ERROR;
	}
	if (!strcmp(str, "Server is full!"))
	{
		printf("%s", str);
		closesocket(client_socket);
		WSACleanup();
		return 0;
	}
	printf("Connection made sucessfully\n");
	printf("Enter your username: ");
	scanf("%s", username);
	while ((getchar()) != '\n');
	retVal = send(client_socket, username, sizeof(username), 0);
	if (retVal == SOCKET_ERROR)
	{
		printf("Unable to send\n");
		system("pause");
		return SOCKET_ERROR;
	}
	ReleaseMutex(Mutex);

	DWORD tid;
	HANDLE t1 = CreateThread(NULL, 0, Receive, &client_socket, 0, &tid);
	if (t1 == NULL) std::cout << "Thread creation error: " << GetLastError();
	HANDLE t2 = CreateThread(NULL, 0, Send, &client_socket, 0, &tid);
	if (t2 == NULL) std::cout << "Thread creation error: " << GetLastError();

	WaitForSingleObject(t1, INFINITE);
	WaitForSingleObject(t2, INFINITE);
	closesocket(client_socket);
	WSACleanup();

	return 0;
}