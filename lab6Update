#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<time.h>

#define lungime_drum 1000
#define lungime_snapshot 200

struct Metadata {
    char nume[lungime_drum];
    ino_t inode;
    off_t size;
    unsigned long ultima_modificare;
    unsigned long permisiuni;
};

void creare_snapshot(const char *drum, const struct Metadata *metadata)
{
    char snapshot[lungime_snapshot];
    sprintf(snapshot, "%s.snapshot", drum);

    int fd = open(snapshot, 2 | 0100 | 01000, 0664);
    if (fd == -1) {
        perror("Error fisier snapshot");
        exit(EXIT_FAILURE);
    }

    if (write(fd, metadata, sizeof(struct Metadata)) == -1) 
{
        perror("Error scriere in fisier snapshot");
        exit(EXIT_FAILURE);
}

    close(fd);
}

int compare_metadata(const struct Metadata *metadata1, const struct Metadata *metadata2) {
    if (metadata1->inode != metadata2->inode ||
        metadata1->size != metadata2->size ||
        metadata1->ultima_modificare != metadata2->ultima_modificare ||
        metadata1->permisiuni != metadata2->permisiuni) {
        return 0; 
    }

    return 1; 
}

void update_snapshot(const char *drum, const struct Metadata *new_metadata) {
    char snapshot[lungime_snapshot];
    sprintf(snapshot, "%s.snapshot", drum);

    struct Metadata old_metadata;
    int fd = open(snapshot, 0, 0);
    if (fd == -1) {
        perror("Error deschidere fisier snapshot pentru update");
        exit(EXIT_FAILURE);
    }

    if (read(fd, &old_metadata, sizeof(struct Metadata)) == -1) {
        perror("Error citire din fisier snapshot");
        exit(EXIT_FAILURE);
    }

    close(fd);

    if (!compare_metadata(&old_metadata, new_metadata)) {
        creare_snapshot(drum, new_metadata);
    }
}

int main(int argc, char ** argv)
{
    DIR* director = NULL;

    struct dirent* intrare = NULL;
    struct stat solutie;
    char drum[lungime_drum]; 

    if(argc != 2)
    {
        fprintf(stderr,"Usage: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if((director = opendir(argv[1])) == NULL)
    {
        fprintf(stderr,"ERROR opening directory!\n");
        exit(EXIT_FAILURE);
    }
    
    while((intrare = readdir(director))!= NULL)
    {
        if(strcmp(intrare->d_name,".") == 0 || strcmp(intrare->d_name,"..")==0){
            continue;
        }
        else
        {
            strcpy(drum, argv[1]);
            strcat(drum,"/");
            strcat(drum, intrare->d_name);

            if(lstat(drum, &solutie) == -1) {
                perror("Error getting file status");
                continue;
            }

            struct Metadata metadata;
            strncpy(metadata.nume, drum, lungime_drum);
            metadata.inode = solutie.st_ino;
            metadata.size = solutie.st_size;
            metadata.ultima_modificare = solutie.st_mtime;
            metadata.permisiuni= solutie.st_mode;

            update_snapshot(drum, &metadata);
        }
    }
    
    closedir(director);
    return 0;
} 
