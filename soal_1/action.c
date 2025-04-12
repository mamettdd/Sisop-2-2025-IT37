#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_FILES 100
#define MAX_FILENAME 50
#define MAX_CONTENT 1000

void download_and_unzip() {
    DIR* dir = opendir("Clues");
    if (dir) {
        closedir(dir);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        char *args[] = {"wget", "-q", "https://drive.google.com/uc?export=download&id=1xFn1OBJUuSdnApDseEczKhtNzyGekauK", "-O", "Clues.zip", NULL};
        execvp("wget", args);
        perror("execvp wget");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }

    pid = fork();
    if (pid == 0) {
        char *args[] = {"unzip", "-q", "Clues.zip", NULL};
        execvp("unzip", args);
        perror("execvp unzip");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }

    remove("Clues.zip");
}

bool is_valid_filename(const char *filename) {
    if (strlen(filename) != 5) return false;
    return (isalpha(filename[0]) || isdigit(filename[0])) && (strcmp(&filename[1], ".txt") == 0);
}

void filter_files() {
    mkdir("Filtered", 0755);

    struct dirent *entry;
    DIR *dp;

    for (int i = 0; i < 4; i++) {
        char dirname[20];
        sprintf(dirname, "Clues/Clue%c", 'A' + i);

        dp = opendir(dirname);
        if (dp == NULL) {
            perror("opendir");
            continue;
        }

        while ((entry = readdir(dp)) != NULL) {
            if (entry->d_type == DT_REG && is_valid_filename(entry->d_name)) {
                char old_path[256], new_path[256];
                sprintf(old_path, "%s/%s", dirname, entry->d_name);
                sprintf(new_path, "Filtered/%s", entry->d_name);

                if (rename(old_path, new_path) != 0) {
                    perror("rename");
                }
            } else if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt") != NULL) {
                char filepath[256];
                sprintf(filepath, "%s/%s", dirname, entry->d_name);
                if (remove(filepath) != 0) {
                    perror("remove");
                }
            }
        }
        closedir(dp);
    }
}

int compare_files(const void *a, const void *b) {
    const char *file1 = *(const char **)a;
    const char *file2 = *(const char **)b;

    if (isdigit(file1[0]) && !isdigit(file2[0])) return -1;
    if (!isdigit(file1[0]) && isdigit(file2[0])) return 1;

    if (isdigit(file1[0]) && isdigit(file2[0])) {
        return file1[0] - file2[0];
    }

    return file1[0] - file2[0];
}

void combine_files() {
    FILE *combined = fopen("Combined.txt", "w");
    if (!combined) {
        perror("Error creating Combined.txt");
        return;
    }

    // Urutan file: 1.txt, a.txt, 2.txt, b.txt, dst.
    const char *files[] = {"1.txt", "a.txt", "2.txt", "b.txt", "3.txt", "c.txt", "4.txt", "d.txt", "5.txt", "e.txt", "6.txt", "f.txt", "7.txt"};
    int num_files = 13;

    for (int i = 0; i < num_files; i++) {
        char filepath[256];
        sprintf(filepath, "Filtered/%s", files[i]);

        FILE *file = fopen(filepath, "r");
        if (file) {
            char ch;
            if (fread(&ch, 1, 1, file) == 1) {
                fputc(ch, combined);  // Tulis karakter ke Combined.txt
            }
            fclose(file);
        }
    }

    fclose(combined);
}

void rot13(char *str) {
    for (int i = 0; str[i]; i++) {
        if (isalpha(str[i])) {
            if ((tolower(str[i]) - 'a') < 13) {
                str[i] += 13;
            } else {
                str[i] -= 13;
            }
        }
    }
}

void decode_file() {
    // Buka file Combined.txt
    FILE *combined = fopen("Combined.txt", "r");
    if (combined == NULL) {
        perror("Error opening Combined.txt");
        return;
    }

    // Baca isi file
    char content[MAX_CONTENT];
    if (!fgets(content, MAX_CONTENT, combined)) {
        fclose(combined);
        perror("Error reading Combined.txt");
        return;
    }
    fclose(combined);

    // Lakukan ROT13 decoding
    rot13(content);

    // Buka file Decoded.txt untuk ditulis
    FILE *decoded = fopen("Decoded.txt", "w");
    if (decoded == NULL) {
        perror("Error creating Decoded.txt");
        return;
    }

    // Tulis hasil decode
    fputs(content, decoded);
    fclose(decoded);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        download_and_unzip();
    } else if (argc == 3 && strcmp(argv[1], "-m") == 0) {
        if (strcmp(argv[2], "Filter") == 0) {
            filter_files();
        } else if (strcmp(argv[2], "Combine") == 0) {
            combine_files();
        } else if (strcmp(argv[2], "Decode") == 0) {
            decode_file();
        } else {
            printf("Invalid argument. Usage:\n");
            printf("./action -m Filter\n");
            printf("./action -m Combine\n");
            printf("./action -m Decode\n");
        }
    } else {
        printf("Usage:\n");
        printf("./action                  - Download and unzip Clues.zip\n");
        printf("./action -m Filter        - Filter files\n");
        printf("./action -m Combine       - Combine file contents\n");
        printf("./action -m Decode        - Decode file contents\n");
    }

    return 0;
}
