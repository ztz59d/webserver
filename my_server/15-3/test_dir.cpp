#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    FILE *fp = fopen("./htdocs/targetfile.txt", "r");
    if (fp == NULL)
    {
        printf("/htdocs/targetfile.txt not found\n");
    }
    int i = 0;
    while (i < 1000)
    {
        char c = fgetc(fp);
        if (feof(fp))
        {
            break;
        }
        printf("%c", c);
        i++;
    }
    fclose(fp);
    return (0);
}