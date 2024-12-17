/**
 * Resources used:
 * https://www.geeksforgeeks.org/thread-functions-in-c-c/
 * https://www.geeksforgeeks.org/cpp-20-semaphore-header/
 * https://pubs.opengroup.org/onlinepubs/009696699/functions/sem_init.html
 */


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
sem_t guest_ready_sem;
sem_t mutex;

// a queue containing all available rooms
queue<int> available_room_queue;

// a queue containing guests waiting to check in
queue<int> check_in_queue;

// a queue for guests waiting to check out; contains the guest ID and guest's room ID
queue<pair<int, int>> check_out_queue;

// a vector containing the potential activities a guest can partake in 
vector<string> guest_activities = {"swimming pool", "restaurant", "fitness center", "business center"};

// works in parallel with guest_activites (i.e., index 0 corresponds to swimming pool)
int activity_counts[4] = {0, 0, 0, 0};
int total_guests = 0;

void* guest(void* arg) {
    int guest_id = *(int*)arg;

    // a guest can only enter the hotel if a room is available
    sem_wait(&available_room_sem);

    printf("Guest %d enters the hotel\n", guest_id);

    // add the guest to the check-in queue
    sem_wait(&mutex);
    check_in_queue.push(guest_id);
    sem_post(&mutex);

    // notify the receptionist that a guest is ready
    sem_post(&check_in_available_sem);
    sem_wait(&guest_ready_sem);

    // get a room for the guest
    sem_wait(&mutex);
    int room_id = available_room_queue.front();
    available_room_queue.pop();
    sem_post(&mutex);
    printf("Guest %d receives Room %d and completes check-in\n", guest_id, room_id);

    // the guest chooses their activity at random
    int rand_activity_index = rand() % guest_activities.size();

    sem_wait(&mutex);
    activity_counts[rand_activity_index]++;
    sem_post(&mutex);
    printf("Guest %d goes to the %s for their activity\n", guest_id, guest_activities[rand_activity_index].c_str());
    sleep(rand() % 3 + 1);

    // guest check out logic
    sem_wait(&mutex);
    check_out_queue.push({guest_id, room_id});
    sem_post(&mutex);
    sem_post(&check_out_available_sem); 
    sem_wait(&guest_ready_sem);         

    // the guest leaves the hotel
    printf("Guest %d leaves the hotel.\n", guest_id);

    // increment the total number of guests who have stayed at the hotel
    sem_wait(&mutex);
    total_guests++;
    sem_post(&mutex);

    // signal that a room is now available again for the next guest
    sem_post(&available_room_sem);

    pthread_exit(nullptr);
}

void* check_in(void* arg) {
    while (true) {
        // wait for a guest to arrive at the check-in desk
        sem_wait(&check_in_available_sem);

        // Retrieve the guest ID from the check-in queue
        sem_wait(&mutex);
        int guest_id = check_in_queue.front();
        check_in_queue.pop();
        sem_post(&mutex);

        // the receptionist greets the guest
        printf("The check-in receptionist greets Guest %d\n", guest_id);

        // signal the guest to proceed with the check-in process
        sem_post(&guest_ready_sem);
    }

    pthread_exit(nullptr);
}

void* check_out(void* arg) {
    while (true) {
        // wait for a guest to arrive at the check-out desk
        sem_wait(&check_out_available_sem);

        // retrieve the guest ID and room ID from the check-out queue
        sem_wait(&mutex);
        pair<int, int> check_out_info = check_out_queue.front();
        check_out_queue.pop();
        sem_post(&mutex);

        // the receptionist greets the guest
        printf("The check-out receptionist greets Guest %d and receives the key for Room %d\n",
               check_out_info.first, check_out_info.second);

        // signal the guest to proceed with the check-out process
        sem_post(&guest_ready_sem);

        // return the room to the available room queue
        sem_wait(&mutex);
        available_room_queue.push(check_out_info.second);
        sem_post(&mutex);
    }

    pthread_exit(nullptr);
}

int main() {
    srand(time(nullptr));

    // initialize the semaphores
    sem_init(&available_room_sem, 0, NUM_ROOMS);
    sem_init(&check_in_available_sem, 0, 0); 
    sem_init(&check_out_available_sem, 0, 0);
    sem_init(&mutex, 0, 1);
    sem_init(&guest_ready_sem, 0, 0);

    // populate the available room queue to keep track of how many rooms are available
    for (int i = 0; i < NUM_ROOMS; i++) {
        available_room_queue.push(i);
    }

    pthread_t guests[NUM_GUESTS];
    pthread_t check_in_thread, check_out_thread;

    // create the receptionist threads
    pthread_create(&check_in_thread, nullptr, check_in, nullptr);
    pthread_create(&check_out_thread, nullptr, check_out, nullptr);

    // create a thread for each guest
    int guest_ids[NUM_GUESTS];
    for (int i = 0; i < NUM_GUESTS; i++) {
        guest_ids[i] = i;
        pthread_create(&guests[i], nullptr, guest, &guest_ids[i]);
    }

    // wait for all guests to finish
    for (int i = 0; i < NUM_GUESTS; i++) {
        pthread_join(guests[i], nullptr);
    }

    // terminate the receptionist threads
    pthread_cancel(check_in_thread);
    pthread_cancel(check_out_thread);

    // print ending information
    printf("\n--- Ending Information ---\n");
    printf("Total Guests: %d\n", total_guests);

    // print the stats for the simulation
    for (int i = 0; i < guest_activities.size(); i++) {
        printf("%s: %d\n", guest_activities[i].c_str(), activity_counts[i]);
    }

    // destroy the semaphores that were used
    sem_destroy(&available_room_sem);
    sem_destroy(&check_in_available_sem);
    sem_destroy(&check_out_available_sem);
    sem_destroy(&mutex);
    sem_destroy(&guest_ready_sem);

    return 0;
}
