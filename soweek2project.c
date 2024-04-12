#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h> //am folosit-o pentru open(), close() si write();
#include <string.h>
#include <time.h> //este pentru ctime()
#include <libgen.h> // pentru basename()
#include <sys/types.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void printeaza_permisiuni_fisier(mode_t mode, int fd) 
{
    char permisiuni[11];
    permisiuni[0] = (S_ISDIR(mode)) ? 'd' : '-';
    permisiuni[1] = (mode & S_IRUSR) ? 'r' : '-';
    permisiuni[2] = (mode & S_IWUSR) ? 'w' : '-';
    permisiuni[3] = (mode & S_IXUSR) ? 'x' : '-';
    permisiuni[4] = (mode & S_IRGRP) ? 'r' : '-';
    permisiuni[5] = (mode & S_IWGRP) ? 'w' : '-';
    permisiuni[6] = (mode & S_IXGRP) ? 'x' : '-';
    permisiuni[7] = (mode & S_IROTH) ? 'r' : '-';
    permisiuni[8] = (mode & S_IWOTH) ? 'w' : '-';
    permisiuni[9] = (mode & S_IXOTH) ? 'x' : '-';
    permisiuni[10] = '\n';

    write(fd, permisiuni, sizeof(permisiuni));
}

void printeaza_metadate_fisier(const char *path, int fd) 
{
    struct stat st;
    if (lstat(path, &st) == -1) //aici verific daca pot obtine detalii despre fisier
    {
        dprintf(fd, "Nu se pot obtine informatii despre fisier: %s\n", path);
        return;
    }
    //aici scriu in fd (file descriptor-ul) informatiile despre fisier
    dprintf(fd, "Nume: %s\n", basename((char *)path));
    dprintf(fd, "Path complet: %s\n", path);
    dprintf(fd, "Dimensiune: %ld bytes\n", st.st_size);
    dprintf(fd, "Are hard links: %s\n", (st.st_nlink > 1) ? "Da" : "Nu"); //vad daca avem legaturi hardlink
    dprintf(fd, "Este symbolic link: %s\n", (S_ISLNK(st.st_mode)) ? "Da" : "Nu"); //daca are symbolic link
    dprintf(fd, "Ultima modificare: %s", ctime(&st.st_mtime));
    printeaza_permisiuni_fisier(st.st_mode, fd);
    write(fd, "\n\n", 2);
}

void traverseaza_director(const char *dir_path, int fd) 
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL) 
    {
        dprintf(fd, "Eroare la deschiderea directorului: %s\n", dir_path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
        {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

            struct stat st;
            if (stat(path, &st) == -1) //aici verific daca pot obtine informatii despre director
            {
                dprintf(fd, "Nu se pot obtine informatii despre director: %s\n", path);
                continue;
            }

            if (S_ISREG(st.st_mode)) 
            {
                printeaza_metadate_fisier(path, fd);
            } 
            else if (S_ISDIR(st.st_mode)) 
            {
                traverseaza_director(path, fd);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char **argv) 
{
    if (argc != 2) 
    {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (stat(argv[1], &st) == -1 || !S_ISDIR(st.st_mode)) 
    {
        fprintf(stderr, "%s NU este un director existent.\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    int fd = open("snapshot.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    //0644 = fisier obisnuit; 6 = 4 + 2, ne arata ca owner-ul are permisiuni de read si write; 4 = grupul are permisiuni de read-only; 4 = "others" au si ei permisiuni de read-only
    if (fd == -1) {
        perror("Nu s-a putut deschide snapshot-ul.txt");
        exit(EXIT_FAILURE);
    }

    traverseaza_director(argv[1], fd);
    close(fd);
    
    return 0;
}
