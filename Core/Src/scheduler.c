/*
 * scheduler.c
 *
 *  Created on: Dec 1, 2023
 *      Author: HP
 */

/*
 * scheduler.c
 *
 *  Created on: Nov 13, 2023
 *      Author: hhaof
 */

#include "scheduler.h"
#include "main.h"
#include "stdio.h"
#include "stdlib.h"


struct sTask{
	struct sTask *prev, *next;
	void (*funcPointer)();
	uint32_t Delay;
	uint32_t Period, taskID;
	uint8_t Delay_Flag;
}*SCH_tasks_G = 0;

#define SCH_MAX_TASKS 40
#define LOCK 100
#define UNLOCK 200

uint8_t taskList[SCH_MAX_TASKS];
uint8_t lock = UNLOCK;


void SCH_Arrange_Task(struct sTask *updateTask){
	if(updateTask == SCH_tasks_G){
		return;
	}
	struct sTask *cur = SCH_tasks_G;
	uint8_t check = 0;
	while(cur->Delay <= updateTask->Delay) {
		if (cur == SCH_tasks_G) check += 1;
		if (check == 2) break;
		updateTask->Delay -= cur->Delay;
		cur = cur->next;
	}

	if(updateTask->Delay <= 0) updateTask->Delay_Flag = 1;
	updateTask->next = cur;
	updateTask->prev = cur->prev;
	cur->prev->next = updateTask;
	cur->prev = updateTask;

	//Update
	// Task is update to lowest
	if(check == 0){
			cur->Delay -= updateTask->Delay;
			if(cur->Delay <= 0) cur->Delay_Flag = 1;
			SCH_tasks_G = updateTask;
	}
	// Task is update to middle
	else if (check == 1){
			cur->Delay -= updateTask->Delay;
			if(cur->Delay <= 0) cur->Delay_Flag = 1;
	}
	// Do nothing when task is update to highest
}

void SCH_Add_Task(void (*funcPointer)(), uint32_t Delay, uint32_t Period){
	if (!funcPointer){
		return;
	}

	uint32_t index = 0;
	while ((index < SCH_MAX_TASKS) && (taskList[index])) {
		index += 1;
	}

	if (index == SCH_MAX_TASKS){
		return -1;
	} else taskList[index] = 1;

	lock = LOCK;

	struct sTask *curTask = (struct sTask*)malloc(sizeof(struct sTask));
	curTask->Delay = Delay;
	curTask->Period = Period;
	curTask->funcPointer = funcPointer;
	curTask->taskID = index;
	curTask->Delay_Flag = 0;
	if (SCH_tasks_G) {
		SCH_Arrange_Task(curTask);
	}else{
		curTask->prev = curTask;
		curTask->next = curTask;
		SCH_tasks_G = curTask;
	}

	lock = UNLOCK;
}

void SCH_Update() {
	if ((SCH_tasks_G) && (!SCH_tasks_G->Delay_Flag) && (lock == UNLOCK)){
		if(SCH_tasks_G->Delay > 0) SCH_tasks_G->Delay -= 1;
		if(SCH_tasks_G->Delay <= 0) SCH_tasks_G->Delay_Flag = 1;
	}
}

void SCH_Dispatch_Tasks(){
	lock = LOCK;
	while ((SCH_tasks_G) && (SCH_tasks_G->Delay_Flag)){
		(*SCH_tasks_G->funcPointer)();

		if (SCH_tasks_G->Period == 0) {
			struct sTask *toDelete = SCH_tasks_G;
			SCH_tasks_G = SCH_tasks_G->next;
			SCH_Delete_Task(toDelete->taskID);
		} else {
			struct sTask *toFind = SCH_tasks_G;
			SCH_tasks_G->next->prev = SCH_tasks_G->prev;
			SCH_tasks_G->prev->next = SCH_tasks_G->next;
			SCH_tasks_G = SCH_tasks_G->next;
			toFind->Delay_Flag = 0;
			toFind->Delay = toFind->Period;
			SCH_Arrange_Task(toFind);
		}
	}
	lock = UNLOCK;

	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

void SCH_Delete_Task(uint32_t index){
	if (taskList[index]) {
		taskList[index] = 0;
		struct sTask *cur = SCH_tasks_G;
		while (cur->taskID != index) {
			cur = cur->next;
		}
		cur->next->prev = cur->prev;
		cur->prev->next = cur->next;
		free(cur);
	}
}
