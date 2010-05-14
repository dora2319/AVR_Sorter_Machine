/*
 *************************************************************************************************************
 *                                                uC/OS-II
 *                                          The Real-Time Kernel
 *
 *                                         ATmega2560  Sample code
 *
 * File : APP.C
 * By   : Fabiano Kovalski - portet to ATMEGA2560 by Ib Havn, 2010
 *************************************************************************************************************
 */

#include  <includes.h>

/*
 **************************************************************************************************************
 *                                               VARIABLES
 **************************************************************************************************************
 */

OS_STK AppTaskStartStk[OS_TASK_START_STK_SIZE];
OS_STK AppTask1Stk[OS_TASK_1_STK_SIZE];
OS_STK AppTask2Stk[OS_TASK_2_STK_SIZE];

OS_EVENT *count_sem; // Pointer to a semaphore
OS_EVENT *a_sem;

INT16U count = 0;
INT8U brick_found = 0;

/*
 **************************************************************************************************************
 *                                           FUNCTION PROTOTYPES
 **************************************************************************************************************
 */

static void InitTask(void *p_arg);
static void AppTaskCreate(void);
static void AppTask1(void *p_arg);
//static void AppTask2(void *p_arg);
void LED_Show(INT8U n);
INT8U checkRange(INT8U number, INT8U number2, INT8U threshold);

/*
 **************************************************************************************************************
 *                                           		CONSTANTS
 **************************************************************************************************************
 */
#define SENSOR_1_THRESHOLD 		5
#define SENSOR_1_DEFAULT 		211

#define TASK_1_PRIO				5
#define TASK_2_PRIO 			4
#define BLACK_BRICK				204
#define YELLOW_BRICK			196
/*
 **************************************************************************************************************
 *                                                MAIN
 *
 * Note(s): 1) You SHOULD use OS_TASK_STK_SIZE (see OS_CFG.H) when setting OSTaskStkSize prior to calling
 *             OSInit() because OS_TASK_IDLE_STK_SIZE and OS_TASK_STAT_STK_SIZE are set to this value in
 *             OS_CFG.H.
 **************************************************************************************************************
 */

int main(void) {
#if (OS_TASK_NAME_SIZE > 14) && (OS_TASK_STAT_EN > 0)
	INT8U err;
#endif
	//DDRB = 0x0F;  // TODO IHA Remove after test

#if 0
    BSP_IntDisAll();                       /* For an embedded target, disable all interrupts until we are ready to accept them */
#endif
	/*---- Any initialization code prior to calling OSInit() goes HERE -------------------------------------*/

	/* IMPORTANT: MUST be setup before calling 'OSInit()'  */
	OSTaskStkSize = OS_TASK_IDLE_STK_SIZE; /* Setup the default stack size                        */

	OSInit(); /* Initialize "uC/OS-II, The Real-Time Kernel"         */

	/*---- Any initialization code before starting multitasking --------------------------------------------*/
	OSTaskStkSize = OS_TASK_START_STK_SIZE;
	OSTaskCreateExt(InitTask,
					(void *) 0,
					(OS_STK *) &AppTaskStartStk[OSTaskStkSize - 1],
					TASK_1_PRIO,
					TASK_1_PRIO,
					(OS_STK *) &AppTaskStartStk[0],
					OSTaskStkSize,
					(void *) 0,
					OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

#if (OS_TASK_NAME_SIZE > 10) && (OS_TASK_STAT_EN > 0)
	OSTaskNameSet(TASK_1_PRIO, "Start Task", &err);
#endif

	/*---- Task initialization code goes HERE! --------------------------------------------------------*/
	OSTaskStkSize = OS_TASK_1_STK_SIZE; /* Setup the default stack size                     */
	OSTaskCreateExt(AppTask1,
					(void *) 0,
					(OS_STK *) &AppTask1Stk[OSTaskStkSize- 1],
					TASK_2_PRIO,
					TASK_2_PRIO,
					(OS_STK *) &AppTask1Stk[0],
					OSTaskStkSize,
					(void *) 0,
					OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

#if (OS_TASK_NAME_SIZE > 14) && (OS_TASK_STAT_EN > 0)
	OSTaskNameSet(TASK_2_PRIO, "Task 1", &err);
#endif

	count_sem = OSSemCreate(1); // create a semaphore

	/*---- Create any other task you want before we start multitasking -------------------------------------*/

	OSStart(); /* Start multitasking (i.e. give control to uC/OS-II)  */
	return 0;
}

/*
 **************************************************************************************************************
 *                                              STARTUP TASK
 *
 * Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
 *               initialize the ticker only once multitasking has started.
 *
 * Arguments   : p_arg   is the argument passed to 'AppStartTask()' by 'OSTaskCreate()'.
 *
 * Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
 *                  used.  The compiler should not generate any code for this statement.
 **************************************************************************************************************
 */

static void InitTask(void *p_arg) {

	(void) p_arg; 					/* Prevent compiler warnings                          */
#if OS_TASK_STAT_EN > 0
    OSStatInit();                                /* Determine CPU capacity                                                     */
#endif
	INT8U err;
	INT8U light_value;

	BSP_Init(); 					/* Initialize the BSP                                 */
	init_lego_interface(); 			/* Initalize LEGO_interface						   */

	//AppTaskCreate();
	light_sensor(0);
	//OSTimeDlyHMSM(0,0,1,0);
	while (1) { 						/* Task body, always written as an infinite loop.     */

		if (count < 5)
		{
			//light_value = light_value >> 2;		//convert 10 bit to 8 bit
			//LED_Show(light_value);
			motor_speed(0, -40);				// start motor 0
			OSTimeDly(OS_TICKS_PER_SEC / 2);
			light_value = light_sensor(0) >> 2; 		//initialize light sensor 0

			if(light_value < 205)
			{
				OSSemPend(count_sem, 0, &err);
				count++;
				OSSemPost(count_sem);
				//motor_speed(2, 5);
				LED_Show(count);
				OSTimeDly(OS_TICKS_PER_SEC / 2);
			}
		}
	}
}

/*
 **************************************************************************************************************
 *                                        CREATE APPLICATION TASKS
 *
 * Description : This function creates the application tasks.
 *
 * Arguments   : p_arg   is the argument passed to 'AppStartTask()' by 'OSTaskCreate()'.
 *
 * Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
 *                  used.  The compiler should not generate any code for this statement.
 **************************************************************************************************************
 */

//static void AppTaskCreate(void) {

//}

/*
 **************************************************************************************************************
 *                                                   TASK #1
 *
 *     start engine 0
 *     start counting bricks
 **************************************************************************************************************
 */

static void AppTask1(void *p_arg) {
	(void) p_arg; 					/* Prevent compiler warnings                          */

	INT8U err;
	while (1) {

		if (count == 5)
		{
			OSTimeDly(OS_TICKS_PER_SEC );
			OSSemPend(count_sem, 0, &err);
			count--;
			OSSemPost(count_sem);
			motor_speed(0, 0);
			LED_Show(count);

		}
	}
}

/*
 **************************************************************************************************************
 *                                                  TASK #2
 **************************************************************************************************************
 */
/*
 static void AppTask2(void *p_arg) {
 (void) p_arg;

 while (1) {
 //  LED_Toggle(8);
 //  OSTimeDly(OS_TICKS_PER_SEC / 5);
 }
 } */

/*
 * Custom functions
 */

void LED_Show(INT8U n) {
	/* Turn off all LEDs before switching them */
	int i;
	for (i = 0; i < 8; i++) {
		LED_Off(i);
		//printf("%s%d\n", "Switching off LED #", i);
	}

	/* For all bits in datatype */
	for (i = 1; i <= 8; i++) {

		if (n & (1 << (i - 1))) /* Test if i^th bit is set in "n" */
			LED_On(i);
		//printf("%s%d\n", "Switching on LED #", i); /* if yes, turn on the i^th LED */
	}
}

INT8U checkRange(INT8U number, INT8U number2, INT8U threshold){
	if((number2 + threshold) )
	if((number > (number2 - threshold)) && (number < (number2 + threshold)))
		return 1;
	return 0;
}
/*
 *********************************************************************************************************
 *                                           TASK SWITCH HOOK
 *
 * Description: This function is called when a task switch is performed.  This allows you to perform other
 *              operations during a context switch.
 *
 * Arguments  : none
 *
 * Note(s)    : 1) Interrupts are disabled during this call.
 *              2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
 *                 will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
 *                 task being switched out (i.e. the preempted task).
 *********************************************************************************************************
 */
void App_TaskSwHook(void) // TODO IHA Remove after test
{
	PORTB = ~(OSTCBHighRdy->OSTCBPrio);
}

void App_TaskCreateHook(OS_TCB *ptcb) {
}
void App_TaskDelHook(OS_TCB *ptcb) {
}
void App_TaskIdleHook(void) {
}
void App_TaskStatHook(void) {
}
void App_TCBInitHook(OS_TCB *ptcb) {
}
void App_TimeTickHook(void) {
}

