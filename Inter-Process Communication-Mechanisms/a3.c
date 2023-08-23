#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define REQ_PIPE_NAME "REQ_PIPE_55429"
#define RESP_PIPE_NAME "RESP_PIPE_55429"
#define MAX_STRING_LENGTH 250
#define SHM_REGION_SIZE 2530607

void processRequest(const char* request, int respPipe);

void writeStringField(int pipe, const char* field) {
    size_t len = strlen(field);
    if (len > 250)
        len = 250;

    write(pipe, field, len);
    write(pipe, "!", 1);
}

void writeNumberField(int pipe, unsigned int number) {
    write(pipe, &number, sizeof(unsigned int));
}


int main() {
    int reqPipe, respPipe;
    char request[MAX_STRING_LENGTH];

    if (mkfifo(RESP_PIPE_NAME, 0666) == -1) {
        perror("ERROR: Cannot create the response pipe");
        exit(1);
    }

    reqPipe = open(REQ_PIPE_NAME, O_RDONLY);
    if (reqPipe == -1) {
        perror("ERROR: Cannot open the request pipe");
        exit(1);
    }

    respPipe = open(RESP_PIPE_NAME, O_WRONLY);
    if (respPipe == -1) {
        perror("ERROR: Cannot open the response pipe");
        exit(1);
    }

    writeStringField(respPipe, "BEGIN");

    printf("SUCCESS\n");

    while (1) {
        ssize_t bytesRead = read(reqPipe, request, sizeof(request) - 1);
        if (bytesRead <= 0) {
            perror("ERROR: Failed to read request");
            exit(1);
        }
        request[bytesRead] = '\0';

        processRequest(request, respPipe);
        if (strcmp(request, "EXIT!") == 0)
        {
            break;
        }
    }

    close(reqPipe);
    close(respPipe);
    unlink(RESP_PIPE_NAME);

    return 0;
}
char* shm_ptr = NULL;
void* file_ptr = NULL;
int file_fd = -1;

void processRequest(const char* request, int respPipe) {
    int shm_fd = -1;

    if (strcmp(request, "ECHO!") == 0) {
        writeStringField(respPipe, "ECHO");
        writeStringField(respPipe, "VARIANT");
        writeNumberField(respPipe, 55429);
    }
    else if (strncmp(request, "CREATE_SHM", 10) == 0) {
        unsigned int size;
        sscanf(request, "CREATE_SHM %u", &size);

        shm_fd = shm_open("/s4NMMR", O_CREAT | O_RDWR, 0664);
        if (shm_fd == -1) {
            perror("Failed to create shared memory");
            writeStringField(respPipe, "CREATE_SHM");
            writeStringField(respPipe, "ERROR");
            return;
        }

        if (ftruncate(shm_fd, size) == -1) {
            perror("Failed to adjust shared memory size");
            writeStringField(respPipe, "CREATE_SHM");
            writeStringField(respPipe, "ERROR");
            close(shm_fd);
            return;
        }

        shm_ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_ptr == MAP_FAILED) {
            perror("Failed to map shared memory");
            writeStringField(respPipe, "CREATE_SHM");
            writeStringField(respPipe, "ERROR");
            close(shm_fd);
            return;
        }

        writeStringField(respPipe, "CREATE_SHM");
        writeStringField(respPipe, "SUCCESS");
    }
    else if (strncmp(request, "WRITE_TO_SHM", 12) == 0) {
    unsigned offset=0, value=0;
    sscanf(request, "WRITE_TO_SHM %u %u", &offset, &value);
 
    printf("Received offset: %u\n", offset);
    printf("Received value: %u\n", value);

    if (shm_ptr == NULL) {
        printf("Shared memory region does not exist\n");
        writeStringField(respPipe, "WRITE_TO_SHM");
        writeStringField(respPipe, "ERROR");
        return;
    }

    if (offset < 0 || offset > 2530607) {
        printf("Invalid offset: %u\n", offset);
        writeStringField(respPipe, "WRITE_TO_SHM");
        writeStringField(respPipe, "ERROR");
        return;
    }
    if(offset + sizeof(value) > 2530607)
    {
    	printf("Offset + value > 2530607");
        writeStringField(respPipe, "WRITE_TO_SHM");
        writeStringField(respPipe, "ERROR");
        return;
    }

    *(unsigned int*)(shm_ptr + offset) = value;

    printf("Wrote value %u at offset %u\n", value, offset);
    writeStringField(respPipe, "WRITE_TO_SHM");
    writeStringField(respPipe, "SUCCESS");
}


    else if (strncmp(request, "MAP_FILE", 8) == 0) {
        size_t file_name_length = strlen(request) - 10;
        char* file_name = malloc(file_name_length + 1);
        if (file_name == NULL) {
            perror("Failed to allocate memory");
            writeStringField(respPipe, "MAP_FILE");
            writeStringField(respPipe, "ERROR");
            return;
        }
        strncpy(file_name, request + 9, file_name_length);
        file_name[file_name_length] = '\0';

        file_fd = open(file_name, O_RDONLY);
        if (file_fd == -1) {
            perror("Failed to open file");
            writeStringField(respPipe, "MAP_FILE");
            writeStringField(respPipe, "ERROR");
            free(file_name); 
            return;
        }

        struct stat file_stat;
        if (fstat(file_fd, &file_stat) == -1) {
            perror("Failed to get file size");
            close(file_fd);
            writeStringField(respPipe, "MAP_FILE");
            writeStringField(respPipe, "ERROR");
            free(file_name); 
            return;
        }

        file_ptr = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
        if (file_ptr == MAP_FAILED) {
            perror("Failed to map file into memory");
            close(file_fd);
            writeStringField(respPipe, "MAP_FILE");
            writeStringField(respPipe, "ERROR");
            free(file_name); 
            return;
        }

        free(file_name); 
        writeStringField(respPipe, "MAP_FILE");
        writeStringField(respPipe, "SUCCESS");
    }
    else if (strncmp(request, "READ_FROM_FILE_OFFSET", 21) == 0) {
    unsigned int offset, num_bytes;
    sscanf(request, "READ_FROM_FILE_OFFSET %u %u", &offset, &num_bytes);

    if (shm_ptr == NULL) {
        printf("Shared memory region does not exist\n");
        writeStringField(respPipe, "READ_FROM_FILE_OFFSET");
        writeStringField(respPipe, "ERROR");
        return;
    }

    if (file_ptr == NULL) {
        printf("No file mapped in memory\n");
        writeStringField(respPipe, "READ_FROM_FILE_OFFSET");
        writeStringField(respPipe, "ERROR");
        return;
    }

    struct stat file_stat;
    if (fstat(file_fd, &file_stat) == -1) {
        perror("Failed to get file size");
        writeStringField(respPipe, "READ_FROM_FILE_OFFSET");
        writeStringField(respPipe, "ERROR");
        return;
    }

    off_t file_size = file_stat.st_size;
    off_t read_end_offset = offset + num_bytes;

    if (read_end_offset > file_size) {
        printf("Read exceeds file size\n");
        writeStringField(respPipe, "READ_FROM_FILE_OFFSET");
        writeStringField(respPipe, "ERROR");
        return;
    }

    memcpy(shm_ptr, file_ptr + offset, num_bytes);
    writeStringField(respPipe, "READ_FROM_FILE_OFFSET");
    writeStringField(respPipe, "SUCCESS");
}





}

