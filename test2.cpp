#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <vector>
#include <unistd.h>
#include <cstdlib>

using namespace std;

const int NUM_GUESTS = 5;
const int NUM_ROOMS = 3;

sem_t check_in_available_sem;
sem_t check_out_available_sem;
sem_t available_room_sem;
sem_t mutex;

queue<int> available_room_queue;
vector<string> guest_activities = {"swimming pool", "restaurant", "fitness center", "business center"};

int activity_counts[4] = {0, 0, 0, 0};
int total_guests = 0;

void* guest(void* arg) {
    int guest_id = *(int*)arg;

    sem_wait(&available_room_sem);

    printf("Guest %d enters the hotel.\n", guest_id);

    sem_wait(&check_in_available_sem);
    sem_wait(&mutex);

    int room_id = available_room_queue.front();
    available_room_queue.pop();
    printf("Guest %d goes to the check-in.\n", guest_id);
    printf("Guest %d receives Room %d and completes check-in.\n", guest_id, room_id);

    sem_post(&mutex);
    sem_post(&check_in_available_sem);

    int rand_activity_index = rand() % guest_activities.size();

    sem_wait(&mutex);
    activity_counts[rand_activity_index]++;
    sem_post(&mutex);

    printf("Guest %d goes to the %s for their activity.\n", guest_id, guest_activities[rand_activity_index].c_str());
    sleep(rand() % 3 + 1);

    sem_wait(&check_out_available_sem);
    sem_wait(&mutex);

    printf("Guest %d goes to the check-out receptionist and returns room %d.\n", guest_id, room_id);

    available_room_queue.push(room_id);

    printf("Guest %d receives the receipt.\n", guest_id);

    sem_post(&mutex);
    sem_post(&check_out_available_sem);

    printf("Guest %d leaves the hotel.\n", guest_id);
    sem_wait(&mutex);
    total_guests++;
    sem_post(&mutex);

    pthread_exit(nullptr);
}

void* check_in(void* arg) {
    while (true) {
        sem_wait(&check_in_available_sem);
        printf("The check-in receptionist is available.\n");
        sem_post(&check_in_available_sem);
        sleep(1);
    }
    pthread_exit(nullptr);
}

void* check_out(void* arg) {
    while (true) {
        sem_wait(&check_out_available_sem);
        printf("The check-out receptionist is available.\n");
        sem_post(&check_out_available_sem);
        sleep(1);
    }
    pthread_exit(nullptr);
}

int main() {
    srand(time(nullptr));

    sem_init(&available_room_sem, 0, NUM_ROOMS);
    sem_init(&check_in_available_sem, 0, 1);
    sem_init(&check_out_available_sem, 0, 1);
    sem_init(&mutex, 0, 1);

    for (int i = 0; i < NUM_ROOMS; i++) {
        available_room_queue.push(i);
    }

    pthread_t guests[NUM_GUESTS];
    pthread_t check_in_thread, check_out_thread;

    pthread_create(&check_in_thread, nullptr, check_in, nullptr);
    pthread_create(&check_out_thread, nullptr, check_out, nullptr);

    int guest_ids[NUM_GUESTS];
    for (int i = 0; i < NUM_GUESTS; i++) {
        guest_ids[i] = i;
        pthread_create(&guests[i], nullptr, guest, &guest_ids[i]);
    }

    for (int i = 0; i < NUM_GUESTS; i++) {
        pthread_join(guests[i], nullptr);
    }

    printf("\n--- Ending Information ---\n");
    printf("Total Guests: %d\n", total_guests);

    for (int i = 0; i < guest_activities.size(); i++) {
        printf("%s: %d\n", guest_activities[i].c_str(), activity_counts[i]);
    }

    sem_destroy(&available_room_sem);
    sem_destroy(&check_in_available_sem);
    sem_destroy(&check_out_available_sem);
    sem_destroy(&mutex);

    return 0;
}
