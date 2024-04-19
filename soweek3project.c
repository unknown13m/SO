#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <libgen.h>

#define MAX_ARGS 10

void print_file_permissions(struct stat st, const int f) {
    char buffer[40];
    int size = snprintf(buffer, sizeof(buffer), "File Permissions: ");
    const char *perms[] = {"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};

    for (int i = 2; i >= 0; --i) {
        int mode = (st.st_mode >> (i * 3)) & 0x7;
        size += snprintf(buffer + size, sizeof(buffer) - size, "%s", perms[mode]);
    }
    write(f, buffer, size);
}

void file_info(const char *file_name, const int f) {
    struct stat st;
    char buffer[8192];
    int size = 0;

    if (lstat(file_name, &st) == -1) {
        size += snprintf(buffer, sizeof(buffer), "Unable to get file information!\n");
        exit(-1);
    }

    size += snprintf(buffer + size, sizeof(buffer) - size, "Name: %s\n", basename((char *)file_name));
    size += snprintf(buffer + size, sizeof(buffer) - size, "Full Name: %s\n", file_name);
    size += snprintf(buffer + size, sizeof(buffer) - size, "Size: %ld bytes\n", st.st_size);

    size += snprintf(buffer + size, sizeof(buffer) - size, "Hard Links: %s\n", (st.st_nlink > 1) ? "Yes" : "No");
    size += snprintf(buffer + size, sizeof(buffer) - size, "Symbolic Links: %s\n", S_ISLNK(st.st_mode) ? "Yes" : "No");
    size += snprintf(buffer + size, sizeof(buffer) - size, "Last Modified: %s", ctime(&st.st_mtime));

    write(f, buffer, size);
    print_file_permissions(st, f);
    write(f, "\n\n", 2);
}

void traverse_directory(const char *dir_name, const int f) {
    DIR *dir = opendir(dir_name);
    struct dirent *entry;

    if (dir == NULL) {
        printf("Error opening directory!\n");
        exit(-1);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);

            if (entry->d_type == DT_REG) {
                file_info(path, f);
            } else if (entry->d_type == DT_DIR) {
                traverse_directory(path, f);
            }
        }
    }
    closedir(dir);
}

int is_directory(const char *dir_name) {
    struct stat st;
    if (stat(dir_name, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : 1;
    }
    return 1;
}

int snapshot_exists(const char *dir_name, const int t) {
    DIR *dir = opendir(dir_name);
    struct dirent *entry;

    if (dir == NULL) {
        printf("Error opening snapshot directory!\n");
        exit(-1);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".txt") != NULL) {
            char *found = entry->d_name;
            for (int i = 0; i < strlen(found); ++i) {
                if ((found[i] >= '0' && found[i] <= '9') && found[i] == t + '0') {
                    closedir(dir);
                    return 0;
                }
            }
        }
    }
    closedir(dir);
    return 1;
}

int is_duplicate(const int i, char **argv, int argc) {
    for (int j = 2; j < argc; ++j) {
        if (strcmp(argv[i], argv[j]) == 0 && i != j) {
            return 1;
        }
    }
    return 0;
}

void remove_snapshot(const char *path) {
    if (unlink(path) != 0) {
        printf("Error removing snapshot file!\n");
        exit(-1);
    }
}

int compare_snapshots(char *path1, char *path2) {
    int fd1 = open(path1, O_RDONLY);
    if (fd1 == -1) {
        printf("Error opening first snapshot for comparison!\n");
        exit(-1);
    }

    int fd2 = open(path2, O_RDONLY);
    if (fd2 == -1) {
        printf("Error opening second snapshot for comparison!\n");
        exit(-1);
    }

    char buffer1[4096], buffer2[4096];
    int size1, size2;

    do {
        size1 = read(fd1, buffer1, 4096);
        size2 = read(fd2, buffer2, 4096);

        if (size1 != size2 || memcmp(buffer1, buffer2, size1) != 0) {
            close(fd1);
            close(fd2);
            return 0;
        }
    } while (size1 > 0 && size2 > 0);

    if (size1 != size2) {
        close(fd1);
        close(fd2);
        return 0;
    }

    close(fd1);
    close(fd2);
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Insufficient arguments!\n");
        exit(-1);
    } else {
        if (argc > MAX_ARGS + 2) {
            printf("Too many arguments!\n");
            exit(-1);
        }
    }

    if (strcmp(argv[1], "-o") != 0) {
        printf("Incorrect syntax!\n");
        exit(-1);
    }

    char output_dir[256];
    int err = is_directory(argv[2]);
    if (err != 0) {
        printf("Output directory does not exist, creating...\n");
        if (mkdir(basename((char *)argv[2]), 0777) == -1) {
            printf("Error creating output directory!\n");
            exit(-1);
        }
    }
    snprintf(output_dir, sizeof(output_dir), "%s", argv[2]);

    int t[10] = {0};
    int f;
    for (int i = 3; i <= 12; ++i) {
        int err = is_directory(argv[i]);
        if (err == 0) {
            if (!is_duplicate(i, argv, argc)) {
                char path[270];
                snprintf(path, sizeof(path), "%s/snapshot%d.txt", output_dir, i - 3);
                t[i - 3] = 1;
                f = open(path, O_WRONLY | O_CREAT, 0744);
                close(f);

                struct stat file_stat;
                if (stat(path, &file_stat) == -1) {
                    printf("Unable to access write file!\n");
                    exit(-1);
                }

                if (file_stat.st_size == 0) {
                    int status, pid = fork();
                    if (pid == -1) {
                        printf("Error creating process!\n");
                    } else if (pid == 0) {
                        f = open(path, O_WRONLY, 0744);
                        traverse_directory(argv[i], f);
                        close(f);
                        exit(-1);
                    }

                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status)) {
                        printf("Process with PID %d exited with code %d!\n", pid, WEXITSTATUS(status));
                    } else {
                        printf("Invalid process for directory %s!\n", argv[i]);
                    }
                } else {
                    char path1[273];
                    snprintf(path1, sizeof(path1), "%s/snapshot%d_v1.txt", output_dir, i - 3);
                    f = open(path1, O_WRONLY | O_CREAT, 0744);

                    int status, pid = fork();
                    if (pid == -1) {
                        printf("Error creating process!\n");
                    } else if (pid == 0) {
                        f = open(path1, O_WRONLY, 0744);
                        traverse_directory(argv[i], f);
                        close(f);
                        exit(-1);
                    }

                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status)) {
                        printf("Process with PID %d exited with code %d!\n", pid, WEXITSTATUS(status));
                    } else {
                        printf("Invalid process for directory %s!\n", argv[i]);
                    }

                    if (!compare_snapshots(path, path1)) {
                        remove_snapshot(path);
                        if (rename(path1, path) != 0) {
                            printf("File could not be replaced!\n");
                            exit(-1);
                        }
                    } else {
                        remove_snapshot(path1);
                    }
                }
            }
        }
    }

    for (int i = 0; i < 10; ++i) {
        if (t[i] == 0 && !snapshot_exists(output_dir, i)) {
            char path[270];
            snprintf(path, sizeof(path), "%s/snapshot%d.txt", output_dir, i);
            remove_snapshot(path);
        }
    }

    return 0;
}