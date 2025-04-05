#include "supabase_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "esp_crt_bundle.h"

static const char *TAG = "supabase_client";

// Queue for data to be sent
static QueueHandle_t data_queue = NULL;

// HTTP client configuration
static esp_http_client_config_t http_config = {
    .url = SUPABASE_URL,
    .method = HTTP_METHOD_POST,
    .timeout_ms = 10000,
    .buffer_size = 2048,
    .buffer_size_tx = 2048,
    .skip_cert_common_name_check = false,
    .transport_type = HTTP_TRANSPORT_OVER_SSL,
    .cert_pem = NULL,
    .keep_alive_enable = true,
    .disable_auto_redirect = true,
    .use_global_ca_store = false,
    .crt_bundle_attach = esp_crt_bundle_attach
};

// HTTP client handle
static esp_http_client_handle_t client = NULL;

void supabase_init(void) {
    int64_t start_time = esp_timer_get_time();
    ESP_LOGI(TAG, "Starting Supabase initialization at %lld ms", start_time/1000);

    // Initialize HTTP client
    client = esp_http_client_init(&http_config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }

    // Create queue for data
    data_queue = xQueueCreate(10, sizeof(char*));
    if (data_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create data queue");
        esp_http_client_cleanup(client);
        client = NULL;
        return;
    }

    // Create Supabase task
    BaseType_t xReturned = xTaskCreate(supabase_task, "supabase_task", SUPABASE_TASK_STACK_SIZE, NULL, SUPABASE_TASK_PRIORITY, NULL);
    if (xReturned != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Supabase task");
        vQueueDelete(data_queue);
        data_queue = NULL;
        esp_http_client_cleanup(client);
        client = NULL;
        return;
    }

    int64_t end_time = esp_timer_get_time();
    ESP_LOGI(TAG, "Supabase client initialized successfully in %lld ms", (end_time - start_time)/1000);
}

void supabase_send_data(const char* data) {
    if (data_queue == NULL || data == NULL) {
        ESP_LOGW(TAG, "Queue not initialized or invalid data");
        return;
    }

    char* data_copy = strdup(data);
    if (data_copy == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for data copy");
        return;
    }

    if (xQueueSend(data_queue, &data_copy, 0) != pdTRUE) {
        free(data_copy);
        ESP_LOGW(TAG, "Failed to send data to queue");
    }
}

void supabase_task(void *pvParameters) {
    char* data;
    char url[256];
    const int MAX_RETRIES = 3;
    const int RETRY_DELAY_MS = 1000;

    while (1) {
        if (xQueueReceive(data_queue, &data, portMAX_DELAY) == pdTRUE) {
            int64_t start_time = esp_timer_get_time();
            ESP_LOGI(TAG, "Starting Supabase connection attempt at %lld ms", start_time/1000);

            if (client == NULL) {
                ESP_LOGE(TAG, "HTTP client not initialized");
                free(data);
                continue;
            }

            // Construct the URL for the specific table
            snprintf(url, sizeof(url), "%s/rest/v1/%s", SUPABASE_URL, SUPABASE_TABLE);

            // Set URL and headers
            esp_http_client_set_url(client, url);
            esp_http_client_set_header(client, "apikey", SUPABASE_KEY);
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_header(client, "Prefer", "return=minimal");

            // Set POST data
            esp_http_client_set_post_field(client, data, strlen(data));

            // Implement retry logic
            esp_err_t err = ESP_FAIL;
            int retry_count = 0;
            
            while (retry_count < MAX_RETRIES) {
                ESP_LOGI(TAG, "Connection attempt %d of %d", retry_count + 1, MAX_RETRIES);
                
                err = esp_http_client_perform(client);
                int64_t attempt_end_time = esp_timer_get_time();
                ESP_LOGI(TAG, "Attempt %d completed in %lld ms", retry_count + 1, (attempt_end_time - start_time)/1000);

                if (err == ESP_OK) {
                    int status_code = esp_http_client_get_status_code(client);
                    if (status_code >= 200 && status_code < 300) {
                        ESP_LOGI(TAG, "Data sent successfully on attempt %d", retry_count + 1);
                        break;
                    } else {
                        ESP_LOGW(TAG, "HTTP error on attempt %d, status: %d", retry_count + 1, status_code);
                    }
                } else {
                    ESP_LOGW(TAG, "Connection failed on attempt %d: %s", retry_count + 1, esp_err_to_name(err));
                }

                if (retry_count < MAX_RETRIES - 1) {
                    ESP_LOGI(TAG, "Retrying in %d ms...", RETRY_DELAY_MS);
                    vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
                }
                retry_count++;
            }

            if (err != ESP_OK) {
                ESP_LOGE(TAG, "All connection attempts failed after %d retries", MAX_RETRIES);
            }

            // Free the data
            free(data);
        }
    }
} 