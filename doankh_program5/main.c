/* Citation: https://replit.com/@cs344/65prodconspipelinec */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LENGTH 1000
#define OUT_PUT_LENGTH 80
#define STOP_SIGNAL "STOP\n"

int num_lines = 0;

/* Buffer 1, shared resource between input thread and line separator thread*/
char *buffer_1[MAX_LENGTH];
/* Number of items in the buffer */
int count_1 = 0;
/* Index where the space delim thread will put the next item */
int prod_idx_1 = 0;
/* Index where the space delim thread will pick up the next item */
int con_idx_1 = 0;
/* Initialize the mutex for buffer 1 */
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
/* Initialize the condition variable for buffer 1 */
pthread_cond_t full_1 = PTHREAD_COND_INITIALIZER;

/* Buffer 2, shared resource between line separator thread and plus sign thread  */
char *buffer_2[MAX_LENGTH];
/* Number of items in the buffer */
int count_2 = 0;
/* Index where the space delim thread will put the next item */
int prod_idx_2 = 0;
/* Index where the space delim thread will pick up the next item */
int con_idx_2 = 0;
/* Initialize the mutex for buffer 2 */
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
/* Initialize the condition variable for buffer 2 */
pthread_cond_t full_2 = PTHREAD_COND_INITIALIZER;

/* Buffer 3, shared resource between plus sign thread and output thread */
char *buffer_3[MAX_LENGTH];
/* Number of items in the buffer */
int count_3 = 0;
/* Index where the replacement thread will put the next item */
int prod_idx_3 = 0;
/* Index where the replacement thread will pick up the next item */
int con_idx_3 = 0;
/* Initialize the mutex for buffer 3 */
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
/* Initialize the condition variable for buffer 3 */
pthread_cond_t full_3 = PTHREAD_COND_INITIALIZER;

/*
 Put an item in buff_1
*/
void put_buff_1(char *item)
{
    /* Lock the mutex before putting the item in the buffer */
    pthread_mutex_lock(&mutex_1);
    /* Put the item in the buffer */
    buffer_1[prod_idx_1] = item;
    /* Increment the index where the next item will be put. */
    prod_idx_1 = prod_idx_1 + 1;
    count_1++;
    /* Signal to the consumer that the buffer is no longer empty */
    pthread_cond_signal(&full_1);
    /* Unlock the mutex */
    pthread_mutex_unlock(&mutex_1);
}

/*
 Put an item in buff_2
*/
void put_buff_2(char *item)
{
    /* Lock the mutex before putting the item in the buffer */
    pthread_mutex_lock(&mutex_2);
    /* Put the item in the buffer */
    buffer_2[prod_idx_2] = item;
    /* Increment the index where the next item will be put. */
    prod_idx_2 = prod_idx_2 + 1;
    count_2++;
    /* Signal to the consumer that the buffer is no longer empty */
    pthread_cond_signal(&full_2);
    /* Unlock the mutex */
    pthread_mutex_unlock(&mutex_2);
}

/*
 Put an item in buff_3
*/
void put_buff_3(char *item)
{
    /* Lock the mutex before putting the item in the buffer */
    pthread_mutex_lock(&mutex_3);
    /* Put the item in the buffer */
    buffer_3[prod_idx_3] = item;
    /* Increment the index where the next item will be put. */
    prod_idx_3 = prod_idx_3 + 1;
    count_3++;
    /* Signal to the consumer that the buffer is no longer empty */
    pthread_cond_signal(&full_3);
    /* Unlock the mutex */
    pthread_mutex_unlock(&mutex_3);
}

/*
Get the next item from buffer 1
*/
char *get_buff_1()
{
    /* Lock the mutex before checking if the buffer has data */
    pthread_mutex_lock(&mutex_1);
    while (count_1 == 0)
        /* Buffer is empty. Wait for the producer to signal that the buffer has data */
        pthread_cond_wait(&full_1, &mutex_1);
    char *item = buffer_1[con_idx_1];
    /* Increment the index from which the item will be picked up */
    con_idx_1 = con_idx_1 + 1;
    count_1--;
    /* Unlock the mutex */
    pthread_mutex_unlock(&mutex_1);
    /* Return the item */
    return item;
}

/*
Get the next item from buffer 2
*/
char *get_buff_2()
{
    /* Lock the mutex before checking if the buffer has data */
    pthread_mutex_lock(&mutex_2);
    while (count_2 == 0)
        /* Buffer is empty. Wait for the producer to signal that the buffer has data */
        pthread_cond_wait(&full_2, &mutex_2);
    char *item = buffer_2[con_idx_2];
    /* Increment the index from which the item will be picked up */
    con_idx_2 = con_idx_2 + 1;
    count_2--;
    /* Unlock the mutex */
    pthread_mutex_unlock(&mutex_2);
    /* Return the item */
    return item;
}

/*
Get the next item from buffer 3
*/
char *get_buff_3()
{
    /* Lock the mutex before checking if the buffer has data */
    pthread_mutex_lock(&mutex_3);
    while (count_3 == 0)
        /* Buffer is empty. Wait for the producer to signal that the buffer has data */
        pthread_cond_wait(&full_3, &mutex_3);
    char *item = buffer_3[con_idx_3];
    /* Increment the index from which the item will be picked up */
    con_idx_3 = con_idx_3 + 1;
    count_3--;
    /* Unlock the mutex */
    pthread_mutex_unlock(&mutex_3);
    /* Return the item */
    return item;
}

void *input_thread(void *args)
{
    size_t line_length = MAX_LENGTH;
    char *item = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *input_line = (char *)malloc(MAX_LENGTH * sizeof(char));

    /* Get the input untill STOP line */
    while (strcmp(input_line, STOP_SIGNAL) != 0)
    {
        getline(&input_line, &line_length, stdin);
        num_lines++;
        if (strcmp(input_line, STOP_SIGNAL) != 0)
        {
            strcat(item, input_line);
        }
    }

    num_lines--;

    put_buff_1(item);

    return NULL;
}

void *line_separator_thread(void *args)
{
    // Get raw lines from buffer 1
    char *separated_item = get_buff_1();

    // Replace '\n' with single space ' ' in 1st buffer
    for (int i = 0; i < strlen(separated_item); i++)
    {
        if (separated_item[i] == '\n')
        {
            separated_item[i] = ' ';
        }
    }

    put_buff_2(separated_item);

    return NULL;
}

void *plus_sign_thread(void *args)
{
    // Get line break removed line from buffer 2
    char *item = get_buff_2();
    char *replaced_item = malloc(strlen(item) * sizeof(char));

    int i = 0, j = 0;

    while (item[i] != '\0')
    {
        if (item[i] == '+' && item[i + 1] == '+')
        {
            replaced_item[j] = '^';
            i += 2;
        }
        else
        {
            replaced_item[j] = item[i];
            i++;
        }
        j++;
    }

    put_buff_3(replaced_item);

    return NULL;
}

void *output_thread(void *args)
{
    // Get plus sign removed line from buffer 3
    char *item = get_buff_3();
    char *outputstring = malloc((OUT_PUT_LENGTH + 1) * sizeof(char)); // 80 characters + \n

    int counter = 0;
    int processed_index = 0;
    int num_chars = strlen(item);

    while ((num_chars - processed_index) >= OUT_PUT_LENGTH)
    {

        for (int i = 0; i < OUT_PUT_LENGTH; i++)
        {
            // Copy string in buffer from the processed_index until there is 80 character
            outputstring[i] = item[i + processed_index];
            counter++;
        }
        processed_index = counter;
        strcat(outputstring, "\n");
        write(STDOUT_FILENO, outputstring, OUT_PUT_LENGTH + 1);
    }

    return NULL;
}

int main()
{
    srand(time(0));
    pthread_t input_t, separated_t, plus_t, output_t;
    /* Create the threads */
    pthread_create(&input_t, NULL, input_thread, NULL);
    pthread_create(&separated_t, NULL, line_separator_thread, NULL);
    pthread_create(&plus_t, NULL, plus_sign_thread, NULL);
    pthread_create(&output_t, NULL, output_thread, NULL);

    /* Wait for the threads to terminate */
    pthread_join(input_t, NULL);
    pthread_join(separated_t, NULL);
    pthread_join(plus_t, NULL);
    pthread_join(output_t, NULL);
    return EXIT_SUCCESS;
}