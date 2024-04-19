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

int existaSnapshot(const char *director, const int nr)//daca numerotez snapshot-urile cu cifra "nr", verific daca si am un snapshot cu cifra "nr"
{
    DIR *d = opendir(director);
    struct dirent *intr;

    if(d == NULL)
    {
        printf("Error.\n");
        exit(-1);
    }

    while ((intr = readdir(d)) != NULL) 
    {
        if(strstr(intr->d_name, ".txt") != NULL)
        {
            char *valid = intr->d_name;
            for(int i = 0; i < strlen(valid); i++)
            {
                if((valid[i] >= '0' && valid[i] <= '9') && valid[i] == nr + '0')
                {
                    closedir(d);
                    return 0;
                }
            }
        }
    }
    closedir(d);
    return 1;
}

int verif_daca_este_director(const char *director)
{
    struct stat s;
    if (stat(director, &s) == 0) //aici verific daca avem un director sau nu.
    {
        if (S_ISDIR(s.st_mode)) 
            return 0;  //=>este director
        else
            return 1;  //=>NU este director
    } 
    return 1; 
}
int main(int argc, char **argv) 
{
    /*if (argc != 2) 
    {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        exit(-1);
    }*/
    //2 cazuri: 1)argumente prea putine daca argc < 2; 2)argumente prea multe daca arcg>10.
    if(argc < 2)
    {
        printf("Argumente insuficiente.\n");
        exit(-1);
    }

    else if(argc > 10)
    {
        printf("Argumentele depasesc numarul maxim.\n");
        exit(-1);
    }
    if(strcmp(argv[1], "-o") != 0)
    {
        printf("Structura gresita la sintaxa.\n");
        exit(-1);
    }


    struct stat st;
    if (stat(argv[1], &st) == -1 || !S_ISDIR(st.st_mode)) 
    {
        fprintf(stderr, "%s NU este un director existent.\n", argv[1]);
        exit(-1);
    }
    char director_output[1024];
    snprintf(director_output, sizeof(director_output), "%s", argv[2]);
    int table[10] = {0};//max 10 argumente
    int full_path, i;
    for(i = 2; i <= 12; i++)
    {
        int c = verif_daca_este_director(argv[i]);
        if(c == 0)
        {
            char drum[1024];
            snprintf(drum, sizeof(drum), "%s/SnApShoT%d.txt", director_output, i - 2);
            table[i - 2] = 1;
            full_path = open(drum, O_WRONLY | O_CREAT | O_TRUNC, 0644);//imi updatez fisierul de snapshot de fiecare data cand rulez
            traverseaza_director(argv[i], full_path);
            close(full_path);
        }
    }
    //daca schimbam ordinea argumentelor raman fisierele vechi -> acestea trebuie sterse
    for(int i = 0; i < 10; i++)
    {
        if(table[i] == 0 && existaSnapshot(director_output, i) == 0)
        {
            char drum[1000];
            snprintf(drum, sizeof(drum), "%s/snapshot%d.txt", director_output, i);
            if(unlink(drum) != 0)//sterg fisierul "drum"
            {
                printf("Nu se poate stereg snapshot.\n");
                exit(-1);
            }
        }
    }


    /*
    int fd = open("sNaPsHoT.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    //0644 = fisier obisnuit; 6 = 4 + 2, ne arata ca owner-ul are permisiuni de read si write; 4 = grupul are permisiuni de read-only; 4 = "others" au si ei permisiuni de read-only
    if (fd == -1) {
        perror("Nu s-a putut deschide snapshot-ul.txt");
        exit(-1);
    }
    

    traverseaza_director(argv[1], fd);
    close(fd);
    */
    
    return 0;
}
