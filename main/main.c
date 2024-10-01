// -------------------------------------------------------------------------
// -------------------- header files ---------------------------------------
// -------------------------------------------------------------------------
#include <stdio.h> 
#include <string.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "freertos/timers.h"
#include "esp_system.h"  
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_log.h" 
#include "esp_event.h" 
#include "nvs_flash.h" 
#include "lwip/err.h" 
#include "lwip/sys.h" 
#include "mqtt_client.h"

// -------------------------------------------------------------------------
// ------------------ Configuration variables ------------------------------
// -------------------------------------------------------------------------
// *******************          WiFi          ******************************
const char *ssid = "YourWiFi";
const char *pass = "yourPasword";
static const char *WiFi = "WiFi Station";

// *******************          MQTT          ******************************
const char *host = "IP and port"; // example "mqtt://192.168.3.125:1883"
const char *topic = "home/fan";
static const char *MQTT = "MQTT Broker";

// *******************          DRIVE          *****************************
static const char *FAN = "DRIVE Fan";
// -Outputs
#define firstSpeed      32
#define secondSpeed     33
#define thirdSpeed      25
#define ligth           26
// -Inputs
#define speedButton     13
#define ligthButton     27

// -------------------------------------------------------------------------
// ------------------ WiFi functions ---------------------------------------
// -------------------------------------------------------------------------
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data){
    int retry_num=0;
    if(event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(WiFi,"WIFI CONNECTING....\n");
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(WiFi,"WiFi CONNECTED\n");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(WiFi,"WiFi lost connection\n");
        if(retry_num<5)
        {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(WiFi,"Retrying to Connect...\n");
        }
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(WiFi,"Wifi got IP...\n\n");
    }
}

void wifi_connection()
{
    esp_netif_init();
    esp_event_loop_create_default();  
    esp_netif_create_default_wifi_sta();                    
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); 
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
        }
    };
    strcpy((char*)wifi_configuration.sta.ssid, ssid);
    strcpy((char*)wifi_configuration.sta.password, pass);    
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_connect();
    printf( "wifi_init_softap finished. SSID:%s  password:%s",ssid,pass);
}

// -------------------------------------------------------------------------
// ------------------ MQTT functions ---------------------------------------
// -------------------------------------------------------------------------
void mqttCommand(int length, char *data);

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe_single(client, "home/fan", 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        mqttCommand(event -> data_len,event -> data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(MQTT, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(MQTT, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(esp_mqtt_client_handle_t client)
{
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

// -------------------------------------------------------------------------
// ------------------ Drive functions --------------------------------------
// -------------------------------------------------------------------------
int fanState = 0b000;
uint8_t ligthState = 0;
TimerHandle_t xTimers;
int timerId = 1;
bool readyToSwitchSpeed = 0;
bool readyToSwitchLigth = 0;
char command[50];

void readyTimerCallback(TimerHandle_t pxTimer)
{
    readyToSwitchSpeed = 1;
    readyToSwitchLigth = 1;
}

esp_err_t initializeGPIO()
{

    gpio_reset_pin(firstSpeed);
    gpio_reset_pin(secondSpeed);
    gpio_reset_pin(thirdSpeed);
    gpio_reset_pin(speedButton);
    gpio_reset_pin(ligthButton);
    gpio_reset_pin(ligth);
    gpio_set_direction(firstSpeed, GPIO_MODE_OUTPUT);
    gpio_set_direction(secondSpeed, GPIO_MODE_OUTPUT);
    gpio_set_direction(thirdSpeed, GPIO_MODE_OUTPUT);
    gpio_set_direction(speedButton, GPIO_MODE_INPUT);
    gpio_set_direction(speedButton, GPIO_MODE_INPUT);
    gpio_set_direction(ligth, GPIO_MODE_OUTPUT);

    gpio_set_pull_mode(ligthButton, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(speedButton, GPIO_PULLUP_ONLY);

    return ESP_OK;
}

void speedChange()
{
    if(readyToSwitchSpeed == 1)
    {
        switch (fanState)
        {
            case 0b000:
            fanState = 0b001;
            gpio_set_level(secondSpeed, 0);
            gpio_set_level(thirdSpeed, 0);
            gpio_set_level(firstSpeed, 1);
            break;
    
        case 0b001:
            fanState = 0b010;
            gpio_set_level(firstSpeed, 0);
            gpio_set_level(thirdSpeed, 0);
            gpio_set_level(secondSpeed, 1);
            break;

        case 0b010:
            fanState = 0b100;
            gpio_set_level(secondSpeed, 0);
            gpio_set_level(firstSpeed, 0);
            gpio_set_level(thirdSpeed, 1);
            break;

        case 0b100:
            fanState = 0b000;
            gpio_set_level(secondSpeed, 0);
            gpio_set_level(thirdSpeed, 0);
            gpio_set_level(firstSpeed, 0);
            break;

        default:
            fanState = 0b000;
            gpio_set_level(secondSpeed, 0);
            gpio_set_level(thirdSpeed, 0);
            gpio_set_level(firstSpeed, 0);
            break;
        }
        readyToSwitchSpeed = 0;
        xTimerReset(xTimers, 0); 
    }   
}

void ligthToggle()
{
    if(readyToSwitchLigth == 1)
    {
        ligthState = !ligthState;
        gpio_set_level(ligth, ligthState);
        readyToSwitchLigth = 0;
        xTimerReset(xTimers, 0);
    } 
}

esp_err_t initializeISR()
{
    gpio_config_t configurationSpeedButton;
    configurationSpeedButton.pin_bit_mask = (1ULL << speedButton);
    configurationSpeedButton.mode = GPIO_MODE_INPUT;
    configurationSpeedButton.pull_up_en = GPIO_PULLUP_ENABLE;
    configurationSpeedButton.pull_down_en = GPIO_PULLDOWN_DISABLE;
    configurationSpeedButton.intr_type = GPIO_INTR_NEGEDGE;

    gpio_config_t configurationLigthButton;
    configurationLigthButton.pin_bit_mask = (1ULL << ligthButton);
    configurationLigthButton.mode = GPIO_MODE_INPUT;
    configurationLigthButton.pull_up_en = GPIO_PULLUP_ENABLE;
    configurationLigthButton.pull_down_en = GPIO_PULLDOWN_DISABLE;
    configurationLigthButton.intr_type = GPIO_INTR_NEGEDGE;

    gpio_config(&configurationLigthButton);
    gpio_config(&configurationSpeedButton);
    gpio_install_isr_service(0);

    gpio_isr_handler_add(speedButton, speedChange, NULL);
    gpio_isr_handler_add(ligthButton, ligthToggle, NULL);

    return ESP_OK;
}

esp_err_t setTimer(void)
{
    printf("Timer initialization configuration\n");
    xTimers = xTimerCreate("Timer",         
                           (pdMS_TO_TICKS(300)), 
                           pdFALSE,          
                           (void *)timerId,  
                           readyTimerCallback
    );

    if (xTimers == NULL)
    {
        printf("Timer not created\n");
    }
    else
    {
        if (xTimerStart(xTimers, 0) != pdPASS)
        {
            printf("Timer not created\n");
        }
    }
    return ESP_OK;
}

void mqttCommand(int length, char *data)
{
    strncpy(command, data, length);
    command[length] = '\0';
    if(strcmp(command, "v1") == 0){
        fanState = 0b000;
        speedChange();
    } else if(strcmp(command, "v2") == 0){
        fanState = 0b001;
        speedChange();
    }
    else if(strcmp(command, "v3") == 0){
        fanState = 0b010;
        speedChange();
    }
    else if(strcmp(command, "v0") == 0){
        fanState = 0b100;
        speedChange();
    }
    else if(strcmp(command, "on") == 0){
        ligthState = 1;
        gpio_set_level(ligth, ligthState);
    }
    else if(strcmp(command, "off") == 0){
        ligthState = 0;
        gpio_set_level(ligth, ligthState);
    }
    else if(strcmp(command, "ligth") == 0){
        ligthState = !ligthState;
        gpio_set_level(ligth, ligthState);
    }
}

// -------------------------------------------------------------------------
// ------------------       Main      --------------------------------------
// -------------------------------------------------------------------------

void app_main(void)
{
    initializeGPIO();
    initializeISR();
    setTimer();
    nvs_flash_init();
    wifi_connection();
    vTaskDelay(8000/portTICK_PERIOD_MS);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = host,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqtt_app_start(client);

    int fanStateAux = 0b000;
    uint8_t ligthStateAux = 0;

    while(1){
        if (fanStateAux != fanState)
        {
            fanStateAux = fanState;
            switch (fanStateAux)
            {
            case 0b000:
                esp_mqtt_client_publish(client, topic, "Fan Off", 0, 0, 0);
                break;
            case 0b001:
                esp_mqtt_client_publish(client, topic, "Fan On. speed: S", 0, 0, 0);
                break;
            case 0b010:
                esp_mqtt_client_publish(client, topic, "Fan On. speed: M", 0, 0, 0);
                break;
            case 0b100:
                esp_mqtt_client_publish(client, topic, "Fan On. speed: H", 0, 0, 0);
                break;
            default:
                ESP_LOGI(FAN, "Unknown fan state\n");
                break;
            }
        }

        if(ligthState != ligthStateAux){
            ligthStateAux = ligthState;
            if(ligthStateAux == 0){
                esp_mqtt_client_publish(client, "home/fan", "Luz apagada", 0, 0, 0);
            } else {
                esp_mqtt_client_publish(client, "home/fan", "Luz encendida", 0, 0, 0);
            }
        }
    }
}