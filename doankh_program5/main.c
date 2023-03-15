#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define BUFFER_SIZE 50
#define MAX_LINE_LENGTH 1001
#define OUTPUT_LINE_LENGTH 80
#define OUTPUT_LINES_PER_PAGE 20
#define STOP_COMMAND "STOP\n"

char buffer[BUFFER_SIZE][MAX_LINE_LENGTH];
int fill_ptr = 0;
int use_ptr = 0;
int count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;

void put(char *line)
{
    pthread_mutex_lock(&mutex);
    while (count == BUFFER_SIZE)
    {
        pthread_cond_wait(&empty, &mutex);
    }
    strcpy(buffer[fill_ptr], line);
    fill_ptr = (fill_ptr + 1) % BUFFER_SIZE;
    count++;
    pthread_cond_signal(&fill);
    pthread_mutex_unlock(&mutex);
}

char *get()
{
    pthread_mutex_lock(&mutex);
    while (count == 0)
    {
        pthread_cond_wait(&fill, &mutex);
    }
    char *line = strdup(buffer[use_ptr]);
    use_ptr = (use_ptr + 1) % BUFFER_SIZE;
    count--;
    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&mutex);
    return line;
}

void *input_thread(void *arg)
{
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, stdin))
    {
        put(line);
        if (strcmp(line, STOP_COMMAND) == 0)
        {
            break;
        }
    }
    pthread_exit(NULL);
}

void *line_separator_thread(void *arg)
{
    while (1)
    {
        char *line = get();
        if (strcmp(line, STOP_COMMAND) == 0)
        {
            free(line);
            put(STOP_COMMAND);
            break;
        }
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
        {
            line[len - 1] = ' ';
        }
        put(line);
        free(line);
    }
    pthread_exit(NULL);
}

void *plus_sign_thread(void *arg)
{
    while (1)
    {
        char *line = get();
        if (strcmp(line, STOP_COMMAND) == 0)
        {
            free(line);
            put(STOP_COMMAND);
            break;
        }
        int len = strlen(line);
        for (int i = 0; i < len - 1; i++)
        {
            if (line[i] == '+' && line[i + 1] == '+')
            {
                line[i] = '^';
                for (int j = i + 1; j < len - 1; j++)
                {
                    line[j] = line[j + 1];
                }
                len--;
            }
        }
        put(line);
        free(line);
    }
    pthread_exit(NULL);
}

void *output_thread(void *arg)
{
    int line_count = 0;
    int page_count = 0;
    while (1)
    {
        char *line = get();
        if (strcmp(line, STOP_COMMAND) == 0)
        {
            free(line);
            break;
        }
        int len = strlen(line);
        while (len > OUTPUT_LINE_LENGTH)
        {
            printf("%.80s\n", line);
            line += OUTPUT_LINE_LENGTH;
            len -= OUTPUT_LINE_LENGTH;
            line_count++;
            // if (line_count >= OUTPUT_LINES_PER_PAGE)
            // {
            //     printf("\f");
            //     page_count++;
            //     line_count = 0;
            // }
        }
        if (len > 0 && strcmp(line, " ") != 0)
        {
            printf("%s", line);
        }
        free(line);
    }
    // // Produce remaining lines if not a full page
    // if (line_count > 0)
    // {
    //     printf("\f");
    //     page_count++;
    // }

    pthread_exit(NULL);
}

int main()
{
    pthread_t input_tid, line_separator_tid, plus_sign_tid, output_tid;

    pthread_create(&input_tid, NULL, input_thread, NULL);
    pthread_create(&line_separator_tid, NULL, line_separator_thread, NULL);
    pthread_create(&plus_sign_tid, NULL, plus_sign_thread, NULL);
    pthread_create(&output_tid, NULL, output_thread, NULL);

    pthread_join(input_tid, NULL);
    pthread_join(line_separator_tid, NULL);
    pthread_join(plus_sign_tid, NULL);
    pthread_join(output_tid, NULL);

    return 0;
}
