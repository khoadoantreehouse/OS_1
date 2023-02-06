#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DIR_PERMISSIONS (S_IRWXU | S_IRGRP | S_IXGRP)
#define FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP)

#define MAX_LANGUAGES_SIZE 5 // As specified in the assignment requirements
#define MAX_LANGUAGE_SIZE_USER_INPUT 25
#define YOUR_ONID "doankh"

typedef struct Movie
{
    char *name;
    int year;
    char **languages;
    int number_of_languages;
    double rating;
    struct Movie *next;
} Movie;

typedef struct MovieList
{
    Movie *head;
    Movie *tail;
    int size;
} MovieList;

int split_languages(char *languages, char **languages_array)
{
    // check if the string have a correct format
    if (languages[0] != '[' || languages[strlen(languages) - 1] != ']')
    {
        return 0;
    }

    // remove brackets
    languages++;                             // front slash
    languages[strlen(languages) - 1] = '\0'; // back slash

    int number_of_languages = 0;
    char *token = NULL;
    char *saveptr = NULL;

    // Split at ";", allocate new memory block, add to block
    while ((token = strtok_r(languages, ";", &saveptr)) != NULL)
    {
        int length = strlen(token);
        languages_array[number_of_languages] = malloc((length + 1) * sizeof(char));
        strcpy(languages_array[number_of_languages], token);
        languages = NULL;
        number_of_languages += 1;
    }

    return number_of_languages;
}

void free_movies(MovieList *movieList)
{
    // Iterate through all instance and free the memory
    Movie *current = movieList->head;
    while (current != NULL)
    {
        for (int i = 0; i < current->number_of_languages; i++)
        {
            free(current->languages[i]);
        }
        free(current->languages);
        free(current->name);
        Movie *next = (Movie *)current->next;
        free(current);
        current = next;
    }
    movieList->head = NULL;
    movieList->tail = NULL;
    movieList->size = 0;
}

void add_movie(MovieList *movieList, char *name, int year, char *languages, double rating)
{
    // Create a new movie
    Movie *new_movie = (Movie *)malloc(sizeof(Movie));
    new_movie->name = (char *)malloc((strlen(name) + 1) * sizeof(char));
    new_movie->languages = (char **)malloc(MAX_LANGUAGES_SIZE * sizeof(char *));
    strcpy(new_movie->name, name);
    new_movie->year = year;
    new_movie->rating = rating;
    new_movie->next = NULL;

    // Split languages -> number of languages is returned and stored
    int i = 0;
    i = split_languages(languages, new_movie->languages);
    new_movie->number_of_languages = i;

    // Add the movie to the linked list
    if (movieList->head == NULL)
    {
        movieList->head = new_movie;
        movieList->tail = new_movie;
    }
    else
    {
        movieList->tail->next = new_movie;
        movieList->tail = new_movie;
    }
    movieList->size++;
}
int read_csv(char *file_name, MovieList *movieList)
{
    // Open the file
    FILE *file = fopen(file_name, "r");
    if (file == NULL)
    {
        return 1;
    }

    // Read the file line by line
    char *currLine = NULL;
    size_t len = 0;
    ssize_t nread;
    getline(&currLine, &len, file); // Skip the first line (header)

    while ((nread = getline(&currLine, &len, file)) != -1)
    {
        // Get the movie name, year, languages and rating
        char *name = strtok(currLine, ",");
        int year = atoi(strtok(NULL, ","));
        char *languages = strtok(NULL, ",");
        double rating = strtod(strtok(NULL, ","), NULL);

        // Add the movie to the linked list
        add_movie(movieList, name, year, languages, rating);
    }

    printf("Processed file %s and parsed data for %d movies\n", file_name, movieList->size);
    printf("\n");

    // Close the file
    free(currLine); // Free the getLine memory block
    fclose(file);
    return 0;
}

void process_selected_file(char *filename)
{
    char directory_name[100];
    sprintf(directory_name, "%s.movies.%d", YOUR_ONID, rand() % 100000);
    int status = mkdir(directory_name, DIR_PERMISSIONS);

    if (status == 0)
    {
        printf("Directory %s created\n", directory_name);
    }
    else
    {
        printf("Unable to create directory %s\n", directory_name);
        return;
    }

    // Initialize a new movie list to parse the file contents
    MovieList movieList = {NULL, NULL, 0};

    read_csv(filename, &movieList);

    // Open the directory
    DIR *dir = opendir(directory_name);
    if (dir == NULL)
    {
        printf("Error opening the directory\n");
        return;
    }

    // For each movie in the movie list, create a new year.txt file and write the movies release that year to the file
    Movie *current = movieList.head;
    while (current != NULL)
    {
        char year_file_path[100];
        sprintf(year_file_path, "%s/%d.txt", directory_name, current->year);
        int year_file = open(year_file_path, O_WRONLY | O_APPEND | O_CREAT, FILE_PERMISSIONS); // Also, allow appending if file exists
        if (year_file == -1)
        {
            printf("Error opening the year file\n");
            return;
        }

        // Write the title of the movie to the year file
        write(year_file, current->name, strlen(current->name));
        write(year_file, "\n", 1);

        // Close the year file
        close(year_file);

        current = current->next;
    }

    // Close the directory
    closedir(dir);

    // Free the memory used by the linked list
    free_movies(&movieList);
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
