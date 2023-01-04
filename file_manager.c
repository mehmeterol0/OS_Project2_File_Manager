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

void process_create_command(int fd_response, char* file_name) {
  // Olası diger clientlerinde erisememesi icin mutex kilitlenir
  pthread_mutex_lock(&lock);

  // dosyanın varlığı kontrol edilir.
  if (access(file_name, F_OK) == -1) {
    // dosya yoksa, dosya oluşturulur.
    FILE* fp = fopen(file_name,"w");
    fclose(fp);

    // response_pipe'dan file_clienta bilgi gönderilir.
    write(fd_response, "Dosya Basariyla Olusturuldu\n", 29);
  } else {
    // zaten oyle bir dosya varsa file_clienta asagidaki response dondurulur.
    write(fd_response, "Olusturulmak Istenen Dosya Zaten Mevcut\n", 41);
  }

  // mutex acılır
  pthread_mutex_unlock(&lock);
}

void process_write_command(int fd_response, char* file_name, char* data) {
  // Olası diger clientlerinde erisememesi icin mutex kilitlenir
  pthread_mutex_lock(&lock);

  // dosyanın varlığı kontrol edilir.
  if (access(file_name, F_OK) != -1) {
    // dosya varsa, dosyaya yazma islemi gerceklestirilir.
    FILE* fp = fopen(file_name,"a");
    fprintf(fp, "%s\n",data);
    fclose(fp);

    // response_pipe'dan file_clienta bilgi gönderilir.
    write(fd_response, "Yazma Islemi Basarili. Dosyaya Veri Yazildi.\n", 46);
  } else {
    // yazacagi dosyayi bulamazsa, file_clienta asagidaki responsu dondurur.
    write(fd_response, "Yazma Islemi Icın Kullanilan Dosya Bulunumadi\n", 47);
  }

  // mutex acilir
  pthread_mutex_unlock(&lock);
}

void process_delete_command(int fd_response, char* file_name) {
  // Olası diger clientlerinde erisememesi icin mutex kilitlenir
  pthread_mutex_lock(&lock);

  // dosyanın varlığı kontrol edilir.
  if (access(file_name, F_OK) != -1) {
    // dosya varsa, dosya silinir.
    remove(file_name);

    // response_pipe'dan file_clienta bilgi gönderilir.
    write(fd_response, "Dosya Basariyla Silindi\n", 25);
  } else {
    // dosya bulunamazsa, file_clienta asagidaki responsu dondurur.
    write(fd_response, "Silinmek Istenen Dosya Bulunamadi\n", 35);
  }

  // mutex acilir
  pthread_mutex_unlock(&lock);
}

void process_read_command(int fd_response, char* file_name) {
  // Olası diger clientlerinde erisememesi icin mutex kilitlenir
  pthread_mutex_lock(&lock);

  // dosyanın varlığı kontrol edilir.
  if (access(file_name, F_OK) != -1) {
    // dosya varsa, dosya icerigi okunur.
    FILE* fp = fopen(file_name, "r");
    char buffer[MAX_BUF_SIZE];
    char response[MAX_BUF_SIZE];
    strcpy(response, "");

    while (fgets(buffer, MAX_BUF_SIZE, fp) != NULL) {
      strcat(response, buffer);
    }

    fclose(fp);

    // response_pipe'dan file_clienta bilgi gönderilir.
    write(fd_response, response, strlen(response));
  } else {
    // dosya bulunamazsa, file_clienta asagidaki responsu dondurur.
    write(fd_response, "Okunmak Istenen Dosya Bulunamadi\n", 35);
  }

  // mutex acilir
  pthread_mutex_unlock(&lock);
}

void process_exit_command(int fd_response) {
  // response_pipe'dan file_clienta bilgi gönderilir.
  write(fd_response, "Programdan Cikis Yapiliyor\n", 27);

  // programdan cikis yapilir
  exit(0);
}

void* listen_thread(void* arg) {
  int fd;
  int fd_response;
  char buf[MAX_BUF_SIZE];
  char file_name[MAX_BUF_SIZE];
  char data[MAX_BUF_SIZE];
  int bytes_read;
  // named_pipe adında bir pipe olusturulur.
  fd = open(NAMED_PIPE, O_RDONLY);
  // gönderilecek olan responslar icin response_named_pipe adında bir pipe olusturulur.
  fd_response = open(RESPONSE_NAMED_PIPE, O_WRONLY);
  // pipe'dan sürekli okuma icin sonsuz dongu olusturuldu.
  while (1) {
    // pipe'dan okunan veri buf array icersinde tutulur.
    bytes_read = read(fd, buf, MAX_BUF_SIZE);
    buf[bytes_read] = '\0';

    // komutlara gore islemler gerceklestirilir
    if (strncmp(buf, "create", 6) == 0) {
      // komuttan dosya adi cikar
      sscanf(buf, "create %s", file_name);

      process_create_command(fd_response, file_name);
    } else if (strncmp(buf, "write", 5) == 0) {
      // komuttan dosya adi ve veri cikar
      sscanf(buf, "write %s %[^\n]", file_name, data);

      process_write_command(fd_response, file_name, data);
    } else if (strncmp(buf, "delete", 6) == 0) {
      // komuttan dosya adi cikar
      sscanf(buf, "delete %s", file_name);

      process_delete_command(fd_response, file_name);
    } else if (strncmp(buf, "read", 4) == 0) {
      // komuttan dosya adi cikar
      sscanf(buf, "read %s", file_name);

      process_read_command(fd_response, file_name);
    } else if (strncmp(buf, "exit", 4) == 0) {
      process_exit_command(fd_response);
    }
  }

  return NULL;
}


int main(int argc, char** argv) {
  // mutex olusturulur
  pthread_mutex_init(&lock, NULL);

  // named_pipe olusturulur
  mkfifo(NAMED_PIPE, 0666);

  // response_named_pipe olusturulur
  mkfifo(RESPONSE_NAMED_PIPE, 0666);

  // 5 tane listen_thread olusturulur ve calistirilir
  pthread_t threads[5];
  for (int i = 0; i < 5; i++) {
    pthread_create(&threads[i], NULL, listen_thread, NULL);
  }

  // tüm threadlerin bitmesini bekle
  for (int i = 0; i < 5; i++) {
    pthread_join(threads[i], NULL);
  }

  return 0;
}
