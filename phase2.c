#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Configuration - experiment w/ diff values
#define NUM_ACCOUNTS 2
#define NUM_THREADS 4
#define TRANSACTIONS_PER_THREAD 10
#define INITIAL_BALANCE 1000.0

// Updated account data structure w/ mutex(given)
typedef struct {
	int account_id;
	double balance;
	int transaction_count;
	pthread_mutex_t lock; // NEW mutex for this account
} Account;

// Global shared array - THIS CAUSES RACE CONDITIONS!
Account accounts[NUM_ACCOUNTS];

// GIVEN: Example of mutex initialization
void initialize_accounts() {
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		accounts[i].account_id = i;
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;

		//initialize the mutex
		pthread_mutex_init(&accounts[i].lock, NULL);
	}
}

// GIVEN: example deposit function WITH proper protection
void deposit_safe(int account_id, double amount) {
	//Acquire lock BEFORE accessing shared data
	pthread_mutex_lock(&accounts[account_id].lock);

	// ==== CRITICAL SECTION ====
	//only ONE thread can execute this at a time for this account
	double current_balance = accounts[account_id].balance;
	usleep(1);
	accounts[account_id].balance = current_balance + amount;
	accounts[account_id].transaction_count++;
	// ==========================

	// Release lock AFTER modifying shared data
	pthread_mutex_unlock(&accounts[account_id].lock);
}

//TODO 1: Implement withdrawl_safe() with mutex protection
// Reference: Follow the pattern of deposit_safe() above
// Remember: lock BEFORE accessing data, unlock AFTER 
void withdrawal_safe(int account_id, double amount) {
	//Hint: pthread_mutex_lock
	//Hint: Modify balance
	//Hint: pthread_mutex_unlock

	//Acquire lock BEFORE accessing shared data
	pthread_mutex_lock(&accounts[account_id].lock);

	// ==== CRITICAL SECTION ====
	double current_balance = accounts[account_id].balance;
	usleep(1);
	accounts[account_id].balance = current_balance - amount;
	accounts[account_id].transaction_count++;
	// ==========================

	//Release lock AFTER modifying shared data
	pthread_mutex_unlock(&accounts[account_id].lock);
}

// Implemented the thread function
// TODO 2: Update teller_thread to use safe functions
void* teller_thread(void* arg) {
	int teller_id = *(int*)arg; // extract thread ID

	// Initialize thread-safe random seed
	unsigned int seed = time(NULL) ^ pthread_self();

	for(int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
		// Randomly select an account (0 to NUM_ACCOUNTS - 1)
		int account_idx = rand_r(&seed) % NUM_ACCOUNTS;

		// Generate random amount (1-100)
		double amount = (rand_r(&seed) % 100) + 1.0;

		// Randomly choose deposit (1) or withdrawal (0)
		int operation = rand_r(&seed) % 2;

		// Call appropriate function
		if (operation == 1) {
			deposit_safe(account_idx, amount);//changed to safe
			printf("Teller %d: Depositied $%.2f to Account %d\n", teller_id, amount, account_idx);
		} else {
			//call withdrawal_safe
			withdrawal_safe(account_idx, amount);
			printf("Teller %d: Withdrew $%.2f from Account %d\n", teller_id, amount, account_idx);
		}
	}

	return NULL;
}

// TODO 4: Add mutex cleanup in main()
// Reference: man pthread_mutex_destroy
// Important: Destroy mutexes AFTER all threads complete!
//Provided method:
void cleanup_mutexes() {
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		pthread_mutex_destroy(&accounts[i].lock);
	}
}

//Implement main function
int main() {

	printf("=== Phase 2: Mutex Protection Application ===\n\n");

	// Initialize accounts with mutexes using provided method
	initialize_accounts();

	//Display initial state (GIVEN)
	printf("Initial State:\n");
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf(" Account %d: $%.2f\n", i, accounts[i].balance);
	}

	// Calculate expected final balance
	double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE;
	printf("\nExpected total: $%2f.\n\n", expected_total);

	// Create thread and thread ID arrays
	pthread_t threads[NUM_THREADS];
	int thread_ids[NUM_THREADS]; // Separate array for IDs

	// TODO 3: Add performance timing
	//Reference: Section 7.2 "Performance Measurement"
	//Hint: Use clock_gettime(CLOCK_MONOTONIC, &start
	//Placed here to make sure the overhead of synchronization, specifically, is measured
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	// Create all threads
	for (int i = 0; i < NUM_THREADS; i++) {
		thread_ids[i] = i; // store ID persistently
		pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);
	}

	// Wait for all threads to complete
	// Reference: man pthread_join
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	//end performance measurement HERE, before results are printed
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed = (end.tv_sec - start.tv_sec) +
			 (end.tv_nsec - start.tv_nsec) / 1e9;

	// Calculate and display results
	printf("\n=== Final Results ===\n");
	double actual_total = 0.0;

	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf("Account %d: $%.2f (%d transactions) \n",
			i, accounts[i].balance, accounts[i].transaction_count);
		actual_total += accounts[i].balance;
	}

	printf("\nExpected total: $%.2f\n", expected_total);
	printf("Actual total:	$%.2f\n", actual_total);
	printf("Difference:	$%.2f\n", actual_total - expected_total);

	// Print performance output here
	printf("\nExecution Time: %.4f seconds\n", elapsed);

	// race condition detection message
	// Instruct user to run multiple times
	if (expected_total != actual_total) {
		printf("\nRACE CONDITION DETECTED!");
	}
	printf("\nRun multiple times to observe Different results.\n\n");

	//TODO 4 finish: call cleanup_mutexes()
	cleanup_mutexes();

	return 0;
}
