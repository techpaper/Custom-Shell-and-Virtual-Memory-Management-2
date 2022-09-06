#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

typedef struct _process_data {
    double **A;
    double **B;
    double **C;
    int veclen, i, j;
} ProcessData;

void printMatrix(double **mat, int r, int c) {
    for (int i = 0; i < r; i++) {
        for (int j = 0; j < c; j++) {
            cout << mat[i][j] << " ";
        }
        cout << "\n";
    }
}

double randDouble(double fMin, double fMax) {
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

void *mult(ProcessData *data) {
    double d = 0;
    for (int i = 0; i < data->veclen; i++) {
        d += data->A[data->i][i] * data->B[i][data->j];
    }
    data->C[data->i][data->j] = d;
    return NULL;
}

int main(int, char **) {
    int r1, c1;
    cin >> r1 >> c1;
    int r2, c2;
    cin >> r2 >> c2;

    int r3 = r1, c3 = c2;
    if (c1 != r2) {
        fprintf(stderr, "Wrong Matrix\n");
        exit(0);
    }

    // key_t key = ftok("shmfile",65);
    int shmid_A = shmget(IPC_PRIVATE, r1 * sizeof(int *), 0666 | IPC_CREAT);
    if (shmid_A < 0) {
        perror("Error");
        exit(-1);
    }
    double **A = (double **)shmat(shmid_A, NULL, 0);
    for (int i = 0; i < r1; i++) {
        int shmid_col =
            shmget(IPC_PRIVATE, c1 * sizeof(double), 0666 | IPC_CREAT);
        A[i] = (double *)shmat(shmid_col, NULL, 0);
    }
    for (int i = 0; i < r1; i++) {
        for (int j = 0; j < c1; j++) {
            A[i][j] = randDouble(-10.0, 10.0);
        }
    }

    int shmid_B = shmget(IPC_PRIVATE, r2 * sizeof(double *), 0666 | IPC_CREAT);
    if (shmid_B < 0) {
        perror("Error");
        exit(-1);
    }
    double **B = (double **)shmat(shmid_B, NULL, 0);
    for (int i = 0; i < r2; i++) {
        int shmid_col =
            shmget(IPC_PRIVATE, c2 * sizeof(double), 0666 | IPC_CREAT);
        B[i] = (double *)shmat(shmid_col, NULL, 0);
    }
    for (int i = 0; i < r2; i++) {
        for (int j = 0; j < c2; j++) {
            B[i][j] = randDouble(-10.0, 10.0);
        }
    }

    int shmid_C = shmget(IPC_PRIVATE, r3 * sizeof(double *), 0666 | IPC_CREAT);
    if (shmid_C < 0) {
        perror("Error");
        exit(-1);
    }
    double **C = (double **)shmat(shmid_C, NULL, 0);
    for (int i = 0; i < r3; i++) {
        int shmid_col =
            shmget(IPC_PRIVATE, c3 * sizeof(double), 0666 | IPC_CREAT);
        C[i] = (double *)shmat(shmid_col, NULL, 0);
    }

    for (int i = 0; i < r1; i++) {
        for (int j = 0; j < c2; j++) {
            pid_t p;
            if ((p = fork()) == 0) {
                ProcessData data{A, B, C, c1, i, j};
                mult(&data);
                exit(0);
            }
        }
    }
    wait(0);

    printf("Matrix A:\n");
    printMatrix(A, r1, c1);

    printf("\n\n\nMatrix B:\n");
    printMatrix(B, r2, c2);

    printf("\n\n\nMatrix C:\n");
    printMatrix(C, r3, c3);

    shmdt(A);
    shmdt(B);
    shmdt(C);

    shmctl(shmid_A, IPC_RMID, NULL);
    shmctl(shmid_B, IPC_RMID, NULL);
    shmctl(shmid_C, IPC_RMID, NULL);
}
