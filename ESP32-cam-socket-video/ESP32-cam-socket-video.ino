#include "secrets.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#define CAMERA_MODEL_AI_THINKER
#include <stdio.h>
#include "camera_pins.h"

const char* ssid = WIFI_SSID; // from secrets.h
const char* password = WIFI_PASSWORD;
const char* websocket_server_host = REMOTE_WS_SERVER_URL;
const uint16_t websocket_server_port1 = 8080;

using namespace websockets;
WebsocketsClient client;

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connection Closed");
        ESP.restart();
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}

void onMessageCallback(WebsocketsMessage message) {
    String data = message.data();
    int index = data.indexOf("=");
    if(index != -1) {
        String key = data.substring(0, index);
        String value = data.substring(index + 1);

        if(key == "ON_BOARD_LED_1") {
          // if(value.toInt() == 1) {
          //   flashlight = 1;
          //   digitalWrite(FLASH_PIN, HIGH);
          // } else {
          //   flashlight = 0;
          //   digitalWrite(FLASH_PIN, LOW);
          // }
        }
        
        Serial.print("Key: ");
        Serial.println(key);
        Serial.print("Value: ");
        Serial.println(value);
    }
}

void sendCameraData(camera_fb_t* fb, const char* camera_id) {
  // Calculate the total size: camera_id length + image data length
  size_t camera_id_len = strlen(camera_id);
  size_t total_size = camera_id_len + fb->len;

  // Create a buffer to hold both the camera_id and the image data
  char* combined_buffer = new char[total_size];

  // Copy the camera_id into the buffer
  memcpy(combined_buffer, camera_id, camera_id_len);

  // Copy the image data into the buffer right after the camera_id
  memcpy(combined_buffer + camera_id_len, fb->buf, fb->len);

  // Send the combined buffer as a single binary message
  client.sendBinary(combined_buffer, total_size);

  // Clean up
  delete[] combined_buffer;
}

void setup() 
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 63; // 0 - 63 best to worst quality
  config.fb_count = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) { return; }

  sensor_t * s = esp_camera_sensor_get();

  s->set_contrast(s, 0);   
  s->set_raw_gma(s, 1);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  // do something when connection fails
  
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  Serial.begin(115200);
  while(!client.connect(websocket_server_host, websocket_server_port1, "/")) { delay(500); }
}

void loop() 
{
  client.poll(); // maintain a websocket connection
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb)
  {
    esp_camera_fb_return(fb);
    return;
  }

  // if(fb->format != PIXFORMAT_JPEG) { return; }

  const char* camera_id = "CAM2";

  // Send the byte array as binary data
  // client.sendBinary((const char*)camera_id, strlen(camera_id));
  // client.sendBinary((const char*) fb->buf, fb->len);

  // Send the camera ID and the image frame
  sendCameraData(fb, camera_id);

  esp_camera_fb_return(fb);
}
