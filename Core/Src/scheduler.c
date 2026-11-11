#include "main.h"
#include "scheduler.h"

/* Task queue arranged in a Delta-List structure */
static sTask SCH_tasks_G[SCH_MAX_TASKS];
static uint32_t current_tasks_count = 0;
static uint32_t task_id_counter = 1;

// Initializes the scheduler data structure and resets the task queue.
void SCH_Init(void){
	for(uint32_t i = 0; i < SCH_MAX_TASKS; i++){
		SCH_tasks_G[i].pTask = 0;
		SCH_tasks_G[i].Delay = 0;
		SCH_tasks_G[i].Period = 0;
		SCH_tasks_G[i].RunMe = 0;
		SCH_tasks_G[i].TaskID = NO_TASK_ID;
	}
	current_tasks_count = 0;
	task_id_counter = 1;
}

// Updates the scheduler timing state inside the Timer ISR (O(1) complexity).
// Only decrements the relative delay of the head task.
void SCH_Update(void){
	// If the task queue is empty, do nothing
	if(current_tasks_count == 0 || SCH_tasks_G[0].pTask == 0){
		return;
	}

	// Decrement the relative delay of the task at the head of the queue
	if(SCH_tasks_G[0].Delay > 0) {
		SCH_tasks_G[0].Delay--;
	}

	if(SCH_tasks_G[0].Delay == 0){
		SCH_tasks_G[0].RunMe = 1;
	}
}

/**
  * @brief  Adds a task into the Delta-List queue with sorted relative delays (O(N) complexity).
  * @param  pFunction: Function pointer to the task callback (must be void (*)(void)).
  * @param  DELAY: Initial delay (in scheduler ticks) before first execution.
  * @param  PERIOD: Interval (in scheduler ticks) between runs (0 for one-shot task).
  * @retval Unique Task ID, or NO_TASK_ID if the queue is full.
  */
uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD){
	if(current_tasks_count >= SCH_MAX_TASKS || pFunction == 0){
		return NO_TASK_ID;
	}

	uint32_t new_task_id = task_id_counter++;
	uint32_t insert_index = 0;
	uint32_t remaining_delay = DELAY;

	// 1. Find the insertion index and calculate the relative delta delay
	while(insert_index < current_tasks_count && remaining_delay >= SCH_tasks_G[insert_index].Delay){
		remaining_delay -= SCH_tasks_G[insert_index].Delay;
		insert_index++;
	}

	// 2. Shift subsequent tasks to the right by one position to make room
	for(int32_t i = (int32_t)current_tasks_count; i > (int32_t)insert_index; i--){
		SCH_tasks_G[i] = SCH_tasks_G[i - 1];
	}

	// 3. Insert the new task at the target index
	SCH_tasks_G[insert_index].pTask = pFunction;
	SCH_tasks_G[insert_index].Delay = remaining_delay;
	SCH_tasks_G[insert_index].Period = PERIOD;
	SCH_tasks_G[insert_index].RunMe  = (insert_index == 0 && remaining_delay == 0) ? 1 : 0;
	SCH_tasks_G[insert_index].TaskID = new_task_id;

	current_tasks_count++;

	// 4. Update the relative delay of the immediately following task
	if(insert_index + 1 < current_tasks_count){
		SCH_tasks_G[insert_index + 1].Delay -= remaining_delay;
	}
	return new_task_id;
}

/**
  * @brief  Removes a task from the scheduler queue using its unique Task ID.
  * @param  taskID: Unique identifier returned by SCH_Add_Task.
  * @retval 1 if deleted successfully, 0 if taskID was not found or queue is empty.
  */
uint8_t SCH_Delete_Task(uint32_t taskID){
	if(current_tasks_count == 0 || taskID == NO_TASK_ID){
		return 0;
	}

	uint32_t delete_index = 0;
	while(delete_index < current_tasks_count && SCH_tasks_G[delete_index].TaskID != taskID){
		delete_index++;
	}

	// Task ID not found
	if(delete_index == current_tasks_count){
		return 0;
	}

	// Restore and add the deleted task's remaining delay to the subsequent task
	if(delete_index + 1 < current_tasks_count){
		SCH_tasks_G[delete_index + 1].Delay += SCH_tasks_G[delete_index].Delay;
	}

	// Shift remaining tasks left to fill the gap
	for(uint32_t i = delete_index; i < current_tasks_count - 1; i++){
		SCH_tasks_G[i] = SCH_tasks_G[i + 1];
	}

	// Clear the last slot
	uint32_t last_index = current_tasks_count - 1;
	SCH_tasks_G[last_index].pTask = 0;
	SCH_tasks_G[last_index].Delay = 0;
	SCH_tasks_G[last_index].Period = 0;
	SCH_tasks_G[last_index].RunMe = 0;
	SCH_tasks_G[last_index].TaskID = NO_TASK_ID;

	current_tasks_count--;
	return 1;
}

// Dispatches and executes ready tasks in the background super loop (while(1)).
// Dispatches and executes ready tasks in the background super loop (while(1)).
void SCH_Dispatch_Tasks(void) {
    void (*callbacks_to_run[SCH_MAX_TASKS])(void);
    uint32_t run_count = 0;

    /* Tạm khóa ngắt vài micro-giây để cập nhật hàng đợi Delta-List an toàn */
    __disable_irq();

    /* 1. Rút tất cả các task đã đến hạn (RunMe > 0 hoặc Delay == 0) ra khỏi hàng đợi */
    while (current_tasks_count > 0 && (SCH_tasks_G[0].RunMe > 0 || SCH_tasks_G[0].Delay == 0)) {
        void (*func_to_run)(void) = SCH_tasks_G[0].pTask;
        uint32_t period           = SCH_tasks_G[0].Period;

        if (func_to_run != 0 && run_count < SCH_MAX_TASKS) {
            callbacks_to_run[run_count++] = func_to_run;
        }

        /* Dịch trái mảng để xóa phần tử đầu */
        for (uint32_t i = 0; i < current_tasks_count - 1; i++) {
            SCH_tasks_G[i] = SCH_tasks_G[i + 1];
        }
        uint32_t last_idx = current_tasks_count - 1;
        SCH_tasks_G[last_idx].pTask  = 0;
        SCH_tasks_G[last_idx].Delay  = 0;
        SCH_tasks_G[last_idx].Period = 0;
        SCH_tasks_G[last_idx].RunMe  = 0;
        SCH_tasks_G[last_idx].TaskID = NO_TASK_ID;
        current_tasks_count--;

        /* NẠP LẠI NGAY LẬP TỨC vào Delta-List TRƯỚC KHI chạy hàm in UART */
        if (period > 0) {
            SCH_Add_Task(func_to_run, period, period);
        }
    }

    /* Mở lại ngắt ngay sau khi cấu trúc Delta-List đã cập nhật xong */
    __enable_irq();

    /* 2. Bây giờ mới ung dung thực thi các hàm callback (in UART, Toggle LED) */
    for (uint32_t i = 0; i < run_count; i++) {
        (*callbacks_to_run[i])();
    }
}
