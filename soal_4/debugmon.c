#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>

#define LOG_FILE "debugmon.log"
#define MAX_LINE 1024
#define DAEMON_IDENTIFIER "debugmon_daemon_process"

void write_log(const char *process, const char *status) {
    time_t now;
    time(&now);
    struct tm *tm_info = localtime(&now);
    
    char timestamp[50];
    strftime(timestamp, sizeof(timestamp), "[%d:%m:%Y]-[%H:%M:%S]", tm_info);
    
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        fprintf(log, "%s_%s_STATUS(%s)\n", timestamp, process, status);
        fclose(log);
    }
}

void list_processes(const char *user) {
    printf("Listing processes for user: %s\n", user);
    
    char command[MAX_LINE];
    snprintf(command, sizeof(command), "ps -u %s -o pid,cmd,%%cpu,%%mem 2>/dev/null", user);
    
    FILE *ps_output = popen(command, "r");
    if (!ps_output) {
        perror("Failed to execute ps command");
        return;
    }
    
    printf("PID\tCOMMAND\t\t\tCPU\tMEM\n");
    printf("------------------------------------------------\n");
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), ps_output)) {
        printf("%s", line);
    }
    
    pclose(ps_output);
    write_log("process_list", "RUNNING");
}

void start_daemon(const char *user) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        printf("Debugmon daemon started for user %s (PID: %d)\n", user, pid);
        write_log(DAEMON_IDENTIFIER, "RUNNING");
        exit(EXIT_SUCCESS);
    }
    
    umask(0);
    setsid();
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    while (1) {
        sleep(30);
        write_log("daemon_monitoring", "RUNNING");
    }
}

void stop_daemon(const char *user) {
    printf("Stopping debugmon daemon for user: %s\n", user);
    
    char command[MAX_LINE];
    snprintf(command, sizeof(command), "pgrep -f '%s' 2>/dev/null", DAEMON_IDENTIFIER);
    
    FILE *pgrep_output = popen(command, "r");
    if (!pgrep_output) {
        perror("Failed to find daemon process");
        return;
    }
    
    char pid_str[16];
    int found = 0;
    
    while (fgets(pid_str, sizeof(pid_str), pgrep_output)) {
        pid_t pid = atoi(pid_str);
        if (kill(pid, SIGTERM) == 0) {
            printf("Successfully stopped daemon (PID: %d)\n", pid);
            write_log(DAEMON_IDENTIFIER, "RUNNING");
            found = 1;
        } else {
            fprintf(stderr, "Failed to kill process %d: %s\n", pid, strerror(errno));
        }
    }
    
    if (!found) {
        printf("No running daemon found for user %s\n", user);
    }
    
    pclose(pgrep_output);
}

void fail_processes(const char *user) {
    printf("Failing all processes for user: %s\n", user);
    
    char command[MAX_LINE];
    snprintf(command, sizeof(command), "ps -u %s -o pid= 2>/dev/null", user);
    
    FILE *ps_output = popen(command, "r");
    if (!ps_output) {
        perror("Failed to get user processes");
        return;
    }
    
    char pid_str[16];
    int count = 0;
    
    while (fgets(pid_str, sizeof(pid_str), ps_output)) {
        pid_t pid = atoi(pid_str);
        if (pid > 1 && kill(pid, SIGKILL) == 0) {  
            printf("Killed process: %d\n", pid);
            write_log("process_kill", "FAILED");
            count++;
        }
    }
    
    pclose(ps_output);
    printf("Total processes killed: %d\n", count);
    
    write_log("user_block", "FAILED");
}

void revert_block(const char *user) {
    printf("Reverting block for user: %s\n", user);
    
    write_log("user_unblock", "RUNNING");
}

void print_usage(const char *program_name) {
    printf("Usage: %s <command> <user>\n", program_name);
    printf("Commands:\n");
    printf("  list <user>    - List all processes for user\n");
    printf("  daemon <user>  - Start monitoring daemon\n");
    printf("  stop <user>    - Stop monitoring daemon\n");
    printf("  fail <user>    - Kill all processes for user\n");
    printf("  revert <user>  - Allow user to run processes again\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    const char *command = argv[1];
    const char *user = argv[2];
    
    if (strcmp(command, "list") == 0) {
        list_processes(user);
    } 
    else if (strcmp(command, "daemon") == 0) {
        start_daemon(user);
    } 
    else if (strcmp(command, "stop") == 0) {
        stop_daemon(user);
    } 
    else if (strcmp(command, "fail") == 0) {
        fail_processes(user);
    } 
    else if (strcmp(command, "revert") == 0) {
        revert_block(user);
    } 
    else {
        fprintf(stderr, "Invalid command: %s\n", command);
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
