#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LANGUAGES_SIZE 5 // As specified in the assignment requirements

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

void print_movies(MovieList *movieList)
{
    // Test function to print all movies in the list
    Movie *current = movieList->head;
    while (current != NULL)
    {
        printf("Name: %s\n", current->name);
        printf("Year: %d\n", current->year);
        printf("Languages: ");

        for (int i = 0; i < current->number_of_languages; i++)
        {
            printf("%s", current->languages[i]);
            if (i < current->number_of_languages - 1)
            {
                printf(", ");
            }
        }
        printf("\nRating: %lf\n", current->rating);
        printf("\n");
        current = current->next;
    }
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
    char line[1024];
    fgets(line, 1024, file); // Skip the first line (header)

    while (fgets(line, 1024, file))
    {
        // Get the movie name, year, languages and rating
        char *name = strtok(line, ",");
        int year = atoi(strtok(NULL, ","));
        char *languages = strtok(NULL, ",");
        double rating = strtod(strtok(NULL, ","), NULL);

        // Add the movie to the linked list
        add_movie(movieList, name, year, languages, rating);
    }
    printf("Processed file %s and parsed data for %d movies\n", file_name, movieList->size);
    // Close the file
    fclose(file);
    return 0;
}

void show_movies_by_year(MovieList *movieList)
{
    int year;
    printf("Enter the release year: ");
    scanf("%d", &year);

    // Scan through all movies and compare the year against user input
    Movie *current = movieList->head;
    int found = 0;
    while (current != NULL)
    {
        if (current->year == year)
        {
            printf("%s\n", current->name);
            found = 1;
        }
        current = current->next;
    }

    if (!found)
    {
        printf("No movies found for the year %d\n", year);
    }
}

void show_highest_rated_movies(MovieList *movieList)
{
    // Create an array to store the highest rated movie for each year
    // The years range from 1900 to 2021 => array size is 2021 - 1900 + 1
    // This work as a simple hash table
    Movie *highest_rated_by_year[2021 - 1900 + 1];
    memset(highest_rated_by_year, 0, sizeof(highest_rated_by_year));

    // Iterate through the linked list of movies
    Movie *current = movieList->head;
    while (current != NULL)
    {
        // If the current movie has a higher rating than the current highest rated movie for that year, update the highest rated movie for that year
        if (highest_rated_by_year[current->year - 1900] == NULL || current->rating > highest_rated_by_year[current->year - 1900]->rating)
        {
            highest_rated_by_year[current->year - 1900] = current;
        }
        current = current->next;
    }

    // Print the highest rated movie for each year
    for (int i = 1900; i <= 2021; i++)
    {
        if (highest_rated_by_year[i - 1900] != NULL)
        {
            printf("%d %.1f %s\n", i, highest_rated_by_year[i - 1900]->rating, highest_rated_by_year[i - 1900]->name);
        }
    }
}

int main(int argc, char *argv[])
{
    // if there is no csv file specified
    if (argc < 2)
    {
        printf("You must provide the name of the file to process\n");
        printf("Example usage: ./movies movies_sample_1.csv\n");
        return EXIT_FAILURE;
    }

    // Initialize movie list
    MovieList movieList = {NULL, NULL, 0};
    // Read CSV file
    if (read_csv(argv[1], &movieList))
    {
        printf("Error reading CSV file\n");
        return 1;
    }

    // do something with the movies data
    // TEST: menu
    int choice;
    do
    {
        printf("Menu:\n");
        printf("1. Show movies released in the specified year\n");
        printf("2. Show highest rated movie for each year\n");
        printf("3. Show the title and year of release of all movies in a specific language\n");
        printf("4. Exit from the program\n");
        printf("Enter a choice from 1 to 4: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            // TODO:
            show_movies_by_year(&movieList);
            break;
        case 2:
            // TODO:
            show_highest_rated_movies(&movieList);
            break;
        case 3:
            // TODO: show_movies_by_language(&movieList);
            break;
        case 4:
            printf("Exiting program...\n");
            break;
        default:
            printf("Invalid choice.\n");
            break;
        }
    } while (choice != 4);
    // Print the movies
    // TEST: print_movies(&movieList);
    // Free the memory
    free_movies(&movieList);
    return EXIT_SUCCESS;
}