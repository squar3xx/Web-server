#include <stdio.h>
#include <string.h>
#include <stdlib.h> //exit
#include <unistd.h> // Для close()
#include <sys/types.h> // Типы данных для сокетов
#include <sys/socket.h> // Сами сокеты
#include <netinet/in.h> // Для sockadr_in
#include <arpa/inet.h> //htons

#define PORT 8080 // Будем слушать этот порт
#define BACKLOG 10 // Очередь входящих соединений (Чтобы не дропать входящие соединения)

int main () {
  int server_fd;
  int client_fd;

  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;

  socklen_t client_len = sizeof(client_addr);

  // Создание сокета: IPv4, TCP
  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd == -1) {
    perror("Ошибка создания сокета");
    exit(EXIT_FAILURE); // Выход из программы с кодом статуса
  }

  // Задаём параметры айпи и порта структуре
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) { // Привязка сокета к значениям (айпи адресу, порту), который мы указали в структуре
    perror("Ошибка привязки");
    exit(EXIT_FAILURE);
  }
  
  // Переключаем в режим прослушивания
  if (listen(server_fd, BACKLOG) < 0) {
    perror("Ошибка listen()");
    exit(EXIT_FAILURE);
  }

  printf("Сервер запущен на порту %d\n", PORT);

  while (1) {
    printf("\nОжидание нового соединения\n");
    
    // Принимаем входящее соединение
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
      perror("Ошибка accept()");
      continue;
    }
  
    printf("Подключился клиент: %s\n", inet_ntoa(client_addr.sin_addr)); // Для вывода айпи адреса клиента
    
    // Читаем HTTP запрос
    char buffer[1024] = {0};
    read(client_fd, buffer, sizeof(buffer) - 1);
    printf("Запрос:\n%s\n", buffer);

    char responce[] = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: text/html\r\n"
                      "Content-Length: 8\r\n"
                      "\r\n"
                      "Fuck you";
    write(client_fd, responce, strlen(responce));
    close(client_fd);
  }

  close(server_fd);
  return 0;
}
