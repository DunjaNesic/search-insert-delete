#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define LIST_CAPACITY 50
#define NUM_OF_SEARCHERS 1
#define NUM_OF_INSERTERS 3
#define NUM_OF_DELETERS 1

typedef struct node NODE;
typedef NODE *PNODE;

struct node
{
    int info;
    PNODE next;
};

int list_count = 0;

void insert_end(PNODE *head, int number)
{
    PNODE new = malloc(sizeof(PNODE));
    new->info = number;
    new->next = NULL;
    PNODE last;
    if (*head == NULL)
    {
        *head = new;
    }
    else
    {
        last = *head;
        while (last->next != NULL)
        {
            last = last->next;
        }

        last->next = new;
    }
}

int search(PNODE head, int number)
{
    while (head != NULL)
    {
        if (head->info == number)
        {
            return 1;
        }
        head = head->next;
    }

    return 0;
}

int delete (PNODE *head, int index)
{
    if ((*head) == NULL)
    {
        return 0;
    }

    if (index == 0)
    {
        *head = (*head)->next;
        return 1;
    }

    int n = 0;
    PNODE curr = *head;
    while (n != index - 1)
    {
        n++;
        curr = curr->next;
    }

    PNODE to_free = curr->next;
    curr->next = curr->next->next;
    free((void *)to_free);
    to_free = NULL;
    return 1;
}

typedef struct
{
    int counter;
    sem_t mutex;
} LIGHT_SWITCH;

LIGHT_SWITCH *ls_init()
{
    LIGHT_SWITCH *ls = (LIGHT_SWITCH *)malloc(sizeof(LIGHT_SWITCH));
    sem_init(&ls->mutex, 1, 1);
    ls->counter = 0;
    return ls;
}

void ls_lock(LIGHT_SWITCH *ls, sem_t *sem)
{
    sem_wait(&ls->mutex);
    ls->counter++;
    if (ls->counter == 1)
    {
        sem_wait(sem);
    }
    sem_post(&ls->mutex);
}

void ls_unlock(LIGHT_SWITCH *ls, sem_t *sem)
{
    sem_wait(&ls->mutex);
    ls->counter--;
    if (ls->counter == 0)
    {
        sem_post(sem);
    }
    sem_post(&ls->mutex);
}

void ls_free(LIGHT_SWITCH *ls)
{
    if (ls != NULL)
    {
        sem_destroy(&ls->mutex);
        free((void *)ls);
        ls = NULL;
    }
}

sem_t s_insert_mutex,
    s_no_searcher, s_no_inserter;
LIGHT_SWITCH *ls_search, *ls_insert;

void *searcher(void *arg)
{
    PNODE *linked_list = (PNODE *)arg;
    while (1)
    {
        ls_lock(ls_search, &s_no_searcher);

        if (list_count == 0)
        {
            printf("Searcher: List is empty!\n");
        }
        else
        {
            int value = rand() % 10;
            printf("Searcher: searching for %d in list!\n", value);

            if (search(*linked_list, value))
            {
                printf("Searcher: found %d in list!\n", value);
            }
        }

        ls_unlock(ls_search, &s_no_searcher);
        sleep(1);
    }

    pthread_exit((void *)0);
}

void *inserter(void *arg)
{
    PNODE *linked_list = (PNODE *)arg;

    while (1)
    {
        ls_lock(ls_insert, &s_no_inserter);
        sem_wait(&s_insert_mutex);

        if (list_count == LIST_CAPACITY)
        {
            printf("Inserter: List is full!\n");
        }
        else
        {
            int value = (rand() % 10);
            printf("Inserter: inserting %d into list, size of list = %d\n", value, list_count + 1);
            insert_end(linked_list, value);
            list_count++;
        }

        sem_post(&s_insert_mutex);
        ls_unlock(ls_insert, &s_no_inserter);

        sleep(1);
    }

    pthread_exit((void *)0);
}

void *deleter(void *arg)
{
    PNODE *linked_list = (PNODE *)arg;

    while (1)
    {
        sem_wait(&s_no_searcher);
        sem_wait(&s_no_inserter);

        if (list_count == 0)
        {
            printf("Deleter: List is empty!\n");
        }
        else
        {
            int index = rand() % list_count;
            printf("Deleter: Deleting %d. index from list!\n", index);
            if (delete (linked_list, index))
            {
                printf("Deleter: Successfully deleted %d. index from list!\n", index);
                list_count--;
            }
        }

        sem_post(&s_no_searcher);
        sem_post(&s_no_inserter);
        sleep(1);
    }

    pthread_exit((void *)0);
}

void search_insert_delete()
{
    PNODE linked_list = NULL;
    pthread_t t_searcher[NUM_OF_SEARCHERS], t_inserters[NUM_OF_INSERTERS], t_deleters[NUM_OF_DELETERS];

    sem_init(&s_insert_mutex, 1, 1);
    sem_init(&s_no_searcher, 1, 1);
    sem_init(&s_no_inserter, 1, 1);
    ls_search = ls_init();
    ls_insert = ls_init();

    for (int i = 0; i < NUM_OF_SEARCHERS; i++)
    {
        pthread_create(&t_searcher[i], NULL, searcher, &linked_list);
    }

    for (int i = 0; i < NUM_OF_INSERTERS; i++)
    {
        pthread_create(&t_inserters[i], NULL, inserter, &linked_list);
    }

    for (int i = 0; i < NUM_OF_DELETERS; i++)
    {
        pthread_create(&t_deleters[i], NULL, deleter, &linked_list);
    }

    for (int i = 0; i < NUM_OF_SEARCHERS; i++)
    {
        pthread_join(t_searcher[i], NULL);
    }

    for (int i = 0; i < NUM_OF_INSERTERS; i++)
    {
        pthread_join(t_inserters[i], NULL);
    }

    for (int i = 0; i < NUM_OF_DELETERS; i++)
    {
        pthread_join(t_deleters[i], NULL);
    }

    sem_destroy(&s_insert_mutex);
    sem_destroy(&s_no_searcher);
    sem_destroy(&s_no_inserter);
    ls_free(ls_search);
    ls_free(ls_insert);
    free((void *)linked_list);
    linked_list = NULL;
}

int main(int argc, char **argv)
{
    search_insert_delete();
    return 0;
}
