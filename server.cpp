#include <stdio.h>
#include <iostream>
#include <winsock2.h>
#include <string>

#pragma comment (lib,"Ws2_32.lib")
#pragma warning(disable:4996)

const int maxClients = 3;
int numClients = 0;
const int SizeOfMessage = 256;
char usernames[maxClients + 1][256];
SOCKET sockets[maxClients];
HANDLE Mutex;

DWORD WINAPI Chat(LPVOID client)
{
	setlocale(LC_ALL, "Russian");
	int cur;
	int retVal;
	char buf[SizeOfMessage];
	SOCKET client_socket = *(SOCKET*)client;
	while (true)
	{
		retVal = recv(client_socket, buf, sizeof(buf), 0);
		if (retVal == SOCKET_ERROR)
		{
			printf("Unable to recv\n");
			system("pause");
			return SOCKET_ERROR;
		}
		if (strcmp(buf, "exit\n") == 0) //Если клиент отсоединился
		{
			WaitForSingleObject(Mutex, INFINITE);
			for (int i = 0; i < numClients; i++)
				if (sockets[i] == client_socket)
				{
					printf("%s Disconnected\nNumber of users: %d\n", usernames[i], numClients - 1);
					strcat(usernames[i], " Disconnected\n");
					retVal = send(sockets[i], "Disconnected", sizeof("Disconnected"), 0);
					if (retVal == SOCKET_ERROR)
					{
						printf("Unable to send\n");
						system("pause");
						return SOCKET_ERROR;
					}
					cur = i;
				}
			for (int i = 0; i < numClients; i++)
				if (sockets[i] != client_socket)
				{
					retVal = send(sockets[i], usernames[cur], sizeof(usernames[cur]), 0);
					if (retVal == SOCKET_ERROR)
					{
						printf("Unable to send\n");
						system("pause");
						return SOCKET_ERROR;
					}
				}
			for (int j = cur; j < numClients; j++)
			{
				sockets[j] = sockets[j + 1];
				strcpy(usernames[j], usernames[j + 1]);
			}
			numClients--;
			char str[SizeOfMessage];
			sprintf(str, "Neumber of users: %d\n", numClients);
			for (int i = 0; i < numClients; i++)
			{
				retVal = send(sockets[i], str, sizeof(str), 0);
				if (retVal == SOCKET_ERROR)
				{
					printf("Unable to send\n");
					system("pause");
					return SOCKET_ERROR;
				}
			}
			ReleaseMutex(Mutex);
			break;
		}
		// Если все нормально, то отсылаем всем сообщения
		WaitForSingleObject(Mutex, INFINITE);
		char message[SizeOfMessage];
		for (int i = 0; i < numClients; i++)
		{
			if (sockets[i] == client_socket)
			{
				strcpy(message, usernames[i]);
				strcat(message, ": ");
				strcat(message, buf);
			}
		}
		printf("%s", message); // Выводим на сервер
		// Отправляем клиентам
		for (int i = 0; i < numClients; i++)
		{
			if (sockets[i] != client_socket)
			{
				retVal = send(sockets[i], message, sizeof(message), 0);
				if (retVal == SOCKET_ERROR)
				{
					printf("Unable to send\n");
					system("pause");
					return SOCKET_ERROR;
				}
			}
		}
		ReleaseMutex(Mutex);
		memset(buf, 0, sizeof(buf)); //Очистить буфер
	}
	return 1;
}

int main()
{
	//setlocale(LC_ALL, "Russian");
	system("chcp 1251 > NUL");
	int retVal;

	// Создали мьютекс
	Mutex = CreateMutex(NULL, FALSE, NULL);

	// Инициализация использования сокетов
	WSADATA ws;
	WSAStartup(MAKEWORD(2, 2), &ws);

	// Создание сокета сервера
	SOCKET server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Дескриптор на серверный сокет
	if (server_socket == INVALID_SOCKET)
	{
		printf("Unable to create socket\n");
		WSACleanup();
		system("pause");
		return SOCKET_ERROR;
	}

	// Формируем адрес сервера
	SOCKADDR_IN server_info;
	server_info.sin_addr.S_un.S_addr = INADDR_ANY;
	server_info.sin_family = AF_INET;
	server_info.sin_port = htons(2006);

	// Связываем сокет сервера с его адресом
	retVal = bind(server_socket, (LPSOCKADDR)&server_info, sizeof(server_info)); // Связывает сокет с адресом
	if (retVal == SOCKET_ERROR)
	{
		printf("Unable to bind\n");
		WSACleanup();
		system("pause");
		return SOCKET_ERROR;
	}
	printf("Server started at %s, port %d\n", inet_ntoa(server_info.sin_addr), htons(server_info.sin_port));

	while (true)
	{
		// Слушаем сокет сервера
		retVal = listen(server_socket, 10);
		if (retVal == SOCKET_ERROR)
		{
			printf("Unable to listen\n");
			WSACleanup();
			system("pause");
			return SOCKET_ERROR;
		}

		// Принимаем соединение нового клиента
		SOCKET client_socket;
		SOCKADDR_IN client_info;
		int client_addr_size = sizeof(client_info);
		client_socket = accept(server_socket, (LPSOCKADDR)&client_info, &client_addr_size);
		if (client_socket == INVALID_SOCKET)
		{
			printf("Unable to accept\n");
			WSACleanup();
			system("pause");
			return SOCKET_ERROR;
		}
		if (numClients == maxClients)
		{
			retVal = send(client_socket, "Server is full!", sizeof("Server is full!"), 0);
			if (retVal == SOCKET_ERROR)
			{
				printf("Unable to send\n");
				system("pause");
				return SOCKET_ERROR;
			}
			continue;
		}
		else
		{
			retVal = send(client_socket, "Good", sizeof("Good"), 0);
			if (retVal == SOCKET_ERROR)
			{
				printf("Unable to send\n");
				system("pause");
				return SOCKET_ERROR;
			}
		}
		printf("New connection accepted from %s, port %d. ", inet_ntoa(client_info.sin_addr), htons(client_info.sin_port));

		// Заблокировали поток, чтобы добавить клиента
		WaitForSingleObject(Mutex, INFINITE);
		retVal = recv(client_socket, usernames[numClients], sizeof(usernames[numClients]), 0);
		if (retVal == SOCKET_ERROR)
		{
			printf("Unable to recv\n");
			system("pause");
			return SOCKET_ERROR;
		}
		sockets[numClients] = client_socket;
		printf("His username: %s\n", usernames[numClients]);
		char new_user[SizeOfMessage] = "New user in chat: ";
		strcat(new_user, usernames[numClients]);
		strcat(new_user, "\n");
		for (int i = 0; i < numClients; i++)
		{
			if (sockets[i] != client_socket)
			{
				retVal = send(sockets[i], new_user, sizeof(new_user), 0);
				if (retVal == SOCKET_ERROR)
				{
					printf("Unable to send\n");
					system("pause");
					return SOCKET_ERROR;
				}
			}
		}
		numClients++;
		char str[SizeOfMessage];
		sprintf(str, "Number of users: %d\n", numClients);
		for (int i = 0; i < numClients; i++)
		{
			retVal = send(sockets[i], str, sizeof(str), 0);
			if (retVal == SOCKET_ERROR)
			{
				printf("Unable to send\n");
				system("pause");
				return SOCKET_ERROR;
			}
		}
		printf("Number of users: %d\n", numClients);
		// Разблокировали поток
		ReleaseMutex(Mutex);

		DWORD tid; //Идентификатор
		CreateThread(NULL, 0, Chat, &client_socket, 0, &tid); //Создание потока для получения данных
	}
}
