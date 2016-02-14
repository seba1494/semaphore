#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>


enum
{ EMPTY, MUTEX, FULL };

int m;
int k;

const int BUFFER_SIZE = 10;
const int CZAS_PRODUCENTA = 1;
const int CZAS_KONSUMENTA = 1;
const key_t KEY = 2;

static struct sembuf buf;

void sem_up(int , int );
void sem_down(int , int );
void konsumuj(int);
void produkuj(int);
void tworz_konsumenta(int);




int main()
{
  int i, j, p;
  int shmid, semid;
  char *buffer[m];

  printf("\n");
  printf("Wprowadz liczbe buforow = ");
  scanf("%d", &m);
  printf("Wprowadz liczbe konsumentow = ");
  scanf("%d", &k);
  printf("Wprowadz liczbe procesów = ");
  scanf("%d", &p);

  /* Tworzenie tablicy zestawu semaforow  */
  for(i = 0; i < m; ++i)
  {
    semid = semget(KEY + i, 3, IPC_CREAT|0600);
    if (semid == -1)
    {
      perror("Utworzenie tablicy semaforow");
      exit(1);
    }



	/* Inicjalizacja semaforow  */
    if (semctl(semid, EMPTY, SETVAL, BUFFER_SIZE) == -1)
    {
      perror("Nadanie wartosci semaforowi 0");
		  exit(1);
    }


    if (semctl(semid, MUTEX, SETVAL, 1) == -1)
    {
      perror("Nadanie wartosci semaforowi 1");
      exit(1);
    }


    if (semctl(semid, FULL, SETVAL, 0) == -1)
    {
      perror("Nadanie wartosci semaforowi 1");
      exit(1);
    }

    if ((shmid = shmget(KEY + i, BUFFER_SIZE, IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    if ((buffer[i] = shmat(shmid, NULL, 0)) == (char *) -1)
       {
           perror("shmat");
           exit(1);
       }

  }



  printf("Zaczynam produkowac\n");
  if(fork() == 0)
  {
    produkuj(p);
    exit(0);
  }
//sleep(10);
  printf("##########Zaczynam konsmowac\n");
  tworz_konsumenta(k);

  return 0;
}





/*****************************************************/
void sem_up(int semid, int semnum)
  {
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1)
    {
     perror("Podnoszenie semafora");
     exit(1);
    }
  }

  void sem_down(int semid, int semnum)
  {
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1)
    {
      perror("Opuszczenie semafora");
      exit(1);
    }
  }

void produkuj(int tasks)
{
  int i, j = 0, z;
  int shmid, semid, nr_bufora, sem_val;
  char *buffer[m];
  srand (time(NULL));

  for(i = 0; i < m; ++i)
  {
     if ((shmid = shmget(KEY + i, BUFFER_SIZE, 0666)) < 0)
     {
         perror("shmget");
         exit(1);
     }

    if ((buffer[i] = shmat(shmid, NULL, 0)) == (char *) -1)
    {
        perror("shmat");
        exit(1);
    }
  }


  for(i = 0; i < m; ++i)
  {
    for(j = 0; j < BUFFER_SIZE; ++j)
    {
      buffer[i][j] = 'a';
    }
  }

  printf("\nk=%d tasks=%d\n",k,tasks);
  while(k < tasks)
  {
    nr_bufora = rand() % m;
    semid = semget(KEY + nr_bufora, 3, IPC_EXCL|0600);

    sem_val = semctl(semid, FULL, GETVAL, 0);

      if(BUFFER_SIZE - sem_val >= 1)
      {


        sem_down(semid, MUTEX);

        sem_val = semctl(semid, FULL, GETVAL, 0);

        if(BUFFER_SIZE - sem_val >= 1)
        {
	         sem_down(semid, EMPTY);
           sem_up(semid, FULL);
           printf("semval=%d\n", sem_val);
          buffer[nr_bufora][sem_val]= 'X';


          printf("Po wyprodukowaniu X dla bufora NR:%d :\n",nr_bufora);
          for(z = 0; z < m; ++z)
          {
            printf("%d: ", z);
            for(j = 0; j < BUFFER_SIZE; ++j)
            {
              printf("%c", buffer[z][j]);
            }
            printf("\n");
          }
        }
        sem_up(semid, MUTEX);
      }

    sleep(CZAS_PRODUCENTA);
    ++k;
  }
  exit(0);
}

void tworz_konsumenta(int k)
{
  int l;
  for(l = 0; l < k; ++l)
  {
    if(fork() == 0)
    {
      konsumuj(l);
      exit(0);
    }
    sleep(1);
  }
}

void konsumuj(int n)
{
  char *buffer[m];
  int i, j, x, q, r;
  int semid, shmid, sem_val;
  int  usuniete = 0;

  for(x = m-1; x >=0 ; --x)
  {
     if ((shmid = shmget(KEY + x, BUFFER_SIZE, 0666)) < 0)
     {
         perror("shmget");
         exit(1);
     }

     if ((buffer[x] = shmat(shmid, NULL, 0)) == (char *) -1)
     {
        perror("shmat");
        exit(1);
    }
  }


  while(usuniete != 1)
  {

      for(q = m-1; q >= 0 && usuniete != 1; q--)
      {
        semid = semget(KEY + q, 3, IPC_EXCL|0600);
        sem_down(semid, MUTEX);


        sem_val = semctl(semid, FULL, GETVAL, 0) ;
        if(sem_val >= 1)
        {
          if(buffer[q][sem_val-1] == 'X')
          {
            buffer[q][sem_val-1] = 'b';

            usuniete = 1;

            printf("Po usunieciu X:\n");
            for(r = 0; r < m; ++r)
            {
              printf("%d: ", r);
              for(j = 0; j < BUFFER_SIZE; ++j)
              {
                printf("%c", buffer[r][j]);
              }
              printf("\n");
            }
            printf("Konsument o numerze %d zakonczył dzialanie\n", n);


              sem_down(semid, FULL);
              sem_up(semid, EMPTY);
              sem_up(semid,MUTEX);
              exit(0);

          }
	       }
        sem_up(semid, MUTEX);

      }
      sleep(CZAS_KONSUMENTA);
  }
  printf("wychodze\n");
  exit(0);
}
