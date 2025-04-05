#ifndef SUPABASE_CLIENT_H
#define SUPABASE_CLIENT_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

// Supabase configuration
#define SUPABASE_URL "https://tuwpamkgvpovvzvvrevi.supabase.co"
#define SUPABASE_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InR1d3BhbWtndnBvdnZ6dnZyZXZpIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDMyNzA1NTcsImV4cCI6MjA1ODg0NjU1N30.5TDQVcH7wyawIIwr1JNtcPQjovzdrWqfMN-qZSttHfo"
#define SUPABASE_TABLE "oxygen_1"

// Task configuration
#define SUPABASE_TASK_STACK_SIZE 8192
#define SUPABASE_TASK_PRIORITY 5

// Function declarations
void supabase_init(void);
void supabase_send_data(const char* data);
void supabase_task(void *pvParameters);

#endif // SUPABASE_CLIENT_H 