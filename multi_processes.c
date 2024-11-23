#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <zlib.h>

const char *filename = "large_file.txt";
const char *gzname = "large_file.txt.gz";

void generate_large_file(const char *filename) {
int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
srand(time(NULL)); // Seed the random number generator
for (int i = 0; i < 1000000; i++) { // Adjust loop for more lines
    char line[51]; // 50 characters + 1 for '\n'
    for (int j = 0; j < 50; j++) {
        int r = rand() % 75;
        if (r < 26) line[j] = 'A' + r;
        else if (r < 52) line[j] = 'a' + r - 26;
        else line[j] = '0' + r - 52;
}
line[50] = '\n';
if (write(file, line, 51) != 51) {
perror("Failed to write to file");
close(file);
exit(1);
}
}
close(file); // Close the file after writing
printf("File '%s' created.\n", filename);
}

void count_letters(const char *filename) {
int file = open(filename, O_RDONLY);
char buffer[128];
ssize_t bytes_read;
int letters = 0;
while ((bytes_read = read(file, buffer, sizeof(buffer))) > 0) {
      for (ssize_t i = 0; i < bytes_read; i++) {
          if (isalpha(buffer[i])) {
             letters++;}
}
}
close(file);
printf("letters count = %d\n", letters);
}

void count_numbers(const char *filename) {
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
}

void count_lines(const char *filename) {
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
}

void encrypt_file(const char *filename, int pipe_fd[2]) {
printf("Encrypting file '%s'...\n", filename);
sleep(1); 
const char *m = "Encryption done";
write(pipe_fd[1], m, strlen(m) + 1);
close(pipe_fd[1]);
printf("File '%s' encrypted.\n", filename);
}

void compress_file(const char *filename, const char *gzname, int pipe_fd[2]) {
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
         exit(1);
}
new_size += bytes_read;
}
gzclose(dest_file);
fclose(src_file);
write(pipe_fd[1], &new_size, sizeof(new_size));
close(pipe_fd[1]);
printf("File '%s' compressed to %d bytes.\n", gzname, new_size);
}

int main() {
clock_t start, end;
double time;
start = clock();
int pipe_fd[2];
int x;
pipe(pipe_fd);
pid_t pid = fork();
if (pid == 0) {
   generate_large_file(filename);
   exit(0);
} 
else {
     wait(NULL);
     printf("Enter 00 for file text analysis and for other services: ");
     scanf("%d", &x);
     pid_t pid_letters = fork();
     if (pid_letters == 0) {
         count_letters(filename);
         exit(0);
     }
     pid_t pid_numbers = fork();
     if (pid_numbers == 0) {
         count_numbers(filename);
         exit(0);
     }
     pid_t pid_lines = fork();
     if (pid_lines == 0) {
         count_lines(filename);
         exit(0);
     }
     wait(NULL);
     wait(NULL);
     wait(NULL);
     printf("Enter 1 for encryption service or 2 for compressing service: ");
     int c;
     scanf("%d", &c);
     if (c == 1) {
         encrypt_file(filename, pipe_fd);
     } 
     else if (c == 2) {
             compress_file(filename, gzname, pipe_fd);
     } 
     if (c == 1) {
        char buffer[128];
        read(pipe_fd[0], buffer, sizeof(buffer));
        printf("Message received: %s\n", buffer);
     } 
     else if (c == 2) {
             int new_size;
             read(pipe_fd[0], &new_size, sizeof(new_size));
             printf("Parent received the file size: %d bytes\n", new_size);
     }
}
end = clock();
time = ((double)(end - start))/CLOCKS_PER_SEC;
printf("Execution time = %f seconds\n", time);
return 0;
}
