#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_BUF_SIZE 1024
#define NAMED_PIPE "named_pipe"
#define RESPONSE_NAMED_PIPE "response_named_pipe"

pthread_mutex_t lock;

int main() {
  int fd;
  int fd_response;
  char buf[MAX_BUF_SIZE];
  int bytes_read;
  
  // named_pipe'a yazmak icin acilir.
  fd = open(NAMED_PIPE, O_WRONLY);
  // alınan responslar acilir.
  fd_response = open(RESPONSE_NAMED_PIPE, O_RDONLY);
  while (1) {

    // Kullanıcının kullanabilecegi komutlar
    printf("Komut Giriniz (create, write, read, delete, exit): ");

    // Kullanıcın girdigi komutu oku
    fgets(buf, MAX_BUF_SIZE, stdin);

    // komutu named_pipe'a yazdır.
    write(fd, buf, strlen(buf));

    // response_pipe'dan alınan responsları oku
    bytes_read = read(fd_response, buf, MAX_BUF_SIZE);
    buf[bytes_read] = '\0';

    // okunan responsları kullanıcıya goster
    printf("%s", buf);
    
  }

  // pipe'ı kapat.
  close(fd);

  return 0;
}

