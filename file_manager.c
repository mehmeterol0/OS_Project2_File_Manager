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

void* listen_thread(void* arg) {
  int fd;
  int fd_response;
  char buf[MAX_BUF_SIZE];
  char file_name[MAX_BUF_SIZE];
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

    // gelen komut create ise;
    if (strncmp(buf, "create", 6) == 0) {
      // komuttan dosya adi cikar
      sscanf(buf, "create %s", file_name);

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
    } else if (strncmp(buf, "write", 5) == 0) {
      // komuttan dosya adi cikar
      char data[MAX_BUF_SIZE];
      sscanf(buf, "write %s %[^\n]", file_name, data);

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
    } else if (strncmp(buf, "read", 4) == 0) {
      // komuttan dosya adi cikar
      int line_num;
      sscanf(buf, "read %s %d", file_name, &line_num);

      // Olası diger clientlerinde erisememesi icin mutex kilitlenir
      pthread_mutex_lock(&lock);

      // dosyanın varlığı kontrol edilir.
      if (access(file_name, F_OK) != -1) {
        // dosya varsa belirlenen satır okunur.
        FILE* fp = fopen(file_name, "r");
        char line[MAX_BUF_SIZE];
        int i = 0;
        while (fgets(line, MAX_BUF_SIZE, fp) != NULL) {
          i++;
          if (i == line_num) {
            // Belirtilen satırı bulundugunda, pipe'a yaz.
            write(fd_response, line, strlen(line));
            break;
          }
        }
        if (i < line_num) {
          // Belirtilen satır numarası aralık dışındaysa, pipe'a hata mesajı yaz
          write(fd_response, "Dosya Icersinde Ilgili Indis Kadar Satir Yok.\n", 47);
        }
        fclose(fp);
      } else {
        // Dosya mevcut değil, belirtilen pipe'a mesajı yaz.
        write(fd_response, "Okunacak Dosya Mevcut Degil\n", 29);
      }

      // mutex acilir.
      pthread_mutex_unlock(&lock);
    } else if (strncmp(buf, "delete", 6) == 0) {
      // dosyanın varlığı kontrol edilir.
      sscanf(buf, "delete %s", file_name);

      // mutex kilitlenir
      pthread_mutex_lock(&lock);

      // dosyanın varlığı kontrol edilir.
      if (access(file_name, F_OK) != -1) {
        // dosya varsa silinir
        remove(file_name);

        // file_clienta response dondurulur.
        write(fd_response, "Dosya Basariyla Silindi\n", 25);
      } else {
        // dosya yoksa asagidaki response file_clienta dondurulur.
        write(fd_response, "Zaten Boyle Bir Dosya Mevcut Degil\n", 36);
      }

      // mutex acilir
      pthread_mutex_unlock(&lock);
    } else if (strncmp(buf, "exit", 4) == 0) {
      // file_clienta asagidaki response dondurulur.
      write(fd_response, "File_Manager Kapandi\n", 22);

      // pipe kapat ve threddan çık.
      close(fd_response);
      pthread_exit(NULL);
    } else {
      // geçersiz komut geldiği takdirde asagidaki response dondurulur.
      write(fd_response, "Gecersiz Komut\n", 16);
    }
  }
}

int main() {
  pthread_t listen_thread_id;

  // pipe olustur.
  mkfifo(NAMED_PIPE, 0664);
  mkfifo(RESPONSE_NAMED_PIPE, 0664);
  // mutexi baslat
  pthread_mutex_init(&lock,NULL);

  // thread olustur
  pthread_create(&listen_thread_id, NULL, listen_thread, NULL);

  // threadin bitmesi beklenir
  pthread_join(listen_thread_id, NULL);

  // mutex yok edilir.
  pthread_mutex_destroy(&lock);

  return 0;
}
