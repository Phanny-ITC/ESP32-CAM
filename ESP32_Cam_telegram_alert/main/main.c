#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include <esp_http_server.h>
#include "driver/gpio.h"

#include "esp_camera.h"
#include "Camera.h"
#include "Wifi_conection.h"
#include "esp_timer.h"

extern char ip_adr[30];

//Pin connected to a led
//#define LED (GPIO_NUM_2)
#define PIR_Pin 16

/*HTTP buffer*/
#define MAX_HTTP_RECV_BUFFER 1024
#define MAX_HTTP_OUTPUT_BUFFER 2048

#define TELEGRAM_HOST "api.telegram.org"
#define TELEGRAM_SSL_PORT 443
#define BOUNDARY "X-ESPIDF_MULTIPART"

/* TAGs for the system*/
static const char *TAG = "HTTP_CLIENT Handler";
//static const char *TAG1 = "wifi station";
//static const char *TAG2 = "Sending getMe";
static const char *TAG3 = "Sending sendMessage";

/*Telegram configuration*/
#define TOKEN "5121753452:ccEx94Isxhp8Lxcy7dfgzTmk5pIVzcthnNY"
char url_string[512] = "https://api.telegram.org/bot";
// Using in the task strcat(url_string,TOKEN)); the main direct from the url will be in url_string
//The chat id that will receive the message
#define chat_ID1 "@GROUP_NAME"
#define chat_ID2 "-734164003"

/* Root cert for extracted from:
 *
 * https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/blob/master/src/TelegramCertificate.h

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const char telegram_certificate_pem_start[] asm("_binary_telegram_certificate_pem_start");
//extern const char telegram_certificate_pem_end[]   asm("_binary_telegram_certificate_pem_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                output_len = 0;
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
    }
    return ESP_OK;
}

void https_telegram_sendPhoto() {
//	camera_fb_t * fb = NULL;
	char url[512] = "";
    char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};   // Buffer to store response of http request
    esp_http_client_config_t config = {
        .url = "https://api.telegram.org",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .cert_pem = telegram_certificate_pem_start,
		.user_data = output_buffer,
    };
    //POST
    ESP_LOGW(TAG3, "Initialize...");
    esp_http_client_handle_t client = esp_http_client_init(&config);

    /* Creating the string of the url*/
    //Copy the url+TOKEN
    strcat(url,url_string);
    //Passing the method
//    strcat(url,"/sendMessage");
    strcat(url,"/sendPhoto?");
    //ESP_LOGW(TAG3, "url string is: %s",url);
    //You set the real url for the request
    esp_http_client_set_url(client, url);


	ESP_LOGW(TAG3, "Send Photo");
	/*Here you add the text and the chat id
	 * The format for the json for the telegram request is: {"chat_id":123456789,"text":"Here goes the message"}
	  */
	// The example had this, but to add the chat id easierly I decided not to use a pointer
	//const char *post_data = "{\"chat_id\":852596694,\"text\":\"Envio de post\"}";
//	char post_data[512] = "";
	char HEADER[512];
	char header[128];

	sprintf(header, "POST %s HTTP/1.1\r\n", "/upload_multipart");
	strcpy(HEADER, header);
	sprintf(header, "Host: %s:%d\r\n", TELEGRAM_HOST, TELEGRAM_SSL_PORT);
	strcat(HEADER, header);
	sprintf(header, "User-Agent: esp-idf/%d.%d.%d esp32\r\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
	strcat(HEADER, header);
	sprintf(header, "Accept: */*\r\n");
	strcat(HEADER, header);
	sprintf(header, "Content-Type: multipart/form-data; boundary=%s\r\n", BOUNDARY);
	strcat(HEADER, header);

	char BODY[512];
	sprintf(header, "--%s\r\n", BOUNDARY);
	strcpy(BODY, header);
	sprintf(header, "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n");
	strcat(BODY, header);
	strcat(BODY, chat_ID2);
	sprintf(header, "\r\n--%s", BOUNDARY);
	strcpy(BODY, header);
	sprintf(header, "Content-Disposition: form-data; name=\"photo\"; filename=\"%s\"\r\n", "img.jpg");
	strcpy(BODY, header);
	sprintf(header, "Content-Type: image/jpeg\r\n\r\n");
	strcat(BODY, header);

	char END[128];
	sprintf(header, "\r\n--%s--\r\n\r\n", BOUNDARY);
	strcpy(END, header);

//	int dataLength = strlen(BODY) + strlen(END) + statBuf.st_size;
//	sprintf(header, "Content-Length: %d\r\n\r\n", dataLength);
//	strcat(HEADER, header);
//
//    sendPhotoByBinary(chat_ID2, "image/jpeg", fb->len,
//                                isMoreDataAvailable, NULL,
//                                getNextBuffer, getNextBufferLen);
	camera_fb_t * fb = esp_camera_fb_get();
	sprintf(header, "%s",(const char*) fb->buf);
	strcat(BODY, header);
//	sprintf(post_data,"{\"chat_id\":%s,\"photo\":\"%s\"}",chat_ID2,fb->buf);
//	sprintf(post_data,"{\"chat_id\":%s,\"image/jpeg\":\"%s\"}",chat_ID2, Msg);
    //ESP_LOGW(TAG, "El json es es: %s",post_data);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
//    esp_http_client_set_post_write(client, HEADER, strlen(HEADER));
    esp_http_client_set_post_field(client, BODY, strlen(BODY));
//    esp_err_t err = esp_http_client_perform(client);
//    camera_fb_t * fb = esp_camera_fb_get();
//
//    	esp_http_client_write(client,(const char*) fb->buf,fb->len);
    	esp_camera_fb_return(fb);
//    	esp_http_client_set_post_field(client, END, strlen(END));
    	esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG3, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        ESP_LOGW(TAG3, "From Perform the output is: %s",output_buffer);

    } else {
        ESP_LOGE(TAG3, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    ESP_LOGW(TAG, "Close and clean Up Client");
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGI(TAG3, "esp_get_free_heap_size: %d", esp_get_free_heap_size ());
}

void https_telegram_sendMessage_perform_post(char* Msg) {


	/* Format for sending messages
	https://api.telegram.org/bot[BOT_TOKEN]/sendMessage?chat_id=[CHANNEL_NAME]&text=[MESSAGE_TEXT]

	For public groups you can use
	https://api.telegram.org/bot[BOT_TOKEN]/sendMessage?chat_id=@GroupName&text=hello%20world
	For private groups you have to use the chat id (which also works with public groups)
	https://api.telegram.org/bot[BOT_TOKEN]/sendMessage?chat_id=-1234567890123&text=hello%20world

	You can add your chat_id or group name, your api key and use your browser to send those messages
	The %20 is the hexa for the space

	The format for the json is: {"chat_id":852596694,"text":"Message using post"}
	*/

	char url[512] = "";
    char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};   // Buffer to store response of http request
    esp_http_client_config_t config = {
        .url = "https://api.telegram.org",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .cert_pem = telegram_certificate_pem_start,
		.user_data = output_buffer,
    };
    //POST
    ESP_LOGW(TAG3, "Initialize...");
    esp_http_client_handle_t client = esp_http_client_init(&config);

    /* Creating the string of the url*/
    //Copy the url+TOKEN
    strcat(url,url_string);
    //Passing the method
    strcat(url,"/sendMessage");
    //ESP_LOGW(TAG3, "url string es: %s",url);
    //You set the real url for the request
    esp_http_client_set_url(client, url);


	ESP_LOGW(TAG3, "Send POST");
	/*Here you add the text and the chat id
	 * The format for the json for the telegram request is: {"chat_id":123456789,"text":"Here goes the message"}
	  */
	// The example had this, but to add the chat id easierly I decided not to use a pointer
	//const char *post_data = "{\"chat_id\":852596694,\"text\":\"Envio de post\"}";
	char post_data[512] = "";
	sprintf(post_data,"{\"chat_id\":%s,\"text\":\"%s\"}",chat_ID2, Msg);
//	sprintf(post_data,"{\"chat_id\":%s,\"image/jpeg\":\"%s\"}",chat_ID2, Msg);
    //ESP_LOGW(TAG, "El json es es: %s",post_data);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG3, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        ESP_LOGW(TAG3, "From Perform the output is: %s",output_buffer);

    } else {
        ESP_LOGE(TAG3, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    ESP_LOGW(TAG, "Close and clean Up Client");
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGI(TAG3, "esp_get_free_heap_size: %d", esp_get_free_heap_size ());
}

static void http_test_task(void *pvParameters) {
    /* Creating the string of the url*/
    // You concatenate the host with the Token ````so you only have to write the method
	strcat(url_string,TOKEN);
    ESP_LOGW(TAG, "Wait 2 second before start");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    int64_t start_time=0;
    int64_t end_time=0;
    while(true){
//        ESP_LOGI(TAG,"%d",gpio_get_level(PIR_Pin));
        start_time = esp_timer_get_time();
//        ESP_LOGW(TAG3, "Send:%lld",start_time);
        if((gpio_get_level(PIR_Pin)==1) && ((start_time - end_time)/1000000 > 1*60)){//second*60=mn
            ESP_LOGW(TAG, "https_telegram_sendMessage_perform_post");
            ESP_LOGW(TAG3, "Send:%s",ip_adr);
//            https_telegram_sendPhoto();
            https_telegram_sendMessage_perform_post(ip_adr);
            end_time = esp_timer_get_time();



        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
//        ESP_LOGI(TAG, "Finish http example");
    }
    vTaskDelete(NULL);
}
//=======================Camera configuration part==========================================//

static camera_config_t camera_config = {
	    .pin_pwdn  = CAM_PIN_PWDN,
	    .pin_reset = CAM_PIN_RESET,
	    .pin_xclk = CAM_PIN_XCLK,
	    .pin_sscb_sda = CAM_PIN_SIOD,
	    .pin_sscb_scl = CAM_PIN_SIOC,

	    .pin_d7 = CAM_PIN_D7,
	    .pin_d6 = CAM_PIN_D6,
	    .pin_d5 = CAM_PIN_D5,
	    .pin_d4 = CAM_PIN_D4,
	    .pin_d3 = CAM_PIN_D3,
	    .pin_d2 = CAM_PIN_D2,
	    .pin_d1 = CAM_PIN_D1,
	    .pin_d0 = CAM_PIN_D0,
	    .pin_vsync = CAM_PIN_VSYNC,
	    .pin_href = CAM_PIN_HREF,
	    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,   //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
	.fb_location = CAMERA_FB_IN_DRAM,
	.grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    .jpeg_quality = 10, //0-63 lower number means higher quality
    .fb_count = 1       //if more than one, i2s runs in continuous mode. Use only with JPEG
};
static esp_err_t init_camera()
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG0, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

esp_err_t camera_capture(){
    //acquire a frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG0, "Camera Capture Failed");
        return ESP_FAIL;
    }
    //replace this with your own function
//    process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);

    //return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
    return ESP_OK;
}
esp_err_t jpg_stream_httpd_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG0, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if(!jpeg_converted){
                ESP_LOGE(TAG0, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG0, "MJPG: %uKB %ums (%.1ffps)",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port=80;
    config.lru_purge_enable = true;

    httpd_uri_t index_uri = {
      .uri       = "/",
      .method    = HTTP_GET,
      .handler   = jpg_stream_httpd_handler,
      .user_ctx  = NULL
    };
    // Start the httpd server
    ESP_LOGI(TAG0, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG0, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_uri);
        return server;
    }

    ESP_LOGI(TAG0, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

//static void Jpeg_Stream_Server(void *pvParameters) {
//
//
//	vTaskDelay(pdMS_TO_TICKS(10));
//	vTaskDelete(NULL);
//}
static httpd_handle_t server = NULL;
void app_main(void)
{
	esp_timer_init();
	//Config gpio pin for PIR sensor
	gpio_config_t pin_conf;
	pin_conf.mode = GPIO_MODE_INPUT;
	pin_conf.pin_bit_mask = (1ULL << PIR_Pin);
	pin_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	pin_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	pin_conf.intr_type = GPIO_INTR_DISABLE;
	ESP_ERROR_CHECK(gpio_config(&pin_conf));

//	Initialize Camera
	init_camera();

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //Change it the pin that has a led
//	gpio_pad_select_gpio(LED);
//	gpio_set_direction(LED, GPIO_MODE_OUTPUT);
//	gpio_set_level(LED, 1);

    ESP_LOGI(TAG1, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    xTaskCreatePinnedToCore(&http_test_task, "http_test_task", 8192*4, NULL, 5, NULL,1);
//    xTaskCreatePinnedToCore(&Jpeg_Stream_Server, "Jpeg_Stream_Server", 8192, NULL, 5, NULL,0);
    server = start_webserver();

}
