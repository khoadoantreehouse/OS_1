#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

void process_selected_file(char *file_name)
{
    char dir_name[100];
    int random_number = rand() % 100000;
    sprintf(dir_name, "doankh.movies.%d", random_number);
    if (mkdir(dir_name, S_IRWXU | S_IRGRP | S_IXGRP) != 0)
    {
        perror("Error creating directory");
        return;
    }
    printf("Directory created: %s\n", dir_name);
    // Additional processing of the file can go here.
}

int compare_file_sizes(const struct dirent **a, const struct dirent **b)
{
    struct stat file1, file2;
    stat((*a)->d_name, &file1);
    stat((*b)->d_name, &file2);
    return (int)(file2.st_size - file1.st_size);
}

int is_csv(char *file_name)
{
    int len = strlen(file_name);
    if (len < 4)
    {
        return 0;
    }
    return (strcmp(file_name + len - 4, ".csv") == 0);
}

void process_file(char *file_name)
{
    printf("Processing file: %s\n", file_name);
    // DONE: processing code goes here
    // TO DO: testing on centOS for proper file handling
    process_selected_file(file_name);
}

void find_file(int choice)
{
    struct dirent **files;
    int count, i;

    count = scandir(".", &files, 0, (choice == 1) ? compare_file_sizes : alphasort);

    for (i = 0; i < count; i++)
    {
        if (strstr(files[i]->d_name, ".csv") && strstr(files[i]->d_name, "movies_"))
        {
            process_file(files[i]->d_name);
            break;
        }
    }

    for (i = 0; i < count; i++)
    {
        free(files[i]);
    }
    free(files);
}

void select_file()
{
    int choice;

    printf("Which file you want to process?\n");
    printf("Enter 1 to pick the largest file\n");
    printf("Enter 2 to pick the smallest file\n");
    printf("Enter 3 to specify the name of a file\n");
    printf("Enter a choice from 1 to 3: ");
    scanf("%d", &choice);

    switch (choice)
    {
    case 1:
        find_file(1);
        break;
    case 2:
        find_file(2);
        break;
    case 3:
    {
        char file_name[100];
        printf("Enter the name of the file: ");
        scanf("%s", file_name);
        if (fopen(file_name, "r") == NULL)
        {
            printf("Error: file not found.\n");
            select_file();
        }
        else if (!is_csv(file_name))
        {
            printf("Error: not a .csv file.\n");
            select_file();
        }
        else
        {
            process_file(file_name);
        }
        break;
    }
    default:
        printf("Invalid choice\n");
        select_file();
    }
}

int main()
{
    int choice;
    while (1)
    {
        printf("1. Select file to process\n");
        printf("2. Exit the program\n");
        printf("Enter a choice 1 or 2: ");
        scanf("%d", &choice);

        if (choice == 1)
        {
            // DONE: file selected successfully
            // TO DO: check if return to main menu is working
            select_file();
        }
        else if (choice == 2)
        {
            break;
        }
        else
        {
            printf("Invalid choice.\n");
        }
    }

    return 0;
}
