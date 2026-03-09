#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define NUM_ACCOUNTS 2
#define NUM_THREADS 2 //changed to help force deadlock
#define TRANSACTIONS_PER_THREAD 1//changed to be relevant to phase 3 freezing transaction part
#define INITIAL_BALANCE 1000

//updated account data structure w/ mutex(given)
typedef struct {
	int account_id;
	double balance;
	int transaction_count;
	pthread_mutex_t lock;
} Account;

//Global shared array
Account accounts[NUM_ACCOUNTS];

// Mutex initialization (given)
void initialize_accounts() {
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		accounts[i].account_id = i;
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;
		pthread_mutex_init(&accounts[i].lock, NULL);
	}
}

// TODO 1: Implement complete transfer function
// basis given
void transfer_deadlock(int from_id, int to_id, double amount) {

	//Lock source account(given)
	pthread_mutex_lock(&accounts[from_id].lock);
	printf("Thread %ld: Locked account %d\n", pthread_self(), from_id);

	//simulate processing delay(given)
	usleep(100);

	//try to lock destination account
	printf("Thread %ld: Waiting for account %d\n", pthread_self(), to_id);
	pthread_mutex_lock(&accounts[to_id].lock); // DEADLOCK

	//Transfer (never reached if deadlocked)
	if(accounts[from_id].balance >= amount) {//if the 'from' account  balance is at least as much as the amount being requested to transfer...
		//performs transaction and increments transaction count
		accounts[from_id].balance -= amount;
		accounts[to_id].balance += amount;
		accounts[from_id].transaction_count++;
		accounts[to_id].transaction_count++;
		//prints confirmation messages
		printf("Transferred $%.2f from %d to %d\n", amount, from_id, to_id);
	} else {//if the sender doesn't have enough money, print issue message
		printf("Transfer failed. Insufficient funds in Account %d.\n", from_id);
	}

	//unlock both accounts
	pthread_mutex_unlock(&accounts[to_id].lock);
	pthread_mutex_unlock(&accounts[from_id].lock);
}

// TODO 2: Create threads that will deadlock
// Instruction set guidelines:
//Thread 1: transfer(0, 1, amount) // Locks 0, wants 1
//Thread 2: transfer(1, 0, amount) // Locks 1, wants 0
//This causes CIRCULAR WAIT!
void* deadlock_thread(void* arg) {
	int thread_id = *(int*) arg;
	double amount = 100.0;

	if (thread_id == 0) {//if-else if statement to help prevent race conditions
		//locks 0, wants 1
		transfer_deadlock(0, 1, amount);
	} else if (thread_id == 1) {
		//locks 1, wants 0
		transfer_deadlock(1, 0, amount);
	}

	return NULL;
}

//provided cleanup mutexes method
void cleanup_mutexes() {
	for(int i = 0; i < NUM_ACCOUNTS; i++) {
		pthread_mutex_destroy(&accounts[i].lock);
	}
}

//main function
//using phase 3 instructions and phase 2 finished file as reference
int main() {
	printf("=== Phase 3: Deadlock Scenario (Implementation) ===\n\n");

	initialize_accounts();

	printf("Initial State:\n");
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf(" Account %d: $%.2f\n", i, accounts[i].balance);
	}

	//create thread and thread ID arrays
	pthread_t threads[NUM_THREADS];
	int thread_ids[NUM_THREADS];// separate array for IDs

	// create threads
	for (int i = 0; i < NUM_THREADS; i++) {
		thread_ids[i] = i;
		pthread_create(&threads[i], NULL, deadlock_thread, &thread_ids[i]);
	}
	// TODO 3: Implement deadlock detection (goes here)

	printf("\nWaiting on threads to finish...");//prints clear status message

	for(int i = 0; i < NUM_THREADS; i++) {//still need to add timeout logic here!
		pthread_join(threads[i], NULL);
	}

	//Calculate and display results
	printf("\n=== Final Results ===\n");
	double actual_total = 0.0;

	for(int i = 0; i < NUM_ACCOUNTS; i++) {
		printf("Account %d: $%.2f (%d transactions) \n",
			i, accounts[i].balance, accounts[i].transaction_count);
		actual_total += accounts[i].balance;
	}

	printf("Actual total: $%.2f, actual_total");
	//diff btwn actual and expected total not needed since the focus of this phase is deadlock

	cleanup_mutexes();

	// TODO 4: Document coffman conditions

	return 0;
}
