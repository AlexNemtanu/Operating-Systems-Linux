#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

int list(char* path) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("ERROR\nCannot open directory\n");
        return -1;
    }

    struct dirent* entry;
    printf("SUCCESS\n");
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char* file_path = (char*) malloc(strlen(path) + strlen(entry->d_name) + 2);
            sprintf(file_path, "%s/%s", path, entry->d_name);
            printf("%s\n", file_path);
            free(file_path);
        }
    }

    closedir(dir);
    return 0;
}

void listRecursive(char* path, int firstLevel) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("ERROR\nInvalid directory path\n");
        return;
    }

    struct dirent* entry;
    struct stat statbuf;
    while ((entry = readdir(dir)) != NULL) {
        char fullPath[512];
        snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
        if (stat(fullPath, &statbuf) == -1) {
            printf("ERROR\nFailed to get file information\n");
            return;
        }
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if (firstLevel == 0) {
                printf("SUCCESS\n");
                firstLevel = 1;
            }
            printf("%s\n", fullPath);
            if (S_ISDIR(statbuf.st_mode)) {
                listRecursive(fullPath, firstLevel);
            }
        }
    }
    closedir(dir);
}

int startsWith(char* name, char* start) {
    int i, j;
    for (i = 0, j = 0; i < strlen(start); i++, j++) {
        if (name[j] != start[i])
            return 0;
    }
    return 1;
}

int printStartsWith(char* path, char* start) {
    DIR* dir = opendir(path);
    struct dirent* entry;
    if (dir != NULL) {
        printf("SUCCESS\n");

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                if (startsWith(entry->d_name, start) == 1)
                    printf("%s/%s\n", path, entry->d_name);
        }
        closedir(dir);
        return 0;
    } else
        return -1;
}


void parse(char* path) {
    char magic[5];
    int headerSize, version, sectNr;
    char name[18];
    int sectType, offset, size;

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("ERROR\nCould not open input file\n");
        return;
    }

    read(fd, magic, 4);
    magic[4] = '\0';
    if (strcmp(magic, "NiRH") != 0) {
        printf("ERROR\nwrong magic\n");
        close(fd);
        return;
    }

    read(fd, &headerSize, 2);
    if (headerSize < 18 || headerSize > 32) {
        printf("ERROR\nWrong header size\n");
        close(fd);
        return;
    }

    read(fd, &version, 2);
    if (version < 54 || version > 153) {
        printf("ERROR\nWrong version\n");
        close(fd);
        return;
    }

    read(fd, &sectNr, 1);
    if (sectNr < 6 || sectNr > 16) {
        printf("ERROR\nWrong section number\n");
        close(fd);
        return;
    }

    printf("SUCCESS\nversion=%d\nnr_sections=%d\n", version, sectNr);

    for (int i = 0; i < sectNr; i++) {
        read(fd, name, 17);
        name[17] = '\0';
        read(fd, &sectType, 4);
        read(fd, &offset, 4);
        read(fd, &size, 4);

        if (sectType != 69 && sectType != 32 && sectType != 62) {
            printf("ERROR\nWrong section types\n");
            close(fd);
            return;
        }

        printf("section%d: %s %d %d\n", i + 1, name, sectType, size);
    }

    close(fd);
}

void findall(char* path) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("ERROR\nInvalid directory path\n");
        return;
    }

    struct dirent* entry;
    printf("SUCCESS\n");
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char file_path[512];
            sprintf(file_path, "%s/%s", path, entry->d_name);

            FILE* file = fopen(file_path, "r");
            if (file == NULL) {
                printf("ERROR\nCannot open file: %s\n", file_path);
                continue;
            }

            int sectionLines = 0;
            int foundSection = 0;
            char line[1024];

            while (fgets(line, sizeof(line), file) != NULL) {
                if (strcmp(line, "===\n") == 0) {
                   
                    if (sectionLines > 16){ 
                        printf("%s\n", file_path);
                        foundSection = 1;
                    }
                    sectionLines = 0;
                } else {
                    
                    sectionLines++;
                }
            }

            
            if (sectionLines > 16) {
                printf("%s\n", file_path);
                foundSection = 1;
            }

            if (!foundSection) {
                printf("No section with more than 16 lines found in file: %s\n", file_path);
            }

            fclose(file);
        }
    }

    closedir(dir);
}

int main(int argc, char** argv) {
    if (argc >= 2) {
        if (strcmp(argv[1], "variant") == 0) {
            printf("55429\n");
        } else if (strcmp(argv[1], "list") == 0) {
            if (argc >= 3) {
                char* path = NULL;

                if (strcmp(argv[2], "recursive") == 0) {
                    if (argc >= 4 && strstr(argv[3], "path=")) {
                        path = malloc(strlen(argv[3]) - 4);
                        strcpy(path, &argv[3][5]);
                        listRecursive(path, 0);
                    } else {
                        printf("ERROR\nInvalid path argument\n");
                    }
                } else if (strstr(argv[2], "path=")) {
                    path = malloc(strlen(argv[2]) - 5);
                    strcpy(path, argv[2] + 5);
                    if (list(path) == -1) {
                        printf("ERROR\nInvalid directory path\n");
                    }
                } else if (strstr(argv[2], "name_starts_with=") && strstr(argv[3], "path=")) {
                    char* path = malloc(strlen(argv[3]) - 5);
                    strcpy(path, argv[3] + 5);
                    char* start = malloc(strlen(argv[2]) - 17);
                    strcpy(start, argv[2] + 17);
                    if (printStartsWith(path, start) == -1) {
                        printf("ERROR\nInvalid directory path\n");
                    }
                    free(path);
                    free(start);
                } else {
                    printf("ERROR\nInvalid arguments\n");
                }

                if (path != NULL) {
                    free(path);
                }
            } else {
                printf("ERROR\nInsufficient arguments\n");
            }
        } else if (strcmp(argv[1], "parse") == 0) {
         char* path = NULL;
            for (int i = 2; i < argc; i++) {
                if (strstr(argv[i], "path=") != NULL) {
                    path = argv[i] + 5;
                    break;
                }
            }

            if (path != NULL) {
                parse(path);
            } else {
                printf("ERROR\nInvalid command line arguments\n");
            }
            
        }else if (strcmp(argv[1], "findall") == 0) {
            if (argc >= 3) {
                char* path = strstr(argv[2], "path=");
                if (path != NULL) {
                    path += 5;
                    findall(path);
                }
                }
                
    } else {
        printf("ERROR\nNo command specified\n");
    }
}
    return 0;
}




    
        
    

    


