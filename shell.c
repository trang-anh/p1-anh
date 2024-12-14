#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>  // for open()

#define MAX_INPUT_SIZE 1024  
#define MAX_ARG_COUNT 100    

// declarations
void execute_command(char *input);
void process_commands(char *input);
int handle_builtin_commands(char **args);
void handle_redirection(char **args);
void handle_pipe_with_redirection(char *input);
char *split_commands(char *input);

char prev_command[MAX_INPUT_SIZE] = "";  // store previous command

void parse_input(char *input, char **args) {
    int i = 0;
    int in_quotes = 0;
    int escape = 0;  // to handle escape sequences like \n
    char *arg = malloc(MAX_INPUT_SIZE);  // temp buffer for each argument
    int arg_len = 0;

    while (*input != '\0') {
        if (*input == '\\' && in_quotes) {
            escape = 1;
        } else if (*input == '"' && !escape) {
            in_quotes = !in_quotes;
        } else if (!in_quotes && (*input == ' ' || *input == '\n')) {
            if (arg_len > 0) {
                arg[arg_len] = '\0';
                args[i] = malloc(strlen(arg) + 1);
                strcpy(args[i], arg);
                i++;
                arg_len = 0;
            }
        } else {
            if (escape && *input == 'n') {
                arg[arg_len++] = '\n';
            } else {
                arg[arg_len++] = *input;
            }
            escape = 0;
        }
        input++;
    }

    if (arg_len > 0) {
        arg[arg_len] = '\0';
        args[i] = malloc(strlen(arg) + 1);
        strcpy(args[i], arg);
        i++;
    }

    args[i] = NULL;
    free(arg);
}


/**
 * built-in commands: cd, source, prev, help, exit
 */
int handle_builtin_commands(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else if (chdir(args[1]) != 0) {
            perror("cd");
        }
        return 1;
    }

    if (strcmp(args[0], "source") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "source: missing file name\n");
        } else {
            FILE *file = fopen(args[1], "r");
            if (file == NULL) {
                perror("source");
            } else {
                char line[MAX_INPUT_SIZE];
                while (fgets(line, sizeof(line), file) != NULL) {
                    size_t len = strlen(line);
                    if (len > 0 && line[len - 1] == '\n') {
                        line[len - 1] = '\0';
                    }
                    process_commands(line);
                }
                fclose(file);
            }
        }
        return 1;
    }

    if (strcmp(args[0], "prev") == 0) {
        if (strlen(prev_command) > 0) {
            process_commands(prev_command);
        } else {
            printf("No previous command.\n");
        }
        return 1;
    }

    if (strcmp(args[0], "help") == 0) {
        printf("Available built-in commands:\n");
        printf("cd [directory]  - Change the current directory\n");
        printf("source [file]   - Execute commands from a file\n");
        printf("prev            - Repeat the last command\n");
        printf("help            - Show this help message\n");
        printf("exit            - Exit the shell\n");
        return 1;
    }

    if (strcmp(args[0], "exit") == 0) {
        printf("Bye bye.\n");
        exit(0);
    }

    return 0;  // Not a built-in command
}

/**
 * Handle input and output redirection.
 */
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            fflush(stdout);
            args[i] = NULL;
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            break;
        }
        if (strcmp(args[i], "<") == 0) {
            args[i] = NULL;
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("open");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            break;
        }
    }
}

/**
 * Executes a single command using fork and execvp, with support for redirection.
 */
void execute_command(char *input) {
    char *args[MAX_ARG_COUNT];

    parse_input(input, args);

    if (handle_builtin_commands(args)) return;

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
    } else if (pid == 0) {
        handle_redirection(args);
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "%s: command not found\n", args[0]);
        }
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

/**
 * Handle pipe commands combined with input redirection.
 */
void handle_pipe_with_redirection(char *input) {
    char *commands[MAX_ARG_COUNT];
    int num_commands = 0;

    commands[num_commands] = strtok(input, "|");
    while (commands[num_commands] != NULL) {
        num_commands++;
        commands[num_commands] = strtok(NULL, "|");
    }

    int pipe_fd[2 * (num_commands - 1)];

    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipe_fd + i * 2) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    for (int i = 0; i < num_commands; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            if (i > 0) {
                dup2(pipe_fd[(i - 1) * 2], STDIN_FILENO);
            }
            if (i < num_commands - 1) {
                dup2(pipe_fd[i * 2 + 1], STDOUT_FILENO);
            }
            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipe_fd[j]);
            }
            char *args[MAX_ARG_COUNT];
            parse_input(commands[i], args);
            if (i == 0) {
                handle_redirection(args);
            }
            if (execvp(args[0], args) == -1) {
                perror(args[0]);
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipe_fd[i]);
    }

    for (int i = 0; i < num_commands; i++) {
        wait(NULL);
    }
}

/**
 * Splits input by semicolons and executes each command independently.
 * Semicolons inside quotes are treated as part of the argument, not as separators.
 */
void process_commands(char *input) {
    // Save the entire input as prev_command if it's not 'prev'
    if (strcmp(input, "prev") != 0) {
        strcpy(prev_command, input);
    }

    char *command;
    while ((command = split_commands(input)) != NULL) {
        while (*command == ' ') command++;  // Trim leading spaces
        if (strchr(command, '|') != NULL) {
            handle_pipe_with_redirection(command);
        } else {
            execute_command(command);
        }
        input = NULL;  // Use NULL for subsequent calls to split_commands
    }
}

/**
 * Splits up the commands by semicolons, respecting quotes.
 */
char *split_commands(char *input) {
    static char *last_pos = NULL;
    if (input != NULL) {
        last_pos = input;
    } else if (last_pos == NULL) {
        return NULL;
    }
    char *start = last_pos;
    int in_quotes = 0;

    while (*start == ' ' || *start == '\t') {
        start++;
    }

    if (*start == '\0') {
        last_pos = NULL;
        return NULL;
    }

    char *current = start;
    while (*current != '\0') {
        if (*current == '"') {
            in_quotes = !in_quotes;
        } else if (*current == ';' && !in_quotes) {
            *current = '\0';  // Replace semicolon with null terminator
            last_pos = current + 1;
            return start;  // Return the current command
        }
        current++;
    }

    last_pos = NULL;
    return start;  // Return the remaining command if no more semicolons
}

int main(int argc, char **argv) {
    char input[MAX_INPUT_SIZE];

    printf("Welcome to mini-shell.\n");

    while (1) {
        printf("shell $ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nBye bye.\n");
            break;
        }

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        process_commands(input);
    }

    return 0;
}
