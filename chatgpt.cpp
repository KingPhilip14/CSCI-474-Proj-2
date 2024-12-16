#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <vector>
#include <unistd.h>
#include <cstdlib>

using namespace std;

#define NUM_GUESTS 5
#define NUM_ROOMS 3

// setup variables for the project
sem_t available_rooms;
sem_t check_in_sem;
sem_t check_out_sem;
sem_t mutex;

queue<int> available_room_queue;
vector<string> guest_activities = {"swimming pool", "restaurant", "fitness center", "business center"};

// Counters for activities; activity_counts works in parallel with guest_activities
int activity_counts[4] = {0, 0, 0, 0};
int total_guests = 0;

void* guest(void* arg) {
    int guest_id = *(int*)arg;

    // Guest arrives at the hotel
    printf("Guest %d enters the hotel.\n", guest_id);

    // Check-in process
    sem_wait(&check_in_sem);
    sem_wait(&mutex);

    // Assign a room
    int room_id = available_room_queue.front();
    available_room_queue.pop();
    printf("Guest %d goes to the check-in.\n", guest_id);
    printf("Guest %d receives Room %d and completes check-in.\n", guest_id, room_id);

    sem_post(&mutex);
    sem_post(&check_in_sem);

    // generate a random number to simulate a guest activity
    int rand_activity_index = rand() % guest_activities.size();

    // Hotel activity
    sem_wait(&mutex);
    activity_counts[rand_activity_index]++;
    sem_post(&mutex);
    printf("Guest %d goes to the %s for their activity.\n", guest_id, guest_activities[rand_activity_index].c_str());
    sleep(rand() % 3 + 1);

    // Check-out process
    sem_wait(&check_out_sem);
    sem_wait(&mutex);

    printf("Guest %d goes to the check-out and returns room %d.\n", guest_id, room_id);
    available_room_queue.push(room_id);
    printf("Guest %d receives the receipt.\n", guest_id);

    sem_post(&mutex);
    sem_post(&check_out_sem);

    // Guest leaves
    printf("Guest %d leaves the hotel.\n", guest_id);
    sem_wait(&mutex);
    total_guests++;
    sem_post(&mutex);

    pthread_exit(nullptr);
}

void* check_in(void* arg) {
    while (true) {
        // Busy-wait simulation for activity log
        sleep(1);
    }
    pthread_exit(nullptr);
}

void* check_out(void* arg) {
    while (true) {
        // Busy-wait simulation for activity log
        sleep(1);
    }
    pthread_exit(nullptr);
}

int main() {
    srand(time(nullptr));

    // Initialize semaphores
    sem_init(&available_rooms, 0, NUM_ROOMS);
    sem_init(&check_in_sem, 0, 1);
    sem_init(&check_out_sem, 0, 1);
    sem_init(&mutex, 0, 1);

    // Fill available rooms
    for (int i = 0; i < NUM_ROOMS; i++) {
        available_room_queue.push(i);
    }

    pthread_t guests[NUM_GUESTS];
    pthread_t check_in_thread, check_out_thread;

    // Start receptionist threads
    pthread_create(&check_in_thread, nullptr, check_in, nullptr);
    pthread_create(&check_out_thread, nullptr, check_out, nullptr);

    // Start guest threads
    int guest_ids[NUM_GUESTS];
    for (int i = 0; i < NUM_GUESTS; i++) {
        guest_ids[i] = i;
        pthread_create(&guests[i], nullptr, guest, &guest_ids[i]);
    }

    // Wait for all guests to finish
    for (int i = 0; i < NUM_GUESTS; i++) {
        pthread_join(guests[i], nullptr);
    }

    // Terminate receptionist threads
    pthread_cancel(check_in_thread);
    pthread_cancel(check_out_thread);

    // Print accounting
    printf("\n--- Ending Information ---\n");
    printf("Total Guests: %d\n", total_guests);

    for (int i = 0; i < guest_activities.size(); i++) {
        printf("%s: %d\n", guest_activities[i].c_str(), activity_counts[i]);
    }

    // Clean up
    sem_destroy(&available_rooms);
    sem_destroy(&check_in_sem);
    sem_destroy(&check_out_sem);
    sem_destroy(&mutex);

    return 0;
}
