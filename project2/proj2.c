#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>

//soubor
FILE *file;

//inicializace semafori
sem_t *sem_mutex;
sem_t *sem_barrier_mutex;
sem_t *sem_oxygen;
sem_t *sem_hydrogen;
sem_t *sem_oxygen2;
sem_t *sem_hydrogen2;
sem_t *sem_print;
sem_t *sem_barrier;
sem_t *sem_barrier2;

//inicializace sdilenych promennych
int *count_for_bar;
int *count_all;
int *count_molecules;
int *count_processes;
int *count_hydrogens;
int *count_oxygens;
int *count_left;

//nacita sdilenou pamet a zjistuje, zda neni chyba u ftruncate
int *open_memory(const char *mem)
{
    int var1 = shm_open(mem, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
    int *var2 = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, var1, 0);
    //zjisti zda u ftruncate neni chyba
    if(ftruncate(var1, sizeof(int)) == -1)
    {
        perror("Chyba u ftruncate");
        return false;
    }
    //vrati mmap
    return var2;
}

//nacitani promennych a semafory
void load()
{
    sem_mutex = sem_open("/xkalaa00-sem_mutex", O_CREAT, 0666, 1);
    sem_oxygen = sem_open("/xkalaa00-sem_oxygen", O_CREAT, 0666, 0);
    sem_oxygen2 = sem_open("/xkalaa00-sem_oxygen2", O_CREAT, 0666, 1);
    sem_hydrogen = sem_open("/xkalaa00-sem_hydrogen", O_CREAT, 0666, 0);
    sem_hydrogen2 = sem_open("/xkalaa00-sem_hydrogen2", O_CREAT, 0666, 2);
    sem_print = sem_open("/xkalaa00-sem_print", O_CREAT, 0666, 1);
    sem_barrier = sem_open("/xkalaa00-sem_barrier", O_CREAT, 0666, 0);
    sem_barrier2 = sem_open("/xkalaa00-sem_barrier2", O_CREAT, 0666, 1);
    sem_barrier_mutex = sem_open("/xkalaa00-sem_barrier_mutex", O_CREAT, 0666, 1);

    count_for_bar = open_memory("/xkalaa00-count_for_bar");
    count_all = open_memory("/xkalaa00-count_all");
    count_left = open_memory("/xkalaa00-count_left");
    count_molecules = open_memory("/xkalaa00-count_molecules");
    count_processes = open_memory("/xkalaa00-count_processes");
    count_hydrogens = open_memory("/xkalaa00-count_hydrogens");
    count_oxygens = open_memory("/xkalaa00-count_oxygens");
}

//uklizeni promennych a semaforu
void clean()
{
    sem_unlink("/xkalaa00-sem_mutex");
    sem_unlink("/xkalaa00-sem_oxygen");
    sem_unlink("/xkalaa00-sem_hydrogen");
    sem_unlink("/xkalaa00-sem_oxygen2");
    sem_unlink("/xkalaa00-sem_hydrogen2");
    sem_unlink("/xkalaa00-sem_print");
    sem_unlink("/xkalaa00-sem_barrier");
    sem_unlink("/xkalaa00-sem_barrier2");
    sem_unlink("/xkalaa00-sem_barrier_mutex");
    
    munmap(count_for_bar, sizeof(int));
    munmap(count_all, sizeof(int));
    munmap(count_left, sizeof(int));
    munmap(count_molecules, sizeof(int));
    munmap(count_processes, sizeof(int));
    munmap(count_oxygens, sizeof(int));
    munmap(count_hydrogens, sizeof(int));

    shm_unlink("/xkalaa00-count_oxygens");
    shm_unlink("/xkalaa00-count_hydrogens");
    shm_unlink("/xkalaa00-count_all");
    shm_unlink("/xkalaa00-count_left");
    shm_unlink("/xkalaa00-count_processes");
    shm_unlink("/xkalaa00-count_for_bar");
    shm_unlink("/xkalaa00-count_molecules");

    munmap("/xkalaa00-count_oxygens", sizeof(int));
    munmap("/xkalaa00-count_hydrogens", sizeof(int));
    munmap("/xkalaa00-count_all", sizeof(int));
    munmap("/xkalaa00-count_left", sizeof(int));
    munmap("/xkalaa00-count_processes", sizeof(int));
    munmap("/xkalaa00-count_for_bar", sizeof(int));
    munmap("/xkalaa00-count_molecules", sizeof(int));
}

//print pro started
void print_started(char a, long id)
{
    sem_wait(sem_print);
    (*count_processes)++; //celkovy pocet procesu
        fprintf(file, "%d: %c %ld: started\n", *count_processes, a, id);
        fflush(file);
    sem_post(sem_print);
}

//print pro going to queue
void print_going(char a, long id)
{
    sem_wait(sem_print);    
    (*count_processes)++;
        fprintf(file, "%d: %c %ld: going to queue\n", *count_processes, a, id);
        fflush(file);
    sem_post(sem_print);
} 

//print pro not enough H
void print_notH(char a, long id)
{
    sem_wait(sem_print);
    (*count_processes)++;
        fprintf(file, "%d: %c %ld: not enough H\n", *count_processes, a, id);
        fflush(file);
    sem_post(sem_print);
}

//print pro not enough O or H
void print_notOH(char a, long id)
{
    sem_wait(sem_print);
    (*count_processes)++;
        fprintf(file, "%d: %c %ld: not enough O or H\n", *count_processes, a, id);
        fflush(file);
    sem_post(sem_print);
}

//print pro creating
void print_creating(char a, long id, long help)
{
    sem_wait(sem_print);
    (*count_processes)++;
        fprintf(file, "%d: %c %ld: creating molecule %ld\n", *count_processes, a, id, help);
        fflush(file);
    sem_post(sem_print);
}

//print pro created
void print_created(char a, long id, long help)
{
    sem_wait(sem_print);
    (*count_processes)++;
        fprintf(file, "%d: %c %ld: molecule %ld created\n", *count_processes, a, id, help);
        fflush(file);
    sem_post(sem_print);
}

//funkce na barieru
void barrier()
{
    sem_wait(sem_barrier_mutex);
    (*count_for_bar)++;

    if(*count_for_bar == 3)
    {
        sem_wait(sem_barrier2);
        sem_post(sem_barrier);
    }
    sem_post(sem_barrier_mutex);

    sem_wait(sem_barrier);
    sem_post(sem_barrier);

    sem_wait(sem_barrier_mutex);
    (*count_for_bar)--;

    if(*count_for_bar == 0)
    {
        sem_wait(sem_barrier);
        sem_post(sem_barrier2);
    }
    sem_post(sem_barrier_mutex);

    sem_wait(sem_barrier2);
    sem_post(sem_barrier2);
}

//oxygen funkce
void oxygen(int NH, long TI, long TB, long id)
{   
    char a = 'O'; 

    print_started(a, id);

    srand(getpid());

    usleep(rand() % (TI + 1) * 1000);
    
    print_going(a, id);

    sem_wait(sem_oxygen2);
    sem_wait(sem_mutex);

    (*count_oxygens)++;
    if(*count_hydrogens >= 2)
    {
        (*count_all)++;
        *count_left = *count_left + 2;

        sem_post(sem_hydrogen);
        sem_post(sem_hydrogen);

        *count_hydrogens = *count_hydrogens - 2;

        sem_post(sem_oxygen);

        (*count_oxygens)--;
    } else 
    {
        //pokud celkovy pocet zadanych vodiku je mensi nez pocet zbyvajicich vodiku 
        if(2 > NH - *count_left)
        {
        print_notH(a, id);

        sem_post(sem_mutex);
        sem_post(sem_oxygen2);
        return;
        }
        sem_post(sem_mutex);
    }

    sem_wait(sem_oxygen);
    int help = *count_all;

    print_creating(a, id, help);

    usleep(rand() % (TB + 1) * 1000);

    barrier();
    
    print_created(a, id, help);

    sem_post(sem_mutex);
    sem_post(sem_oxygen2);
    sem_post(sem_hydrogen2);
    sem_post(sem_hydrogen2);
}

//hydrogen funkce
void hydrogen(int NO, int NH, long TI, long id)
{   
    char a = 'H';

    print_started(a, id);

    srand(getpid());

    usleep(rand() % (TI + 1) * 1000);
    
    print_going(a, id);

    sem_wait(sem_hydrogen2);
    sem_wait(sem_mutex);
    
    (*count_hydrogens)++;
    if(*count_hydrogens >= 2 && *count_oxygens >= 1)
    {
        (*count_all)++;
        *count_left = *count_left + 2;

        sem_post(sem_hydrogen);
        sem_post(sem_hydrogen);

        *count_hydrogens = *count_hydrogens - 2;

        sem_post(sem_oxygen);

        (*count_oxygens)--;
    } else 
    {  
        //pokud rozdil vsech pozadovanych vodiku a vodiku ktere jsme spotrebovali je 1
        //nebo rozdil pozadovanych kysliku a kysliku ktere jsme spotrebovali je 0
        if((NH - *count_left == 1) || NO - *count_all == 0)
        {
        print_notOH(a, id);

        sem_post(sem_mutex);
        sem_post(sem_hydrogen2); 
        return;
        }
        sem_post(sem_mutex);
    }

    sem_wait(sem_hydrogen);
    int help = *count_all;
    
    print_creating(a, id, help);
    
    barrier();

    print_created(a, id, help);
}

int main(int argc, char *argv[])
{
    //otevre soubor, pokud neotevre, hodi error
    if((file = fopen("proj2.out", "w")) == NULL)
    {
        fprintf(stderr, "Soubor se nepodarilo otevrit.\n");
        clean();
        exit(1);
    }

    //pokud pocet argumentu neni 5, hodi error
    if (argc < 5 || argc > 5)
    {
        fprintf(stderr, "Spatne zadane argumenty.\n");
        clean();
        exit(1);
    }

    //pokud argument je prazdny, hodi error
    if((isdigit(argv[1][0]) == 0) || (isdigit(argv[2][0]) == 0) || (isdigit(argv[3][0]) == 0) || (isdigit(argv[4][0]) == 0))
    {
        fprintf(stderr, "Spatne zadane argumenty.\n");
        clean();
        exit(1);
    }

    //pokud se v argumentu objevuje neco jineho nez cislo, hodi error
    for(int i = 1; i < 5; i++)
    {
        for(int j = 0; argv[i][j] != '\0'; j++)
        {
            if((isdigit(argv[i][j]) == 0))
            {
                fprintf(stderr, "Argumenty mohou byt pouze kladna cisla.\n");
                clean();
                exit(1);
            }
        }
    }
    
    int NO = atoi(argv[1]);
    int NH = atoi(argv[2]);
    int TI = atoi(argv[3]);
    int TB = atoi(argv[4]);

    //pokud vodik nebo kyslik je mensi nebo rovno nule, hodi error
    if(NO <= 0 || NH <= 0)
    {
        fprintf(stderr, "Neni zadan vodik nebo kyslik.\n");
        clean();
        exit(1);
    }

    //pokud TI neni v rozmezi 0-1000, hodi error
    if (TI > 1000 || TI < 0)
    {
        fprintf(stderr, "TI musi byt v rozmezi 0-1000.\n");
        clean();
        exit(1);
    }

    //pokud TB neni v rozmezi 0-1000, hodi error
    if (TB > 1000 || TB < 0)
    {
        fprintf(stderr, "TB musi byt v rozmezi 0-1000.\n");
        clean();
        exit(1);
    }

    pid_t pid[NO + NH];
    int len_pid = 0;

    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    //nejprve vycisti pripadne semafory a memory a pote nacte
    clean();
    load();

    for (int i = 1; i <= NO; i++)
    {
        pid_t IDoxygen = fork();
        if(IDoxygen < 0)
        {
            perror("Chyba u fork");
            exit(1);
        }
        if(IDoxygen == 0)
        {
            oxygen(NH, TI, TB, i);
            exit(0);
        }
        else
        {
            pid[len_pid++] = IDoxygen;
        }
    }

    for (int i = 1; i <= NH; i++)
    {
        pid_t IDhydrogen = fork();
        if(IDhydrogen < 0)
        {
            perror("Chyba u fork");
            exit(1);
        }
        if(IDhydrogen == 0)
        {
            hydrogen(NO, NH, TI, i);
            exit(0);
        }
        else
        {
            pid[len_pid++] = IDhydrogen;
        }
    }

    for(int i = 0; i < len_pid; i++)
    {
        waitpid(pid[i], NULL, 0);
    }

    clean();

    fclose(file);
    return 0;
}