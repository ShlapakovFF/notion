#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "string.h"

typedef StaticQueue_t osStaticMessageQDef_t;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

osThreadId_t mainTaskHandle;
const osThreadAttr_t mainTask_attributes = { .name = "mainTask", .stack_size = 128 * 4, .priority =
		(osPriority_t) osPriorityNormal, };

osThreadId_t usart_taskHandle;
const osThreadAttr_t usart_task_attributes = { .name = "usart_task", .stack_size = 128 * 4, .priority =
		(osPriority_t) osPriorityNormal, };

osThreadId_t spi_taskHandle;
const osThreadAttr_t spi_task_attributes = { .name = "spi_task", .stack_size = 128 * 4, .priority =
		(osPriority_t) osPriorityNormal, };

osMessageQueueId_t spiQueueHandle;
uint8_t spiQueueBuffer[10 * 10 * sizeof(uint8_t) ];
osStaticMessageQDef_t spiQueueControlBlock;
const osMessageQueueAttr_t spiQueue_attributes = { .name = "spiQueue", .cb_mem = &spiQueueControlBlock, .cb_size =
		sizeof(spiQueueControlBlock), .mq_mem = &spiQueueBuffer, .mq_size = sizeof(spiQueueBuffer) };

osMessageQueueId_t usartQueueHandle;
uint8_t usartQueueBuffer[10 * 10 * sizeof(uint8_t) ];
osStaticMessageQDef_t usartQueueControlBlock;
const osMessageQueueAttr_t usartQueue_attributes = { .name = "usartQueue", .cb_mem = &usartQueueControlBlock, .cb_size =
		sizeof(usartQueueControlBlock), .mq_mem = &usartQueueBuffer, .mq_size = sizeof(usartQueueBuffer) };

void StartMaintTask(void *argument);
void StartUsartTask(void *argument);
void StartSpiTask(void *argument);

void MX_FREERTOS_Init(void);

size_t strlen(const char *str) {
    size_t length = 0;
    while (*str++ != '\0') {
        length++;
    }
    return length;
}

void MX_FREERTOS_Init(void)
{
	spiQueueHandle = osMessageQueueNew(10 , sizeof(uint8_t)* 10, &spiQueue_attributes);
	usartQueueHandle = osMessageQueueNew(10 , sizeof(uint8_t)* 10, &usartQueue_attributes);

	mainTaskHandle = osThreadNew(StartMaintTask, NULL, &mainTask_attributes);
	usart_taskHandle = osThreadNew(StartUsartTask, NULL, &usart_task_attributes);
	spi_taskHandle = osThreadNew(StartSpiTask, NULL, &spi_task_attributes);
}

void StartMaintTask(void *argument)
{
	for (;;) {
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		osDelay(250);
	}
}

void StartUsartTask(void *argument)
{
	uint8_t usartBuffer[10] = {0};
	uint8_t spiBuffer[10] = {0};
	uint16_t len;

	for (;;) {
		if (HAL_UARTEx_ReceiveToIdle(&huart1, usartBuffer, 10, &len, 10) == HAL_OK) {
			osMessageQueuePut(spiQueueHandle, usartBuffer, 0, 0);
			memset(usartBuffer, 0, sizeof(usartBuffer));
		}

		if (osMessageQueueGet(usartQueueHandle, spiBuffer, NULL, 0) == osOK) {
			HAL_UART_Transmit(&huart1, spiBuffer, strlen((char *)spiBuffer), 20);
		}
		osDelay(1);
	}
}

void StartSpiTask(void *argument)
{
	uint8_t spiBuffer[10] = {0};
	uint8_t usartBuffer[10] = {0};
	uint8_t receivedChar;
	uint8_t index = 0;

	for (;;) {
		if (HAL_SPI_Receive(&hspi1, &receivedChar, sizeof(receivedChar), 1) == HAL_OK) {
			usartBuffer[index++] = receivedChar;

			if (receivedChar == '\0' || index >= sizeof(usartBuffer)) {
				osMessageQueuePut(usartQueueHandle, usartBuffer, 0, 0);
				index = 0;
			}
		}

		if (osMessageQueueGet(spiQueueHandle, spiBuffer, NULL, 0) == osOK) {
			HAL_SPI_Transmit(&hspi1, spiBuffer, strlen((char *)spiBuffer), 10);
			//HAL_UART_Transmit(&huart1, spiBuffer, strlen((char *)spiBuffer), 20);
		}
		osDelay(1);
	}
}
