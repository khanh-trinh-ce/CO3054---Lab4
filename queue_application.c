#include <stdio.h>

// FreeRTOS libraries.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// We need this to use queues.
#include "freertos/queue.h"
#include "sdkconfig.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

QueueHandle_t queue;
static const char* TAG = "Lab4";
// This struct represents a request.
typedef struct 
{
    // Indicates which task this frame is for.
    int32_t id;
    // Actual payload of the frame.
    int32_t val;
} Frame;

// Constructor for a frame.
Frame NewFrame(uint32_t id, uint32_t value) 
{ 
  Frame* p = malloc(sizeof(Frame));
  p->id = id;
  p->val = value;
  return *p;
}

// This function takes in a frame and sends it to a queue.
// It will report to the terminal whether the frame is successfully
// added to the queue or not.
void sendRequest(QueueHandle_t q, Frame f)
{
    // printf("At %dms: [Request Sender] Creating request with ID %d, value %d.\n", pdTICKS_TO_MS(xTaskGetTickCount()), f.id, f.val);
    ESP_LOGI(TAG, "At %dms: [Request Sender] Creating request with ID %d, value %d.\n", pdTICKS_TO_MS(xTaskGetTickCount()), f.id, f.val);
    if (xQueueSend(q, (void*)&f, (TickType_t)0) != pdTRUE)
    {
        // printf("At %dms: [Request Sender] Cannot send request because the queue is full.\n", pdTICKS_TO_MS(xTaskGetTickCount()));
        ESP_LOGI(TAG, "At %dms: [Request Sender] Cannot send request because the queue is full.\n", pdTICKS_TO_MS(xTaskGetTickCount()));
    }
    else
    {
        // printf("At %dms: [Request Sender] Sent request successfully.\n", pdTICKS_TO_MS(xTaskGetTickCount()));
        ESP_LOGI(TAG, "At %dms: [Request Sender] Sent request successfully.\n", pdTICKS_TO_MS(xTaskGetTickCount()));
    }
}

// This high-priority task creates requests and sends them to the queue.
void generateRequest()
{
    // Inititalizes the queue/
    queue = xQueueCreate(5, sizeof(Frame));


    // Create requests.
    Frame f0 = NewFrame(0, 10); // Will be processed by Task 0 Handler.
    Frame f1 = NewFrame(1, 11); // Will be processed by Task 1 Handler.
    Frame f2 = NewFrame(2, 22); // Will be ignored.
    Frame f3 = NewFrame(1, 22); // Will be processed by Task 1 Handler.
    Frame f4 = NewFrame(15, 55); // Will be ignored.
    Frame f5 = NewFrame(2, 100); // Will be discarded due to queue overflow.

    // Send requests.
    sendRequest(queue, f0);
    sendRequest(queue, f1);
    sendRequest(queue, f2);
    sendRequest(queue, f3);
    sendRequest(queue, f4);
    sendRequest(queue, f5); // Cannot add due to queue overflow.

    // This task only executes once.
    while (1)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

// This task will handle Frames of which id is 0.
void zeroIdTaskHandler()
{
    while (1)
    {
        Frame rxFrame;
        if (xQueuePeek(queue, (void*)&rxFrame,(TickType_t)5) == pdTRUE)
        {   
            if (rxFrame.id == 0)
            {
                xQueueReceive(queue, (void*)&rxFrame,(TickType_t)5);
                // printf("At %dms: [ID 0 Task Handler] Event handler 0 called and executed. Value is: %d\n", pdTICKS_TO_MS(xTaskGetTickCount()), rxFrame.val);
                ESP_LOGI(TAG, "At %dms: [ID 0 Task Handler] Event handler 0 called and executed. Value is: %d\n", pdTICKS_TO_MS(xTaskGetTickCount()), rxFrame.val);
            }
            else
            {
                // printf("At %dms: [ID 0 Task Handler] Event handler 0 called but not executed.\n", pdTICKS_TO_MS(xTaskGetTickCount()));
                ESP_LOGI(TAG, "At %dms: [ID 0 Task Handler] Event handler 0 called but not executed.\n", pdTICKS_TO_MS(xTaskGetTickCount()));
                vTaskDelay(1000 / portTICK_RATE_MS);
            }
            
        }
    }
    
}

// This task handles requests of which id is 1.
// Also removes invalid requests.
void oneIdTaskHandler()
{
    while (1)
    {
        Frame rxFrame;
        if (xQueuePeek(queue, (void*)&rxFrame,(TickType_t)5) == pdTRUE)
        {
            if (rxFrame.id == 1)
            {
                xQueueReceive(queue, (void*)&rxFrame,(TickType_t)5);
                // printf("At %dms: [ID 1 Task Handler] Event handler 1 called and executed. Value is: %d\n", pdTICKS_TO_MS(xTaskGetTickCount()), rxFrame.val);
                ESP_LOGI(TAG, "At %dms: [ID 1 Task Handler] Event handler 1 called and executed. Value is: %d\n", pdTICKS_TO_MS(xTaskGetTickCount()), rxFrame.val);
            }
            else
            {
                // printf("At %dms: [ID 1 Task Handler] Event handler 1 called but not executed.\n", pdTICKS_TO_MS(xTaskGetTickCount()));
                ESP_LOGI(TAG, "At %dms: [ID 1 Task Handler] Event handler 1 called but not executed.\n", pdTICKS_TO_MS(xTaskGetTickCount()));
                xQueueReceive(queue, (void*)&rxFrame, (TickType_t)5);
                // printf("At %dms: [ID 1 Task Handler] Discarded invalid frame with ID %d.\n", pdTICKS_TO_MS(xTaskGetTickCount()), rxFrame.id);
                ESP_LOGI(TAG, "At %dms: [ID 1 Task Handler] Discarded invalid frame with ID %d.\n", pdTICKS_TO_MS(xTaskGetTickCount()), rxFrame.id);
            }
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    }  
}

void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    xTaskCreatePinnedToCore(generateRequest, "generateRqeust", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(zeroIdTaskHandler, "zeroIdTaskHandler", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(oneIdTaskHandler, "oneIdTaskHandler", 4096, NULL, 1, NULL, 0);

}
