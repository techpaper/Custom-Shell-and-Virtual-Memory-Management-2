#include <bits/stdc++.h>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <cstdlib>

using namespace std;

#define HISTORY_PATH ".shell_history.txt"

#define COMMAND_LEN 200
#define PATH_LEN 200
#define ARG_MAX 50
#define MAX 50

int PROCESS_ID = -1;
int NUMFIFO = 0;
pid_t PARENT_PROCESS_ID = -1;
void ctrlcSignalHandler(int sig_num) {}

void ctrlzSignalHandler(int sig_num) {
    if (PROCESS_ID != 0) {
        fflush(stdout);
        kill(PROCESS_ID, SIGSTOP);
    }
}

void removeExtraWhitespaces(char* str) {
    int i, j;
    char newString[COMMAND_LEN];

    i = 0;
    j = 0;

    while (str[i] != '\0') {
        if (str[i] == ' ') {
            newString[j] = ' ';
            j++;
            while (str[i] == ' ') i++;
        }
        newString[j] = str[i];
        i++;
        j++;
    }
    newString[j] = '\0';
    strcpy(str, newString);
}

int splitCommand(char* command, char** argv, char** infile, char** outfile,
                 const char* delim) {
    int argc = 0;
    int i = 0;

    char* temp = strtok(command, delim);
    while (temp != NULL) {
        if (temp[0] == '>') {
            *outfile = strtok(NULL, delim);
        } else if (temp[0] == '<') {
            *infile = strtok(NULL, delim);
        } else {
            argv[i] = temp;
            argc++;
        }
        i++;
        temp = strtok(NULL, delim);
    }
    return argc;
}

void runExternal(char* command) {
    char* argv[ARG_MAX] = {0};

    char *infile = 0, *outfile = 0;

    int argc = splitCommand(command, argv, &infile, &outfile, " \n");

    if (infile) {
        int file = open(infile, O_RDONLY);
        close(0);  // Close stdin
        dup(file);
        close(file);
    }
    if (outfile) {
        int file = open(outfile, O_CREAT | O_WRONLY | O_TRUNC,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        close(1);  // Close stdout
        dup(file);
        close(file);
    }

    execvp(argv[0], argv);
    perror("Error in executing the command");

    kill(getpid(), SIGTERM);
    exit(0);
}
void executePipe(char** args, int num_pipes) {
    // define num_pipes-1 pipes
    int pipe_array[num_pipes - 1][2];

    for (int i = 0; i < num_pipes - 1; i++) {
        // create pipe
        if (pipe(pipe_array[i]) < 0) {
            printf("Pipe Could not be created!");
            return;
        }
    }

    // create num_pipes child processes
    pid_t pd;
    for (int i = 0; i < num_pipes; i++) {
        if ((pd = fork()) == 0) {
            if (i != 0) {
                close(pipe_array[i - 1][1]);
                dup2(pipe_array[i - 1][0], 0);
                close(pipe_array[i - 1][0]);
            }

            if (i != num_pipes - 1) {
                close(pipe_array[i][0]);
                dup2(pipe_array[i][1], 1);
                close(pipe_array[i][1]);
            }

            for (int j = 0; j < num_pipes - 1; j++) {
                close(pipe_array[j][0]);
                close(pipe_array[j][1]);
            }

            runExternal(args[i]);

            exit(0);
        } else {
            for (int j = 0; j < i; j++) {
                close(pipe_array[j][0]);
                close(pipe_array[j][1]);
            }
        }
    }
    int status = 0;
    while (wait(&status) > 0) {
        if (status < 0) {
            perror("Error in executing pipe.");
            exit(0);
        }
    }
    exit(0);
}

int extractCommandsForMultiWatch(char* command, char** cmds) {
    int i = 0;
    cmds[i] = strtok(command, "\"");
    cmds[i] = strtok(NULL, "\"");
    while (cmds[i] != NULL) {
        i++;
        cmds[i] = strtok(NULL, "\"");
        cmds[i] = strtok(NULL, "\"");
    }
    return i;
}

void showHistory() {
    fstream fin(HISTORY_PATH,
                std::fstream::in | std::fstream::out | std::fstream::app);
    char buff[COMMAND_LEN];
    while (fin.getline(buff, COMMAND_LEN)) {
        cout << buff << endl;
    }
    fin.close();
}
void saveHistory(char* cmd) {
    fstream fin(HISTORY_PATH,
                std::fstream::in | std::fstream::out | std::fstream::app);
    removeExtraWhitespaces(cmd);
    fin << cmd << endl;
    char buff[10010][COMMAND_LEN];
    int count = 0;
    while (fin.getline(buff[count], COMMAND_LEN)) {
        removeExtraWhitespaces(buff[count]);
        count++;
    }
    fin.close();
    fstream tin(HISTORY_PATH, std::fstream::in | std::fstream::out |
                                  std::fstream::app | std::fstream::trunc);
    if (count > 10000) {
        for (int i = 1; i < 10001; i++) {
            tin << buff[i] << endl;
        }
    } else {
        for (int i = 0; i < 10000; i++) {
            tin << buff[i] << endl;
        }
    }
}
void autoComplete(char* cmd_buffer) {
    char lastword[50] = {0};
    char* p = strrchr(cmd_buffer, ' ');
    if (p) {
        p++;
        for (int i = 0; p != 0 && *p != '\0'; i++, p++) {
            lastword[i] = *p;
        }
    } else {
        strcat(lastword, cmd_buffer);
    }
    struct dirent* list[1000];
    int count = 0;
    if (strlen(lastword) == 0) {
        DIR* d;
        struct dirent* dir;
        d = opendir(".");
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_type == 8) {
                    list[count] = dir;
                    count++;
                }
            }
            closedir(d);
        }
    } else {
        DIR* d;
        struct dirent* dir;
        d = opendir(".");

        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (strncmp(lastword, dir->d_name, strlen(lastword)) == 0 &&
                    dir->d_type == 8) {
                    list[count] = dir;
                    count++;
                }
            }
            closedir(d);
        }
    }
    printf("\n");
    if (count == 0) {
        printf("No files with the given initials found\n");
        exit(0);
    } else {
        printf("Enter the desired number to choose the file else enter 0\n");

        for (int i = 0; i < count; i++) {
            printf("%d )  %s\n", i + 1, list[i]->d_name);
        }
    }
    int op;
    scanf("%d", &op);
    if (op == 0) {
        exit(0);
    } else {
        for (int j = strlen(cmd_buffer) - 1; j >= 0;) {
            if (!isspace(cmd_buffer[j])) {
                cmd_buffer[j] = 0;
                if (j != 0)
                    j--;
                else
                    break;
            } else {
                break;
            }
        }
        strcat(cmd_buffer, list[op - 1]->d_name);

        removeExtraWhitespaces(cmd_buffer);

        printf("%s\n", cmd_buffer);
    }
}
int longestCommonSubstringLen(char* s, char* t, int m, int n) {
    int longest[m + 1][n + 1];
    int len = 0;
    int row, col;
    for (int i = 0; i <= m; i++) {
        for (int j = 0; j <= n; j++) {
            if (i == 0 || j == 0)
                longest[i][j] = 0;
            else if (s[i - 1] == t[j - 1]) {
                longest[i][j] = longest[i - 1][j - 1] + 1;
                if (len < longest[i][j]) {
                    len = longest[i][j];
                    row = i;
                    col = j;
                }
            } else
                longest[i][j] = 0;
        }
    }
    return len;
}

void reversesearch() {
    printf("\nreverse-search: ");
    char term[100] = {0};
    scanf("%[^\n]s", term);
    fstream fin(HISTORY_PATH,
                std::fstream::in | std::fstream::out | std::fstream::app);
    char buff[10010][COMMAND_LEN];
    int count = 0;
    while (fin.getline(buff[count], COMMAND_LEN)) {
        removeExtraWhitespaces(buff[count]);
        count++;
    }
    fin.close();
    vector<int> res;
    for (int i = 0; i < count; i++) {
        if (strcmp(buff[i], term) == 0) {
            res.push_back(i);
        }
    }
    if (res.empty()) {
        int mx = 0;
        for (int i = 0; i < count; i++) {
            int len = longestCommonSubstringLen(buff[i], term, strlen(buff[i]),
                                                strlen(term));
            mx = max(len, mx);
        }
        if (mx > 2) {
            for (int i = 0; i < count; i++) {
                int len = longestCommonSubstringLen(
                    buff[i], term, strlen(buff[i]), strlen(term));
                if (len == mx) {
                    res.push_back(i);
                }
            }
        }
    }
    printf("\n");
    if (res.empty()) {
        printf("No match for search term in history\n");
    } else {
        for (int i = 0; i < res.size(); i++) {
            printf("%s\n", buff[res[i]]);
        }
    }
}

void runMultiWatch(char* command) {
    char* cmds[MAX] = {0};
    int numofcmds = extractCommandsForMultiWatch(command, cmds);
    NUMFIFO = numofcmds;
    int pipe_array[numofcmds][2];
    struct pollfd fds[numofcmds];
    for (int i = 0; i < numofcmds; i++) {
        if (pipe(pipe_array[i]) < 0) {
            printf("Pipe Could not be created!");
            return;
        }
        fds[i].fd = pipe_array[i][0];
        fds[i].events = 0;
        fds[i].events |= POLLIN;
    }
    pid_t pd;
    for (int i = 0; i < numofcmds; i++) {
        if ((pd = fork()) == 0) {
            dup2(pipe_array[i][1], 1);
            // close(1);  // Close stdout
            close(pipe_array[i][1]);

            runExternal(cmds[i]);

            exit(0);
        }
    }

    char arr1[8000];
    while (true) {
        int ret = poll(fds, numofcmds, -1);
        if (ret > 0) {
            for (int i = 0; i < numofcmds; i++) {
                if (fds[i].revents & POLLIN) {
                    int t = read(pipe_array[i][0], arr1, 8000);
                    if (t < 0) {
                        perror("Error in reading ");
                        exit(0);
                    }
                    arr1[t] = 0;
                    printf("\"%s\", %lu\n", cmds[i], (unsigned long)time(NULL));
                    printf("<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n\n");
                    printf("%s\n", arr1);
                    printf("->->->->->->->->->->->->->->->->->->->\n\n\n");
                }
            }
        }
        // close(pipe_array[i%numofcmds][1]);
        fflush(stdout);
    }
    exit(0);
}

int main() {
    signal(SIGINT, ctrlcSignalHandler);
    signal(SIGTSTP, ctrlzSignalHandler);

    static struct termios oldt, newt;
    tcgetattr(0, &oldt);
    /*now the settings will be copied*/
    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    tcsetattr(0, TCSANOW, &newt);

    cout << "\033[1;32mWelcome to the Shell :\033[0m\n";

    while (true) {
        char addr[PATH_LEN];
        char* add_ptr;

        // get current directory
        add_ptr = getcwd(addr, sizeof(addr));
        // error in getcwd
        if (add_ptr == NULL) {
            perror("Error in getting the current directory");
            continue;
        }

        // print present working directory in blue colour
        cout << "\033[1;34m" << addr << "\033[0m"
             << "$ ";

        char cmd_buffer[COMMAND_LEN];
        char last_char = -1;
        for (int i = 0; i < COMMAND_LEN; i++) {
            last_char = getchar();
            if (last_char == '\n' || last_char == '\t' || last_char == 18) {
                cmd_buffer[i] = '\0';
                break;
            } else {
                cmd_buffer[i] = last_char;
            }
        }

        // cin.getline(cmd_buffer, COMMAND_LEN, '\n');

        removeExtraWhitespaces(cmd_buffer);

        if (strcmp(cmd_buffer, "exit()") == 0) {
            cout << "\033[1;32mExiting from the Shell :\033[0m\n";
            exit(0);
        }
        saveHistory(cmd_buffer);
        pid_t p = fork();
        int status = 0;
        if (p < 0) {
            perror("Error in Executing the command");
            continue;
        } else if (p == 0) {
            // child process
            PROCESS_ID = 0;
            NUMFIFO = 0;
            char firstword[50] = {0};
            for (int i = 0; i < strlen(cmd_buffer); i++) {
                if (isspace(cmd_buffer[i])) {
                    break;
                }
                firstword[i] = cmd_buffer[i];
            }
            if (last_char == 18) {
                reversesearch();
            } else if (last_char == '\t') {
                autoComplete(cmd_buffer);
            } else if (strcmp(firstword, "multiwatch") == 0) {
                runMultiWatch(cmd_buffer);
            } else if (strcmp(firstword, "history") == 0) {
                showHistory();
            } else {
                char* pipe_args[MAX];
                int i = 0;
                pipe_args[i] = strtok(cmd_buffer, "|");

                while (pipe_args[i] != NULL) {
                    i++;
                    pipe_args[i] = strtok(NULL, "|");
                }
                executePipe(pipe_args, i);
            }
            exit(0);
        } else {
            // parent process
            PROCESS_ID = p;

            if (cmd_buffer[strlen(cmd_buffer) - 1] != '&') {
                waitpid(-1, &status, WUNTRACED);

                if (WIFSTOPPED(status)) {
                    fflush(stdout);
                    kill(p, SIGCONT);
                }
            }
            if (status < 0) {
                perror("Error in child process");
                exit(-1);
            }
        }
    }
}
