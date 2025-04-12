#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <limits.h>

#define FOLDER_QUARANTINE "./quarantine"
#define FOLDER_STARTERKIT "./starter_kit"

void downloadZipFile() { // soal 1
    char *file_id = "1_5GxIGfQr3mNKuavJbte_AoRkEQLXSKS";
    char *filename = "starterkit.zip";

    struct stat st;

    if (stat(FOLDER_STARTERKIT, &st) == -1) {
        mkdir(FOLDER_STARTERKIT, 0777);
    }

    char command[512];
    snprintf(command, sizeof(command),
    "wget --no-check-certificate \"https://docs.google.com/uc?export=download&id=%s\" -O %s",
    file_id, filename);

    system(command);
    system("unzip starterkit.zip -d starter_kit");
    system("rm starterkit.zip");
}

void writeLog(const char *message) { // log untuk soal terakhir
    FILE *logfile = fopen("activity.log", "a");
    if (!logfile) {
        perror("Gagal membuka log file");  // Debug
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "[%d-%m-%Y][%H:%M:%S]", t);

    fprintf(logfile, "%s - %s\n", timebuf, message);
    fclose(logfile);
}

void decryptFileName() { // soal 2
    pid_t pid = fork();
    if (pid > 0) exit(0); // Parent keluar
    if (pid < 0) exit(1); // Fork gagal

    setsid();

    // Log
    char logbuf[128];
    snprintf(logbuf, sizeof(logbuf), "Successfully started decryption process with PID %d.", getpid());
    writeLog(logbuf);

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    chdir(cwd);

    struct stat st;

    if (stat(FOLDER_QUARANTINE, &st) == -1) {
        mkdir(FOLDER_QUARANTINE, 0777);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    while (1) {
        DIR *folder = opendir(FOLDER_QUARANTINE);
        struct dirent *entry;

        if (folder != NULL) {
            while ((entry = readdir(folder)) != NULL) {
                if (entry->d_type == DT_REG) {
                    char oldpath[512], newname[256], newpath[512];
                    snprintf(oldpath, sizeof(oldpath), "%s/%s", FOLDER_QUARANTINE, entry->d_name);

                    // Cek apakah nama ada ekstensinya jika tidak ada di lewati
                    size_t len = strlen(entry->d_name);
                    if (strrchr(entry->d_name, '.') != NULL) continue;

                    char command[512];
                    snprintf(command, sizeof(command), "echo %s | base64 -d", entry->d_name);

                    FILE *fp = popen(command, "r");
                    if (fp == NULL) continue;

                    if (fgets(newname, sizeof(newname), fp) == NULL) {
                        pclose(fp);
                        continue;
                    }

                    pclose(fp);
                    newname[strcspn(newname, "\n")] = 0;

                    snprintf(newpath, sizeof(newpath), "%s/%s", FOLDER_QUARANTINE, newname);
                    rename(oldpath, newpath);
                }
            }
            closedir(folder);
        }
        sleep(5);
    }
}

void quarantineFiles() { // soal 3
    DIR *folder = opendir(FOLDER_STARTERKIT);
    struct dirent *entry;

    if (folder != NULL) {
        while ((entry = readdir(folder)) != NULL) {
            if (entry->d_type == DT_REG) {
                char oldpath[512], encoded[256], newpath[512];

                snprintf(oldpath, sizeof(oldpath), "%s/%s", FOLDER_STARTERKIT, entry->d_name);
                snprintf(newpath, sizeof(newpath), "%s/%s", FOLDER_QUARANTINE, entry->d_name);

                rename(oldpath, newpath);

                //Log
                char logbuf[512];
                snprintf(logbuf, sizeof(logbuf), "%s - Successfully moved to quarantine directory.", entry->d_name);
                writeLog(logbuf);
            }
        }
        closedir(folder);
    }
}

void returnFiles() { // soal 3 juga
    DIR *folder = opendir(FOLDER_QUARANTINE);
    struct dirent *entry;

    if (folder != NULL) {
        while ((entry = readdir(folder)) != NULL) {
            if (entry->d_type == DT_REG) {
                char oldpath[512], newname[256], newpath[512];

                snprintf(oldpath, sizeof(oldpath), "%s/%s", FOLDER_QUARANTINE, entry->d_name);
                snprintf(newpath, sizeof(newpath), "%s/%s", FOLDER_STARTERKIT, entry->d_name);

                rename(oldpath, newpath);

                //Log
                char logbuf[512];
                snprintf(logbuf, sizeof(logbuf), "%s - Successfully returned to starter kit directory.", entry->d_name);
                writeLog(logbuf);
            }
        }
        closedir(folder);
    }
}

void eradicateFiles() { // soal 4
    DIR *folder = opendir(FOLDER_QUARANTINE);
    struct dirent *entry;

    if (folder == NULL) return;

    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", FOLDER_QUARANTINE, entry->d_name);
            remove(filepath); 

            //Log
            char logbuf[512];
            snprintf(logbuf, sizeof(logbuf), "%s - Successfully deleted.", entry->d_name);
            writeLog(logbuf);
        }
    }
    closedir(folder);
}

void shutdownDecrypt() { // soal 5
    FILE *fp = popen("pgrep -af starterkit", "r");
    if (!fp) return;

    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        pid_t pid;
        char cmd[256];

        if (sscanf(line, "%d %[^\n]", &pid, cmd) == 2) { // Pisah PID dan command
            if (strstr(cmd, "--decrypt") && pid != getpid()) { // Spesifikan untuk starterkit --decrypt
                kill(pid, SIGTERM);

                char logbuf[512];
                snprintf(logbuf, sizeof(logbuf), "Successfully shut off decryption process with PID %d.", pid);
                writeLog(logbuf);
            }
        }
    }
    pclose(fp);
}

int main(int argc, char *argv[]) {

    // Tidak ada argumen, tidak apa-apa
    if (argc == 1) {
        downloadZipFile();
        return 0;
    }

    if (
        strcmp(argv[1], "--decrypt") != 0 &&
        strcmp(argv[1], "--quarantine") != 0 &&
        strcmp(argv[1], "--return") != 0 &&
        strcmp(argv[1], "--eradicate") != 0 &&
        strcmp(argv[1], "--shutdown") != 0
    ) {
        fprintf(stderr, "Argumen %s tidak dikenal\n", argv[1]);
        fprintf(stderr, "Gunakan argumen: --decrypt, --quarantine, --return, --eradicate, atau --shutdown\n");
        return 1;
    }

    downloadZipFile();
    
    if (strcmp(argv[1], "--decrypt") == 0) {
        decryptFileName();
    } else if (strcmp(argv[1], "--quarantine") == 0) {
        quarantineFiles();
    } else if (strcmp(argv[1], "--return") == 0) {
        returnFiles();
    } else if (strcmp(argv[1], "--eradicate") == 0) {
        eradicateFiles();
    } else if (strcmp(argv[1], "--shutdown") == 0) {
        shutdownDecrypt();
    }

    return 0;
}