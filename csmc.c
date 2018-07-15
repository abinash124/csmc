/** CS 4348.003 Project 3 
 * Anthony Iorio ati140030
 * Lucas Castro ldc140030
* 
* Sleeping Tutor problem.
* 
* Coordinator, tutors, students, chairs, #ofHelps.
* 
**/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>


// arbitrary caps for number of students and number of tutors
#define MAX_STUDENTS 50
#define MAX_TUTORS 50

// function headers
void *student(void *iter);
void *tutor(void *);
void *coordinator(void *);
int getRandomNumber(int seed);
void goToBed(int duration);


// semaphore declarations
sem_t mutex;
sem_t studentInChair;
sem_t tutorAvailability;
sem_t goTeach;
sem_t goLearn;

// counters and boolean for csmcOpen
bool csmcOpen = true;
int numStudents = 0;
int numTutors = 0;
int numChairsMax = 0;
int maxIteration = 0;
int numChairsAvailable = 0;
int waitingStudents = 0;
int helpedCount = 0;


int main(int argc, char *argv[]) {
        // create thread IDs
	pthread_t cid;
	pthread_t tid[MAX_TUTORS];
	pthread_t sid[MAX_STUDENTS];
	int i; // counter for for loops

	if (argc != 5){
		printf("Usage: <# of Students> <# of tutors> <# of chairs> <iteration of help>\n");
		exit(-1);
	}
	

	numStudents = atoi(argv[1]);
	numTutors = atoi(argv[2]);
	numChairsAvailable = atoi(argv[3]);
	maxIteration = atoi(argv[4]);

	numChairsMax =  numChairsAvailable;	// numChairsMax will remain unchanged, numChairsAvailable will fluctuate

	if(numStudents > 50 || numTutors > 50){
		printf("Too many students or tutors. Max is 50.\n");
		exit(-1);
	}

	// initialize semaphores with appropriate values
	sem_init(&mutex, 0, 1);
	sem_init(&studentInChair, 0, 0);
	sem_init(&tutorAvailability, 0, numTutors);
	sem_init(&goTeach, 0, 0);
	sem_init(&goLearn, 0, 0);

	// create coordinator thread
	pthread_create(&cid, NULL, coordinator, NULL);
	
	// create student threads, pass in index as a legible studentID
	for (i=0;i<numStudents;i++)
		pthread_create(&sid[i], NULL, student, (void*) (size_t) i);	
	// create tutor threads, pass in index as a legible tutorID	
	for (i=0;i<numTutors;i++)
		pthread_create(&tid[i], NULL, tutor, (void*) (size_t) i);	
        // join student threads
	for (i=0; i<numStudents; i++)					
		pthread_join(sid[i], NULL);
	// close center
	csmcOpen = false;
	// join tutor threads
	for (i=0; i<numTutors; i++)
		pthread_join(tid[i], NULL);				
	// join coordinator thread
	pthread_join(cid, NULL);				
	
	return 0;
}

void* coordinator(void *fakeNews){
	int teachValue;
	while (csmcOpen){
		if (helpedCount < (numStudents*maxIteration)){		// breaks loop when all students are helped
			sem_wait(&tutorAvailability);			// waits for a tutor to be available
			sem_wait(&studentInChair);			// waits for student to sit in a chair
		} 
		
		// extract value of semaphore
		sem_getvalue(&goTeach, &teachValue);

		if (teachValue > numTutors)
			csmcOpen = false;

		sem_post(&goTeach);	// wake tutor
		sem_post(&goLearn);	// wake student

		helpedCount++;		// increase number of students helped		

	}
}

void* student(void *i){
    int studentID = (long) i;
	int curIteration = 0;						// track current iteration	

	while (curIteration < maxIteration){				// while the student is still seeking help
		if ((rand() % 2) == 1){					// if the student decides to get help
			if (waitingStudents < numChairsMax){		// if chair is available
				sem_wait(&mutex);
				numChairsAvailable--;			// student takes seat
				waitingStudents++;			// increment waiting students
				printf("Student %d takes a seat. Waiting students = %d\n", studentID, waitingStudents);
				curIteration++;				// increment iteration
				sem_post(&studentInChair);		// notify coordinator
				sem_post(&mutex);			// let other people try to sit down
				sem_wait(&goLearn);			// waiting on coordinator				
							
				sem_wait(&mutex);						
					numChairsAvailable++;		// student gets up from seat		
				sem_post(&mutex);	

				goToBed(getRandomNumber(3));		// student sleeps
			} else {				        // else, no chair is available
				printf("Student %d found no empty chair and wil try again later.\n", studentID);
				goToBed(getRandomNumber(5));
			}
		} else{							// else, student decides not to get help
			goToBed(getRandomNumber(3));			// student sleeps
		}
	}
}

void* tutor(void *i){
    int tutorID = (long) i;
    int randomNumber = getRandomNumber(4);				// extract random number

	while (csmcOpen){
		sem_wait(&goTeach);					// wait on coordinator
		sem_post(&tutorAvailability);				// tutor is able to teach

		sem_wait(&mutex);			
			waitingStudents--;				// decrement waiting students
		sem_post(&mutex);						
		
		if (waitingStudents >= 0)				
			printf("Tutor %d helping student for %d seconds. Waiting students = %d.\n", tutorID, randomNumber, waitingStudents);
		
		goToBed(randomNumber);					// tutor sleeps (helping student)
	}
}

// random number generator
int getRandomNumber(int seed){
	return (int) ((drand48() * seed) + 1);
}

// sleep function
void goToBed(int duration){
	sleep(duration);
}

// print semaphores for testing purposes. 
void printSemaphores(){
    int s, t, m, gt, gl;
	sem_getvalue(&studentInChair, &s);
	sem_getvalue(&tutorAvailability, &t);
	sem_getvalue(&mutex, &m);
	sem_getvalue(&goTeach, &gt);
	sem_getvalue(&goTeach, &gl);
	printf("studentInChair: %d, tutorAvailability: %d, mutex: %d, goTeach: %d, goLearn: %d\n", s, t, m, gt, gl);
}
