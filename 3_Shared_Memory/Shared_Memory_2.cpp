#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <queue>

#define MAXID 100000
#define QUEUESIZE 1024

#define DIMN 1000
#define RMIN -9.0
#define RMAX 9.0

using namespace std;

void killchild() {
    kill(0, SIGTERM);
    int status;
    if (waitpid(-1, &status, WNOHANG) < 0)
        ;
}

typedef struct _job {
    pid_t producer_pid;
    int job_id;
    int producer_number;
    int status;
    int count;
    int mat_shm_id;
    double *mat;
    int n, m;
    int mat_id;
} Job;

typedef struct _shared_memory {
    Job jobs[QUEUESIZE];
    int front, rear;

    int job_created;
    int task_remaining;
    int maxtask;

    int temp_mat;

    sem_t mutex;
    sem_t full;
    sem_t empty;

} Shared;

/* ------------------------ Queue -------------------------- */

bool isempty(Shared *SHM) { return (SHM->front == -1 && SHM->rear == -1); }

bool isfull(Shared *SHM) { return ((SHM->rear + 1) % QUEUESIZE == SHM->front); }

int queueSize(Shared *SHM) {
    if (isempty(SHM))
        return 0;
    else if (isfull(SHM))
        return QUEUESIZE;
    else
        return (QUEUESIZE - SHM->front + SHM->rear + 1) % QUEUESIZE;
}
void EnQueue(Shared *SHM, Job data) {
    if ((SHM->rear + 1) % QUEUESIZE == SHM->front)
        cerr << "Queue is full " << endl;
    else {
        if (SHM->front == -1) SHM->front = 0;
        SHM->rear = (SHM->rear + 1) % QUEUESIZE;
        SHM->jobs[SHM->rear] = data;
    }
}
void DeQueue(Shared *SHM) {
    if (SHM->front == SHM->rear)
        SHM->front = SHM->rear = -1;
    else
        SHM->front = (SHM->front + 1) % QUEUESIZE;
}

Job &Front(Shared *SHM) { return SHM->jobs[SHM->front]; }

Job &Second(Shared *SHM) { return SHM->jobs[(SHM->front + 1) % QUEUESIZE]; }

void createQueue(int size, Shared *SHM) { SHM->front = SHM->rear = -1; }

/* ---------------------------------------------------------- */

void printMatrix(int mat_id, int r, int c) {
    double *mat = (double *)shmat(mat_id, NULL, 0);

    for (int i = 0; i < r; i++) {
        for (int j = 0; j < c; j++) {
            cout << mat[i * c + j] << " ";
        }
        cout << endl;
    }
}

double randDouble(double fMin, double fMax) {
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}
void print_job(Job job) {
    printf("Producer PID: %d\n", job.producer_pid);
    printf("Job ID: %d\n", job.job_id);
    printf("Producer Number: %d\n", job.producer_number);
    printf("Status: %d\n", job.status);
    printf("Matrix ID: %d\n", job.mat_id);

    fflush(stdout);
}

Job create_job(pid_t producer_pid, int producer_num) {
    Job job;
    job.job_id = rand() % MAXID + 1;
    job.m = DIMN;
    job.n = DIMN;

    while ((job.mat_shm_id = shmget(rand(), job.n * job.m * sizeof(double),
                                    0666 | IPC_CREAT)) < 0) {
    }

    job.mat = (double *)shmat(job.mat_shm_id, NULL, 0);

    for (int i = 0; i < job.n; i++) {
        for (int j = 0; j < job.m; j++) {
            job.mat[i * job.m + j] = randDouble(RMIN, RMAX);
        }
    }
    job.mat_id = rand() % MAXID + 1;
    job.producer_number = producer_num;
    job.producer_pid = producer_pid;
    job.status = 0;
    job.count = 0;

    shmdt(job.mat);
    return job;
}

void producer(int shmid, pid_t pid, int prod_num) {
    Shared *shmp = (Shared *)shmat(shmid, NULL, 0);
    while (shmp->job_created < shmp->maxtask) {
        Job job = create_job(pid, prod_num);
        sleep(rand() % 4);

        sem_wait(&(shmp->mutex));

        if (shmp->job_created == shmp->maxtask) {
            sem_post(&(shmp->mutex));
            break;
        }
        if (shmp->job_created < QUEUESIZE) {
            sem_wait(&(shmp->empty));
            EnQueue(shmp, job);
            shmp->job_created++;
            cout << "<------------------------------ Producer "
                    "---------------------------------->"
                 << endl;
            cout << "Job Created: " << shmp->job_created << "\n";
            print_job(job);
            printMatrix(job.mat_shm_id, DIMN, DIMN);
            cout << "<---------------------------- Producer End "
                    "-------------------------------->"
                 << endl;
        }

        sem_post(&(shmp->mutex));
    }
    shmdt(shmp);
}

double *multiply_matrices(int mat1_id, int mat2_id, int res_mat_id, int n) {
    double *mat1 = (double *)shmat(mat1_id, NULL, 0);
    double *mat2 = (double *)shmat(mat2_id, NULL, 0);
    double *res_mat = (double *)shmat(res_mat_id, NULL, 0);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                res_mat[i * n + j] += mat1[i * n + k] * mat2[k * n + j];
            }
        }
    }
    shmdt(mat1);
    shmdt(mat2);
    shmdt(res_mat);
    return res_mat;
}

int multiply_matrices1(Shared *shm, int mat1_id, int mat2_id, int res_mat_id,
                       int n, int &s1, int &s2) {
    double *mat1 = (double *)shmat(mat1_id, NULL, 0);
    double *mat2 = (double *)shmat(mat2_id, NULL, 0);
    double *res_mat = (double *)shmat(res_mat_id, NULL, 0);

    sem_wait(&(shm->mutex));

    int ret = 0;
    if (s1 == 0 && s2 == 0) {
        int len = n / 2;
        for (int i = 0; i < n / 2; ++i) {
            for (int j = 0; j < n / 2; ++j) {
                for (int k = 0; k < len; ++k) {
                    res_mat[i * n + j] += mat1[i * n + k] * mat2[k * n + j];
                }
            }
        }
        ret = 1;
        s1 = 0;
        s2 = 1;

    } else if (s1 == 0 && s2 == 1) {
        int len = n / 2;
        for (int i = 0; i < n / 2; ++i) {
            for (int j = n / 2; j < n; ++j) {
                for (int k = 0; k < len; ++k) {
                    res_mat[i * n + j] += mat1[i * n + k] * mat2[k * n + j];
                }
            }
        }
        ret = 1;
        s1 = 2;
        s2 = 0;

    } else if (s1 == 2 && s2 == 0) {
        int len = n / 2;
        for (int i = n / 2; i < n; ++i) {
            for (int j = 0; j < n / 2; ++j) {
                for (int k = 0; k < len; ++k) {
                    res_mat[i * n + j] += mat1[i * n + k] * mat2[k * n + j];
                }
            }
        }
        ret = 1;
        s1 = 2;
        s2 = 1;

    } else if (s1 == 2 && s2 == 1) {
        int len = n / 2;
        for (int i = n / 2; i < n; ++i) {
            for (int j = n / 2; j < n; ++j) {
                for (int k = 0; k < len; ++k) {
                    res_mat[i * n + j] += mat1[i * n + k] * mat2[k * n + j];
                }
            }
        }
        ret = 1;
        s1 = 1;
        s2 = 2;

    }
    // D0 Constructed

    else if (s1 == 1 && s2 == 2) {
        int len = n / 2;

        for (int i = 0; i < n / 2; ++i) {
            for (int j = 0; j < n / 2; ++j) {
                for (int k = 0; k < len; ++k) {
                    res_mat[i * n + j] +=
                        mat1[i * n + k + n / 2] * mat2[(k + n / 2) * n + j];
                }
            }
        }
        ret = 1;
        s1 = 1;
        s2 = 3;

    } else if (s1 == 1 && s2 == 3) {
        int len = n / 2;
        for (int i = 0; i < n / 2; ++i) {
            for (int j = n / 2; j < n; ++j) {
                for (int k = 0; k < len; ++k) {
                    res_mat[i * n + j] +=
                        mat1[i * n + k + n / 2] * mat2[(k + n / 2) * n + j];
                }
            }
        }
        ret = 1;
        s1 = 3;
        s2 = 2;

    } else if (s1 == 3 && s2 == 2) {
        int len = n / 2;
        for (int i = n / 2; i < n; ++i) {
            for (int j = 0; j < n / 2; ++j) {
                for (int k = 0; k < len; ++k) {
                    res_mat[i * n + j] +=
                        mat1[i * n + k + n / 2] * mat2[(k + n / 2) * n + j];
                }
            }
        }
        ret = 1;
        s1 = 3;
        s2 = 3;

    } else if (s1 == 3 && s2 == 3) {
        int len = n / 2;
        for (int i = n / 2; i < n; ++i) {
            for (int j = n / 2; j < n; ++j) {
                for (int k = len; k < n; ++k) {
                    res_mat[i * n + j] += mat1[i * n + k] * mat2[k * n + j];
                }
            }
        }
        ret = 1;
        s1 = 7;
        s2 = 7;
    }

    cout << "<------------------------------------ In multiplication "
            "------------------------------------------>"
         << endl;
    printMatrix(mat1_id, DIMN, DIMN);
    cout << endl;
    printMatrix(mat2_id, DIMN, DIMN);
    cout << endl;
    printMatrix(res_mat_id, DIMN, DIMN);

    cout << "<------------------------------------ End Multiplication "
            "----------------------------------------->"
         << endl;

    cout << "Status1: " << s1 << " "
         << "Status2 :" << s2 << endl
         << endl;
    sem_post(&(shm->mutex));

    // C0 Constructed
    // Res Matrix Constructed

    shmdt(mat1);
    shmdt(mat2);
    shmdt(res_mat);
    return ret;
}

void worker(int shmid, pid_t pid, int work_num) {
    Shared *shmw = (Shared *)shmat(shmid, NULL, 0);
    while (shmw->task_remaining > 0) {
        sleep(rand() % 4);

        cout << "Queue Size in Worker : " << queueSize(shmw) << " "
             << shmw->task_remaining << endl;

        sem_wait(&(shmw->mutex));
        bool job_retrieved = 0;

        if (queueSize(shmw) < 2) {
            sem_post(&(shmw->mutex));
            sleep(rand() % 2);
            continue;
        }
        Job &job1 = Front(shmw);
        Job &job2 = Second(shmw);
        if (job1.count >= 8 || job2.count >= 8) {
            sem_post(&(shmw->mutex));
            sleep(rand() % 2);
            continue;
        }
        cout << "<------------------------------------ Retrieved Job "
                "------------------------------------------>"
             << endl;
        cout << "Worker : " << work_num << endl;
        print_job(job1);
        printMatrix(job1.mat_shm_id, DIMN, DIMN);
        cout << endl;
        print_job(job2);
        printMatrix(job2.mat_shm_id, DIMN, DIMN);
        cout << "<------------------------------------ End Retrieved "
                "----------------------------------------->"
             << endl;

        job_retrieved = true;
        job1.count++;
        job2.count++;
        sem_post(&(shmw->mutex));

        // cout << "Queue Size: " << queueSize(shmw) << endl;
        // double *res_mat = multiply_matrices(job1.mat_shm_id, job2.mat_shm_id,
        //                                     j.mat_shm_id, job1.n);

        if (job_retrieved) {
            int ret = multiply_matrices1(shmw, job1.mat_shm_id, job2.mat_shm_id,
                                         shmw->temp_mat, job1.n, job1.status,
                                         job2.status);

            if (job1.status == 7 && job2.status == 7 && ret > 0) {
                sem_wait(&(shmw->mutex));

                job1.status = 15;
                job2.status = 15;

                Job j = create_job(pid, work_num);
                while ((j.mat_shm_id =
                            shmget(rand(), job1.n * job2.n * sizeof(double),
                                   0666 | IPC_CREAT)) < 0) {
                }
                double *mat = (double *)shmat(j.mat_shm_id, NULL, 0);
                double *res_mat = (double *)shmat(shmw->temp_mat, NULL, 0);

                for (int i = 0; i < j.n * j.n; i++) {
                    mat[i] = res_mat[i];
                    res_mat[i] = 0;
                }

                DeQueue(shmw);
                shmctl(job1.mat_shm_id, IPC_RMID, 0);
                sem_post(&(shmw->empty));
                DeQueue(shmw);
                shmctl(job2.mat_shm_id, IPC_RMID, 0);
                sem_post(&(shmw->empty));

                cout << "<------------------------------ Worker "
                        "---------------------------------->"
                     << endl;
                cout << "Matrix multiplication\n";

                printMatrix(j.mat_shm_id, j.n, j.n);

                j.mat = res_mat;
                shmw->task_remaining--;

                sem_wait(&(shmw->empty));
                while (queueSize(shmw) >= QUEUESIZE) {
                    sleep(rand() % 2);
                }
                EnQueue(shmw, j);

                cout << "Enqueue" << endl;
                cout << "Queue Size : " << queueSize(shmw) << endl;
                cout << "Worker : " << work_num << endl;
                print_job(j);
                printMatrix(j.mat_shm_id, DIMN, DIMN);

                cout << "Enqueue" << endl;

                cout << "Queue Size: " << queueSize(shmw) << endl;
                cout << "<----------------------------- Worker End "
                        "--------------------------------->"
                     << endl;

                sem_post(&(shmw->mutex));

                shmdt(mat);
                shmdt(res_mat);
            }
        }
    }
    shmdt(shmw);
}

Shared *init_shm(int shmid, int tasks) {
    Shared *shm = (Shared *)shmat(shmid, NULL, 0);

    int sema = sem_init(&(shm->mutex), 1, 1);
    // counting semaphore to check if the priority_queue is full
    int full_sema = sem_init(&(shm->full), 1, 0);
    // counting semaphore to check if the priority_queue is empty
    int empty_sema = sem_init(&(shm->empty), 1, QUEUESIZE);
    if (sema < 0 || full_sema < 0 || empty_sema < 0) {
        perror("Error in initializing semaphore. Exitting..");
        exit(1);
    }

    shm->job_created = 0;
    shm->front = -1;
    shm->rear = -1;
    shm->task_remaining = tasks - 1;
    shm->maxtask = tasks;

    while ((shm->temp_mat = shmget(rand(), DIMN * DIMN * sizeof(double),
                                   0666 | IPC_CREAT)) < 0) {
    }

    return shm;
}

int main() {
    // atexit(killchild);

    int np = 4, nw = 1, num = 2;
    cout << "Enter np, nw, tasks: " << endl;
    cin >> np >> nw >> num;
    int r = DIMN, c = DIMN;
    cout << getpid() << endl;
    time_t start = time(0);
    int SHM_id;
    while ((SHM_id = shmget(rand(), sizeof(Shared), 0666 | IPC_CREAT)) < 0) {
    }

    Shared *SHM = init_shm(SHM_id, num);
    createQueue(QUEUESIZE, SHM);

    pid_t producers[np], workers[nw];

    pid_t pp;
    for (int i = 0; i < np; i++) {
        if ((pp = fork()) < 0) {
            perror("Error in creating producer. ");
            exit(-1);
        } else if (pp == 0) {
            srand(time(0) + i);
            int producer_pid = getpid();

            producer(SHM_id, producer_pid, i);
            exit(0);
        } else {
            producers[i] = pp;
        }
    }
    sleep(rand() % 4);
    cout << "Producers created : " << queueSize(SHM) << " " << SHM->job_created
         << endl;
    pid_t pw;
    for (int i = 0; i < nw; i++) {
        pw = fork();
        if (pw < 0) {
            perror("Error in creating Worker. ");
            exit(-1);
        } else if (pw == 0) {
            srand(time(0) + i);
            int worker_pid = getpid();
            worker(SHM_id, worker_pid, i);
            exit(0);
        } else {
            workers[i] = pw;
        }
    }

    cout << "Workers Created : " << SHM->task_remaining << endl;
    sleep(rand() % 4);
    while (true) {
        if (SHM->job_created == num && SHM->task_remaining == 0) {
            for (int i = 0; i < np; i++) kill(producers[i], SIGTERM);
            for (int i = 0; i < nw; i++) kill(workers[i], SIGTERM);

            time_t end = time(0);
            int time_taken = end - start;
            cout << "Final Matrix : " << SHM->job_created << endl;
            Job j(Front(SHM));

            DeQueue(SHM);
            sem_post(&(SHM->empty));

            cout << "Time taken " << time_taken << endl;
            printMatrix(j.mat_shm_id, DIMN, DIMN);
            cout << endl;
            double sum = 0;
            j.mat = (double *)shmat(j.mat_shm_id, NULL, 0);
            for (int i = 0; i < j.n; i++) {
                sum += j.mat[i * j.n + i];
            }
            cout << "Sum of Elements in the diagonal : " << sum << endl;
            shmdt(j.mat);
            shmctl(j.mat_shm_id, IPC_RMID, 0);
            exit(0);
        }
    }

    shmdt(SHM);
    shmctl(SHM_id, IPC_RMID, 0);
    return 0;
}