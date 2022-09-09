#include <bits/stdc++.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXTIME 250000
#define MAXRAND 100000000

#define ROOT_INDEX 0

using namespace std;

enum status { NOT_STARTED, ON_GOING, DONE };

struct Node {
    int job_id;
    int job_time;
    int dependent_jobs[15000];
    int next_job;
    int active_childs;

    int par;
    int ind;

    sem_t lock;
    status process_status;

    Node();
    void print_job();
    ~Node();
};
Node::Node() {}
void Node::print_job() {
    cout << "<--------------------------------\n";
    cout << "Job ID     : " << job_id << endl;
    cout << "Job Index  : " << ind << endl;
    cout << "Job Parent : " << par << endl;
    cout << "Job Time   : " << job_time << endl;
    cout << "Job Status : " << process_status << endl;
    cout << "-------------------------------->\n" << endl;
}

Node::~Node() {}

struct Shared {
    Node jobs[15000];

    int next;

    int jobs_created;
    int jobs_remaining;
    int jobs_still_to_add;

    int edges;

    sem_t mutex;
};

Shared *init_shm(int shmid) {
    Shared *shm = (Shared *)shmat(shmid, NULL, 0);

    memset(shm, 0, sizeof(Shared));

    int sema = sem_init(&(shm->mutex), 1, 1);

    int initial_jobs = rand() % 201 + 300, jobs_to_add_later = 14500;
    // cout << "initial jobs : ";
    // cin >> initial_jobs;
    // cout << "jobs to add later : ";
    // cin >> jobs_to_add_later;
    shm->jobs_created = 0;
    shm->jobs_remaining = 0;
    shm->jobs_still_to_add = jobs_to_add_later;

    for (shm->next = 0; (shm->next) < initial_jobs; shm->next++) {
        shm->jobs[shm->next].job_id = rand() % MAXRAND + 1;
        shm->jobs[shm->next].job_time = rand() % MAXTIME + 1;
        int sema = sem_init(&(shm->jobs[shm->next].lock), 1, 1);
        shm->jobs[shm->next].process_status = NOT_STARTED;
        shm->jobs[shm->next].next_job = 0;
        shm->jobs[shm->next].active_childs = 0;

        shm->jobs[shm->next].ind = shm->next;
        shm->jobs[shm->next].par = -1;

        shm->jobs_remaining++;

        // Add Edge
        if (shm->next) {
            int p = rand() % shm->next;
            shm->jobs[p].dependent_jobs[shm->jobs[p].next_job] = shm->next;
            shm->jobs[p].next_job++;
            shm->jobs[p].active_childs++;

            shm->jobs[shm->next].par = p;
        }

        cout << "Job added " << endl;
        shm->jobs[shm->next].print_job();
        usleep(rand() % 301000 + 200000);
    }

    return shm;
}

void print_tree_inorder(int root, Shared *shm) {
    cout << root << " ";
    for (int i = 0; i < shm->jobs[root].next_job; i++) {
        print_tree_inorder(shm->jobs[root].dependent_jobs[i], shm);
    }
}

void *produce_jobs(void *param) {
    Shared *shm = static_cast<Shared *>(param);

    time_t maxtime = rand() % 11 + 10;
    time_t start = time(0);

    while (time(0) - start < maxtime) {
        int ind = rand() % (shm->next);
        if (shm->jobs_still_to_add) {
            sem_wait(&(shm->jobs[ind].lock));
            if (shm->jobs[ind].process_status == NOT_STARTED) {
                sem_wait(&(shm->mutex));

                if (!shm->jobs_still_to_add) {
                    sem_post(&(shm->mutex));
                    sem_post(&(shm->jobs[ind].lock));
                    continue;
                }

                // setting up the new job
                shm->jobs[shm->next].job_id = rand() % MAXRAND + 1;
                shm->jobs[shm->next].job_time = rand() % MAXTIME + 1;
                int sema = sem_init(&(shm->jobs[shm->next].lock), 1, 1);
                shm->jobs[shm->next].process_status = NOT_STARTED;
                shm->jobs[shm->next].next_job = 0;
                shm->jobs[shm->next].active_childs = 0;

                shm->jobs[shm->next].par = ind;
                shm->jobs[shm->next].ind = shm->next;

                // updating the parent job
                shm->jobs[ind].dependent_jobs[shm->jobs[ind].next_job] =
                    shm->next;
                shm->jobs[ind].next_job++;
                shm->jobs[ind].active_childs++;

                cout << "Job added by threads " << endl;
                shm->jobs[shm->next].print_job();

                // updating the informations in shared memory
                shm->next++;
                shm->jobs_created++;
                shm->jobs_remaining++;
                shm->jobs_still_to_add--;

                sem_post(&(shm->mutex));

                sem_post(&(shm->jobs[ind].lock));

                usleep(rand() % 301000 + 200000);

            } else {
                sem_post(&(shm->jobs[ind].lock));
            }
        }
    }
    pthread_exit(0);
    return NULL;
}
int first_free_job(int root, Shared *shm) {
    if (shm->jobs[root].active_childs == 0 &&
        shm->jobs[root].process_status == NOT_STARTED) {
        return root;
    }
    int free_job = -1;
    for (int i = 0; i < shm->jobs[root].next_job; i++) {
        int t = first_free_job(shm->jobs[root].dependent_jobs[i], shm);
        free_job = max(free_job, t);
    }
    return free_job;
}
void *consume_jobs(void *param) {
    Shared *shm = static_cast<Shared *>(param);
    while (shm->jobs_remaining) {
        // cout << "remaining " << shm->jobs_remaining << endl;

        int free_job = first_free_job(ROOT_INDEX, shm);
        if (free_job == -1) {
            usleep(200000);
            continue;
        }
        sem_wait(&(shm->jobs[free_job].lock));
        if (shm->jobs[free_job].process_status == NOT_STARTED &&
            shm->jobs[free_job].active_childs == 0) {
            sem_wait(&(shm->mutex));

            shm->jobs_remaining--;

            shm->jobs[free_job].process_status = ON_GOING;
            cout << "Job Started : " << free_job << endl;
            shm->jobs[free_job].print_job();

            sem_post(&(shm->mutex));

            usleep(shm->jobs[free_job].job_time);

            sem_wait(&(shm->mutex));

            shm->jobs[shm->jobs[free_job].par].active_childs--;

            shm->jobs[free_job].process_status = DONE;
            cout << "Job Completed : " << free_job << endl;
            shm->jobs[free_job].print_job();

            sem_post(&(shm->mutex));
        }
        sem_post(&(shm->jobs[free_job].lock));
    }
    // cout << "consumer end" << shm->jobs_remaining << endl;
    pthread_exit(0);
    return NULL;
}

int main() {
    int shm_id;
    while ((shm_id = shmget(rand(), sizeof(Shared), 0666 | IPC_CREAT)) < 0) {
    }

    // ifstream fin("input.txt", ios::out);
    srand(time(0));

    Shared *shm = init_shm(shm_id);

    int producer_threads = 15, consumer_threads = 12;
    cout << "producer threads : ";
    cin >> producer_threads;
    cout << "consumer threads :";
    cin >> consumer_threads;

    pthread_t tid_p[producer_threads];
    pthread_attr_t attr_p[producer_threads];

    pthread_t tid_c[consumer_threads];
    pthread_attr_t attr_c[consumer_threads];

    int pd_c = fork();
    if (pd_c < 0) {
        perror("could not be forked");
        exit(-1);
    } else if (pd_c == 0) {
        for (int i = 0; i < consumer_threads; i++) {
            pthread_attr_init(&attr_c[i]);
            pthread_create(&tid_c[i], &attr_c[i], consume_jobs, shm);
        }
        for (int i = 0; i < consumer_threads; i++) {
            pthread_join(tid_c[i], NULL);
        }
        exit(0);
    } else if (pd_c != 0) {
        for (int i = 0; i < producer_threads; i++) {
            pthread_attr_init(&attr_p[i]);
            pthread_create(&tid_p[i], &attr_p[i], produce_jobs, shm);
        }
        for (int i = 0; i < producer_threads; i++) {
            pthread_join(tid_p[i], NULL);
        }
        sem_wait(&(shm->mutex));

        sem_post(&(shm->mutex));
    }
    wait(0);
    cout << "Begin Parent: \n";
    // print_tree_inorder(ROOT_INDEX, shm);
    cout << shm->next << endl;
    cout << endl;
    shmdt(shm);
    shmctl(shm_id, IPC_RMID, 0);
    return 0;
}
