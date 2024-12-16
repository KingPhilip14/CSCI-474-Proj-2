#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstdlib>

// create constant values for the number of guests, the number of rooms, and activities guests can partake in 
const int MAX_GUESTS = 5;
const int MAX_ROOMS = 3
const char* activities[4] = {"swam at the swimming pool", "ate at the restaurant", 
                             "exercised at the fitness center", "visited the business center"}

// create the variables for the program
sem_t check_in_sem;
sem_t check_out_sem;
sem_t available_room_sem[MAX_ROOMS];
sem_t check_in_available_sem;
sem_t check_out_available_sem;

int totalGuests = 0;
int pool = 0;                     	  
int restaurant = 0;                  	  
int fitnessCenter = 0;
int businessCenter = 0;

int activityTracker[4] = {pool, restaurant, fitnessCenter, businessCenter}

void *guest(void *arg) {
    int guest_id = *((int *)arg);

    // Simulate guest entering hotel
    printf("Guest %d enters the hotel\n", guest_id);

    // Check-in process
    sem_wait(&check_in_sem);
    printf("Guest %d goes to the check-in receptionist\n", guest_id);
    sem_wait(&check_in_available_sem);
    sem_post(&check_in_sem);

    int room_assigned = -1;

    // for all rooms, find the first one that is available 
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (sem_trywait(&available_room_sem[i]) == 0) {
            room_assigned = i;
            break;
        }
    }

    // if no room was assigned, say that guest is still waiting for one and return
    if (room_assigned == -1)
    {
        printf("Guest %d is waiting for a room\n", guest_id);
        return;
    }

    // if there is an available room, check in the guest
    printf("Guest %d receives Room %d and completes check-in\n", guest_id, room_assigned);

    // increment the amount of guests that have been checked in 
    totalGuests++;

    // grab a random activity from the list of activities and display what the guest does
    int rand_activity_num = rand() % 4;
    printf("Guest %d %s\n", guest_id, activities[rand_activity_num]);

    // increment the amount of times the specific activity was done by a guest
    activityTracker[rand_activity_num]++;

    sleep(rand() % 3 + 1);

    // Check-out process
    sem_wait(&check_out_sem);
    printf("Guest %d goes to the check-out receptionist and returns room %d\n", guest_id, room_assigned);
    sem_post(&available_room_sem[room_assigned]);
    sem_post(&check_out_available_sem);
    printf("Guest %d receives the receipt\n", guest_id);
    sem_post(&check_out_sem);
}

void *checkIn(void *arg) {
    while (1) {
        sem_wait(&check_in_available_sem);
        sem_post(&check_in_available_sem);
        printf("The check-in receptionist greets Guest %d\n", *((int *)arg));
        sem_post(&check_in_sem);
        sleep(1);
    }
}

void *checkOut(void *arg) {
    while (1) {
        sem_wait(&check_out_available_sem);
        sem_post(&check_out_available_sem);
        printf("The check-out receptionist greets Guest %d and receives the key from room %d\n", *((int *)arg), *((int *)arg));
        printf("The receipt was printed\n");
        sem_post(&check_out_sem);
        sleep(1);
    }
}

int main() {
    // seeds a random number to prevent using the same seed for each execution
    srand(time(nullptr));

    pthread_t guests[MAX_GUESTS];
    pthread_t check_in_thread, check_out_thread;

    int guest_ids[MAX_GUESTS];

    // create a thread for every guest
    for (int i = 0; i < MAX_GUESTS; i++) {
        guest_ids[i] = i;
        pthread_create(&guests[i], NULL, guest, &guest_ids[i]);
    }

    pthread_create(&check_in_thread, NULL, checkIn, &guest_ids[0]);
    pthread_create(&check_out_thread, NULL, checkOut, &guest_ids[0]);

    for (int i = 0; i < MAX_GUESTS; i++) {
        pthread_join(guests[i], NULL);
    }

    pthread_cancel(check_in_thread);
    pthread_cancel(check_out_thread);

    return 0;
}