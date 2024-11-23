#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <zlib.h>
#include <pthread.h>

const char *filename = "large_file.txt";
const char *gzname = "large_file.txt.gz";

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *generate_large_file(void *arg) {
int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
srand(time(NULL));
for (int i = 0; i < 100000; i++) { 
    char line[51]; 
    for (int j = 0; j < 50; j++) {
        int r = rand() % 75;
        if (r < 26) line[j] = 'A' + r;
        else if (r < 52) line[j] = 'a' + r - 26;
        else line[j] = '0' + r - 52;
}
line[50] = '\n';
if ( write(file, line, 51) != 51) {
    perror("Failed to write to file");
    close(file);
    pthread_exit(NULL);
}
}
close(file); 
printf("File '%s' created.\n", filename);
pthread_exit(NULL);
}

void *count_letters(void *arg) {
int file = open(filename, O_RDONLY);
char buffer[128];
ssize_t bytes_read;
int letters = 0;
while ((bytes_read = read(file, buffer, sizeof(buffer))) > 0) {
       for (ssize_t i = 0; i < bytes_read; i++) {
           if (isalpha(buffer[i])) {
              letters++;
}
}
}
close(file);
printf("letters count = %d\n", letters);
pthread_exit(NULL);
}

void *count_numbers(void *arg) {
int file = open(filename, O_RDONLY);
char buffer[128];
ssize_t bytes_read;
int numbers = 0;
while ((bytes_read = read(file, buffer, sizeof(buffer))) > 0) {
       for (ssize_t i = 0; i < bytes_read; i++) {
           if (isdigit(buffer[i])) {
              numbers++;
}
}
}
close(file);
printf("numbers count = %d\n", numbers);
pthread_exit(NULL);
}

void *count_lines(void *arg) {
int file = open(filename, O_RDONLY);
char buffer[128];
ssize_t bytes_read;
int lines = 0;
while ((bytes_read = read(file, buffer, sizeof(buffer))) > 0) {
      for (ssize_t i = 0; i < bytes_read; i++) {
          if (buffer[i] == '\n') {
             lines++;
}
}
}
close(file);
printf("lines count = %d\n", lines);
pthread_exit(NULL);
}

void *encrypt_file(void *arg) {
int *pipe_fd = (int *)arg;
pthread_mutex_lock(&mutex);
printf("Encrypting file '%s'...\n", filename);
sleep(1);
const char *m = "Encryption done";
write(pipe_fd[1], m, strlen(m) + 1);
close(pipe_fd[1]);
pthread_mutex_unlock(&mutex);
printf("File '%s' encrypted.\n", filename);
pthread_exit(NULL);
}

void *compress_file(void *arg) {
int *pipe_fd = (int *)arg;
pthread_mutex_lock(&mutex);
printf("Compressing file '%s'...\n", filename);
sleep(1);
FILE *src_file = fopen(filename, "rb");
gzFile dest_file = gzopen(gzname, "wb");
char buffer[128];
size_t bytes_read;
int new_size = 0;
while ((bytes_read = fread(buffer, 1, 128, src_file)) > 0) {
      if (gzwrite(dest_file, buffer, bytes_read) != bytes_read) {
          perror("Failed to write to compressed file");
          gzclose(dest_file);
          fclose(src_file);
          pthread_exit(NULL);
}
new_size += bytes_read;
}
gzclose(dest_file);
fclose(src_file);
write(pipe_fd[1], &new_size, sizeof(new_size));
close(pipe_fd[1]);
pthread_mutex_unlock(&mutex);
printf("File '%s' compressed to %d bytes.\n", gzname, new_size);
pthread_exit(NULL);
}

int main() {
clock_t start, end;
double time;
start = clock();
int pipe_fd[2];
pthread_t thread_ids[5];
int c;
int x;
pipe(pipe_fd);
pthread_create(&thread_ids[0], NULL, generate_large_file, NULL);
pthread_join(thread_ids[0], NULL);
printf("Enter 00 for file text analysis and for other services: ");
scanf("%d", &c);
pthread_create(&thread_ids[1], NULL, count_letters, NULL);
pthread_create(&thread_ids[2], NULL, count_numbers, NULL);
pthread_create(&thread_ids[3], NULL, count_lines, NULL);
pthread_join(thread_ids[1], NULL);
pthread_join(thread_ids[2], NULL);
pthread_join(thread_ids[3], NULL);
printf("Enter 1 for encryption service or 2 for compressing service: ");
scanf("%d", &c);
if (c == 1) {
    pthread_create(&thread_ids[4], NULL, encrypt_file, (void *)pipe_fd);}
else if (c == 2) {
        pthread_create(&thread_ids[4], NULL, compress_file, (void *)pipe_fd);}
pthread_join(thread_ids[4], NULL);
if (c == 1) {
   char buffer[128];
   read(pipe_fd[0], buffer, sizeof(buffer));
   printf("Message received: %s\n", buffer);} 
else if (c == 2) {
        int new_size;
        read(pipe_fd[0], &new_size, sizeof(new_size));
        printf("Parent received the file size: %d bytes\n", new_size);
}
close(pipe_fd[0]);
end = clock();
time = ((double)(end - start))/CLOCKS_PER_SEC;
printf("Execution time = %f seconds\n", time);
return 0;
}
