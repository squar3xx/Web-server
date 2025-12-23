#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 

#define PORT 8080 
#define BACKLOG 10 

#define WEB_DIR "www"

void send_file(int client_fd, const char* filepath) {
  
  FILE* file = fopen(filepath, "rb");
  if (!file) {
    char* error = 
      "HTTP/1.1 404 Not Found\r\n"
      "Content-Type: text/html\r\n"
      "Content-Length: 20"
      "\r\n"
      "404 - File Not Found";
    send(client_fd, error, strlen(error), 0);
    return;
  }

  fseek(file, 0, SEEK_END);
  long file_len = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  const char* mime_type = "text/html";
  const char* ext = strrchr(filepath, '.');

  if (ext != NULL) {
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
      mime_type = "text/html";
    } else if(strcmp(ext, ".css") == 0) {
      mime_type = "text/css";
    } else if(!strcmp(ext, ".js")) {
      mime_type = "application/javascript";
    } else if(strcmp(ext, ".png") == 0) {
      mime_type = "image/png";
    } else if(strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
      mime_type = "image/jpg";
    } else if(strcmp(ext, ".gif") == 0) {
      mime_type = "image/gif";
    } else if (strcmp(ext, ".ico") == 0) {
      mime_type = "image/x-icon";
    }
  }

  char headers[512];
  int header_length = snprintf(headers, sizeof(headers), 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %ld\r\n"
    "\r\n"
  , mime_type, file_len);
  send(client_fd, headers, header_length, 0);

  char buffer[4096];
  size_t bytes_read;
  while ((bytes_read = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
    write(client_fd, buffer, bytes_read);
  }

  fclose(file);
  printf("Файл отправлен");
}

int main () {
  int server_fd;
  int client_fd;

  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;

  socklen_t client_len = sizeof(client_addr);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd == -1) {
    perror("Ошибка создания сокета");
    exit(EXIT_FAILURE);
  }

  // Установка опций и параметров сокету сервера для переиспользования адреса (не ждёт освобождения порта после предедущего закрытия)
  int optval = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    perror("Ошибка установки параметров сокета");
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) { 
    perror("Ошибка привязки");
    exit(EXIT_FAILURE);
  }
  
  if (listen(server_fd, BACKLOG) < 0) {
    perror("Ошибка listen()");
    exit(EXIT_FAILURE);
  }

  printf("Сервер запущен на порту %d\n", PORT);

  while (1) {
    printf("\nОжидание нового соединения\n");
    
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
      perror("Ошибка accept()");
      continue;
    }
  
    printf("Подключился клиент: %s\n", inet_ntoa(client_addr.sin_addr)); 
    
    char buffer[1024] = {0};
    read(client_fd, buffer, sizeof(buffer) - 1);
    printf("Запрос:\n%s\n", buffer);

    char method[16], path[256], protocol[16];
    sscanf(buffer, "%s %s %s", method, path, protocol);
    
    char file_path[512];
    if (strcmp(path, "/") == 0) {
      snprintf(file_path, sizeof(file_path), "%s/index.html", WEB_DIR);
    } else {
      snprintf(file_path, sizeof(file_path), "%s%s", WEB_DIR, path);
    }

    if (strstr(file_path, "..") != NULL) {
      char* forbidden = 
        "HTTP/1.1 403 Forbidden\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 15"
        "403 - Forbidden";
      send(client_fd, forbidden, strlen(forbidden), 0);
      exit(EXIT_FAILURE);
    }

    send_file(client_fd, file_path);

    close(client_fd);
  }

  close(server_fd);
  return 0;
}
