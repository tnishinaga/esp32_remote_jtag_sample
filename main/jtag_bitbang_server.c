#include "freertos/FreeRTOS.h"
#include <string.h>

#define TCP_PORT 3333

// input
#define TDO_PIN GPIO_NUM_33
// output
#define LED_PIN GPIO_NUM_4
#define TCK_PIN GPIO_NUM_25
#define TMS_PIN GPIO_NUM_26
#define TDI_PIN GPIO_NUM_27
// reset
#define TRST_PIN GPIO_NUM_32
#define SRST_PIN GPIO_NUM_35

static const char *TAG = "jtag_bitbang_server";

int remote_bitbang_process(int sock)
{
    // input
    // tdo
    gpio_set_direction(TDO_PIN, GPIO_MODE_INPUT);

    // output
    // led
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    // tck
    gpio_set_direction(TCK_PIN, GPIO_MODE_OUTPUT);
    // tms
    gpio_set_direction(TMS_PIN, GPIO_MODE_OUTPUT);
    // tdi
    gpio_set_direction(TDI_PIN, GPIO_MODE_OUTPUT);
    // trst
    gpio_set_direction(TRST_PIN, GPIO_MODE_OUTPUT);
    // srst
    gpio_set_direction(SRST_PIN, GPIO_MODE_OUTPUT);

    bool finish = false;
    while (finish != true) {
        uint8_t command;
        // read command
        int len = read(sock, command, 1);
        if (len < 0) {
            ESP_LOGE(TAG, "recv failed: %d", errno);
            return errno;
        }
        if (len == 0) {
            ESP_LOGI(TAG, "connection closed");
            return 0;
        }
        
        // data received
        switch(command) {
            case 'B':
                // Blink on
                gpio_set_level(LED_PIN, 0);
                break;
            case 'b':
                // Blink off
                gpio_set_level(LED_PIN, 1);
                break;
            case 'R':
                // Read request
                uint8_t level = gpio_get_level(TDO_PIN);
                // value to ascii
                level = '0' + level;
                int res = send(sock, &level, 1, 0);
                if (res < 0) {
                    // send error
                    ESP_LOGI(TAG, "send error: %d", errno);
                }
                break;
            case 'Q':
                // Quit request
                finish = true;
                break;
            case '0':
                // Write 0 0 0
                gpio_set_level(TCK_PIN, 0);
                gpio_set_level(TMS_PIN, 0);
                gpio_set_level(TDI_PIN, 0);
                break;
            case '1':
                // Write 0 0 1
                gpio_set_level(TCK_PIN, 0);
                gpio_set_level(TMS_PIN, 0);
                gpio_set_level(TDI_PIN, 1);
                break;
            case '2':
                // Write 0 1 0
                gpio_set_level(TCK_PIN, 0);
                gpio_set_level(TMS_PIN, 1);
                gpio_set_level(TDI_PIN, 0);
                break;
            case '3':
                // Write 0 1 1
                gpio_set_level(TCK_PIN, 0);
                gpio_set_level(TMS_PIN, 1);
                gpio_set_level(TDI_PIN, 1);
                break;
            case '4':
                // Write 1 0 0
                gpio_set_level(TCK_PIN, 1);
                gpio_set_level(TMS_PIN, 0);
                gpio_set_level(TDI_PIN, 0);
                break;
            case '5':
                // Write 1 0 1
                gpio_set_level(TCK_PIN, 1);
                gpio_set_level(TMS_PIN, 0);
                gpio_set_level(TDI_PIN, 1);
                break;
            case '6':
                // Write 1 1 0
                gpio_set_level(TCK_PIN, 1);
                gpio_set_level(TMS_PIN, 1);
                gpio_set_level(TDI_PIN, 0);
                break;
            case '7':
                // Write 1 1 1
                gpio_set_level(TCK_PIN, 1);
                gpio_set_level(TMS_PIN, 1);
                gpio_set_level(TDI_PIN, 1);
                break;
            case 'r':
                // Reset 0 0
                gpio_set_level(TRST_PIN, 0);
                gpio_set_level(SRST_PIN, 0);
                break;
            case 's':
                // Reset 0 1
                gpio_set_level(TRST_PIN, 0);
                gpio_set_level(SRST_PIN, 1);
                break;
            case 't':
                // Reset 1 0
                gpio_set_level(TRST_PIN, 1);
                gpio_set_level(SRST_PIN, 0);
                break;
            case 'u':
                // Reset 1 1
                gpio_set_level(TRST_PIN, 1);
                gpio_set_level(SRST_PIN, 1);
                break;
            default:
                // error?
                ESP_LOGI(TAG, "unknown command received");
                break;
        }
    }
}

void bitbangServerTask(void *pvParameters)
{
    while (1) {
        // create socket
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE(TAG, "failed to create socket: %d", errno);
            return;
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        // for ipv4
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(TCP_PORT);

        // bind
        {
            int res = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
            if (res != 0) {
                ESP_LOGE(TAG, "failed to bind: %d", errno);
                return;
            }
        }
        
        // listen
        {
            // only 1 queue
            int res = listen(sock, 1);
            if (res != 0) {
                ESP_LOGE(TAG, "failed to listen: %d", errno);
                return;
            }
        }

        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        // accept
        {
            int res = accept(sock, (struct sockaddr *)&client_addr, sizeof(client_addr));
            if (res != 0) {
                ESP_LOGE(TAG, "failed to accept: %d", errno);
                return;
            }
        }

        remote_bitbang_process(sock);
    }
}