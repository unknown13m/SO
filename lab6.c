#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<string.h>

int main(int argc, char ** argv)
{
    DIR* director = NULL;

    struct dirent* intrare = NULL;
    struct stat solutie;
    char drum[100]; 

    if(argc > 2)
    {
        fprintf(stderr,"EROARER\n");
        exit(-1);
    }

    if((director = opendir(argv[1])) == NULL)
    {
        fprintf(stderr,"ERROR dir!\n");
        exit(-1);
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

            printf("%s\n", drum);
            printf("%d\n", lstat(drum, &solutie));
        }
    }
    
    closedir(director);
    return 0;
}

