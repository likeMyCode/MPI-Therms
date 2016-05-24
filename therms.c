#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

typedef int bool;

#define MAX_INT 214748364
#define true 1
#define false 0

#define CHANGE_TIME 		100
#define SWIM_TIME 		800
#define REST_TIME 		1500
#define REQUEST_TAG 		1
#define RESPONSE_TAG 		2
#define RANK 			0
#define GENDER 			1
#define LOCKER 			2
#define STATE 			3
#define TIME 			4
#define SCALAR_TIME_PERIOD 	1

//-----------------------------------------------------------------//

void sendLocalRequest ();
void receiveLocalStateResponse (); 
void checkAnotherLockerRoom ();
void tryToEnter ();

enum action {CHANGE, SWIM, REST};
enum state  {WAITING, CHANGING, SWIMMING, RESTING};
enum gender {MALE = 0, FEMALE = 1};

pthread_t tid;
bool starving;
int lockers, lockerRooms, lockerRoomNo, people, state, 
    gender, scalarTime, rank, provided;

//-----------------------------------------------------------------//

void enterSwimmingPool() { state = SWIMMING; }
void leaveSwimmingPool() { state = CHANGING; }
void leaveLocker() { state = RESTING; lockerRoomNo = 0; }
void finalize() { MPI_Finalize(); }


void doAction (int action) {
	int miliseconds;
	char *actionStr;

	switch (action) {
		case CHANGE:
			actionStr = "CHANGING\t";
			miliseconds = 25 + rand() % CHANGE_TIME;
			break;
		case SWIM:
			actionStr = "SWIMMING\t";
			miliseconds = 100 + rand() % SWIM_TIME;
			break;
		case REST:
			actionStr = "RESTING \t";
			miliseconds = 300 + rand() % REST_TIME;
			break;
	}
	printf("%sLOCKER: %d  PROCESS: %d  GENDER: %s\n", actionStr, lockerRoomNo, rank, (gender == FEMALE) ? "Female" : "Male");
	usleep(miliseconds * 1000);
}


void* answerToProcesses() {
	while (1) {
		MPI_Status status;
		int receiveMsg[2];
		int sendMsg[5];	
		
		MPI_Recv(receiveMsg, 2, MPI_INT, MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &status);

		if(receiveMsg[1] > scalarTime) scalarTime = receiveMsg[1];
		
		scalarTime += SCALAR_TIME_PERIOD;

		sendMsg[RANK] = rank;
		sendMsg[GENDER] = gender;
		sendMsg[LOCKER] = lockerRoomNo;
		sendMsg[STATE] = state;
		sendMsg[TIME] = scalarTime;
		
		scalarTime += SCALAR_TIME_PERIOD;
		
		MPI_Send(sendMsg, 5, MPI_INT, receiveMsg[0], RESPONSE_TAG, MPI_COMM_WORLD);
	}
}


void init (int argc, char **argv) {
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &people);

	srand(time(NULL) + rank);	
	lockerRooms = atoi(argv[1]);
	lockers = atoi(argv[2]);
	state = RESTING;
	lockerRoomNo = 0;
	scalarTime = 0;	
	gender = (rand() % 2 == 1) ? FEMALE : MALE;
	starving = false;

	pthread_create(&tid, NULL, &answerToProcesses, NULL);
}


void checkAnotherLockerRoom () {
	if (starving == false) {
		lockerRoomNo++;
		if (lockerRoomNo > lockerRooms) {
			starving = true;
			lockerRoomNo = 1 + rand() % lockerRooms;
		}
	} 
	tryToEnter();
}


bool canEnterByGender (int receivedData[people][5], int emptyLockers) {
	int i, j, myPos, firstGender = -1;

	for (i = 0; i < emptyLockers; i++) {
		int minIdx, minTime = MAX_INT, minRank = MAX_INT;

		for (j = 0; j < people; j++) {
			if (i == 0 || receivedData[j][GENDER] == firstGender) {
				if (receivedData[j][STATE] == WAITING && receivedData[j][LOCKER] == lockerRoomNo) {
					if (receivedData[j][TIME] <= minTime) {
						if (receivedData[j][TIME] == minTime) {
							if (receivedData[j][TIME] + receivedData[j][RANK] < minTime + minRank) {
								minTime = receivedData[j][TIME];
								minRank = receivedData[j][RANK];
								minIdx = j;
							}	
						} else {
							minTime = receivedData[j][TIME];
							minRank = receivedData[j][RANK];
							minIdx = j;
						}
					} 
				}
			}
		}	
		
		if (i == 0) {
			firstGender = receivedData[minIdx][GENDER];
			if (gender != firstGender) return false;
		}
		if (minRank == rank) return true;
		receivedData[minIdx][TIME] = MAX_INT;
	}
	return false;
}


bool canEnter (int receivedData[people][5], int emptyLockers) {
	int i, j, myPos;

	for (i = 0; i < emptyLockers; i++) {
		int minIdx, minTime = MAX_INT, minRank = MAX_INT;

		for (j = 0; j < people; j++) {
			if (receivedData[j][STATE] == WAITING && receivedData[j][LOCKER] == lockerRoomNo) {
				if (receivedData[j][TIME] <= minTime) {
					if (receivedData[j][TIME] == minTime) {
						if (receivedData[j][TIME] + receivedData[j][RANK] < minTime + minRank) {
							minTime = receivedData[j][TIME];
							minRank = receivedData[j][RANK];
							minIdx = j;
						}
					} else {
						minTime = receivedData[j][TIME];
						minRank = receivedData[j][RANK];
						minIdx = j;
					}
				}
			}
		}

		if (minRank == rank) return true;
		receivedData[minIdx][TIME] = MAX_INT;	
	}
	return false;
}


void receiveLocalStateResponse () {
	MPI_Status status;
	int i, receivedData[people][5];

	for (i = 0; i < people; i++) {
		int receiveMsg[5];
		
		MPI_Recv(receiveMsg, 5, MPI_INT, MPI_ANY_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD, &status);	
		
		if(receiveMsg[4] > scalarTime) scalarTime = receiveMsg[4];

		scalarTime += SCALAR_TIME_PERIOD;

		receivedData[i][0] = receiveMsg[0];		
		receivedData[i][1] = receiveMsg[1];
		receivedData[i][2] = receiveMsg[2];
		receivedData[i][3] = receiveMsg[3];
		receivedData[i][4] = receiveMsg[4];
	}

	bool isLockerEmpty = true;
	int emptyLockers = lockers;

	for (i = 0; i < people; i++) {
		if (receivedData[i][GENDER] != gender 
		&&  receivedData[i][LOCKER] == lockerRoomNo
		&& (receivedData[i][STATE]  == CHANGING 
		||  receivedData[i][STATE]  == SWIMMING)) { checkAnotherLockerRoom(); return; }

		
		if (receivedData[i][GENDER] == gender
		&&  receivedData[i][LOCKER] == lockerRoomNo
		&& (receivedData[i][STATE]  == CHANGING 
		||  receivedData[i][STATE]  == SWIMMING)) { isLockerEmpty = false; emptyLockers--; }
	}

	if (isLockerEmpty) {
		if (!canEnterByGender(receivedData, emptyLockers)) {
			checkAnotherLockerRoom();
		}
	} else {
		if (!canEnter(receivedData, emptyLockers)) {
			checkAnotherLockerRoom();
		}
	}			
}


void sendLocalStateRequest () {
	int i, sendMsg[2];
	scalarTime += SCALAR_TIME_PERIOD;
	sendMsg[0] = rank;
	sendMsg[1] = scalarTime;
	for (i = 0; i < people; i++) 
		MPI_Send(sendMsg, 2, MPI_INT, i, REQUEST_TAG, MPI_COMM_WORLD);
}


void tryToEnter () {
	sendLocalStateRequest();
	receiveLocalStateResponse();
}


void takeLocker () {
	starving = false;
	lockerRoomNo = 1;
	state = WAITING;

	tryToEnter();
}


void mainLoop () {
	while (1) {
		takeLocker();
		doAction(CHANGE);
		enterSwimmingPool();
		doAction(SWIM);
		leaveSwimmingPool();
		doAction(CHANGE);
		leaveLocker();
		doAction(REST);
	}
}


int main (int argc, char **argv) {	
	init(argc, argv);
	mainLoop();
	finalize();
}

