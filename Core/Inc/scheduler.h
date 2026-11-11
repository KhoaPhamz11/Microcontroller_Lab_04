#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#include <stdint.h>

// Define the maximum number of tasks that the scheduler can manage simultaneously
#define SCH_MAX_TASKS 40
#define NO_TASK_ID 0

typedef struct{
	void (*pTask)(void); // Function pointer points to the function to be executed
	uint32_t Delay; // Delay differential (measured in interrupt ticks)
	uint32_t Period; // Period of one-shot task = 0
	uint8_t RunMe;
	uint32_t TaskID;
} sTask;

/* Basic API functions */
void SCH_Init(void);
void SCH_Update(void);
void SCH_Dispatch_Tasks(void);
uint32_t SCH_Add_Task(void (*pFunction)(void),
					  uint32_t DELAY,
					  uint32_t PERIOD
					  );

uint8_t SCH_Delete_Task(uint32_t taskID);

#endif /* INC_SCHEDULER_H_ */
