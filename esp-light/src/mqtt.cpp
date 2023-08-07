/*
 * 智能语言控制控制，支持同时天猫、小爱、小度、google Assistent控制
 * 也同时支持web控制、小程序控制、app控制，定时控制等
 * QQ群：566565915
 * 项目示例：通过发送on或off控制开关
 * 官网：bemfa.com
 */

#include <ESP8266WiFi.h>  //默认，加载WIFI头文件
#include <PubSubClient.h> //默认，加载MQTT库文件
#include "led.h"
//********************需要修改的部分*******************//
#define ID_MQTT "2a9fbf504dbf3497645c8310f48e9618" // 用户私钥，控制台获取
// 客厅灯
const char *topic = "led3002";                    // 主题名字，可在巴法云控制台自行创建，名称随意
//**************************************************//

const char *mqtt_server = "bemfa.com"; // 默认，MQTT服务器
const int mqtt_server_port = 9501;     // 默认，MQTT服务器
WiFiClient espClient;
PubSubClient client(espClient);


String msg = "";
void callback(char *topic, byte *payload, unsigned int length)
{
    payload[length] = '\0';
    

    String msg = (const char*)payload;
    String status = "";
    String bright = "";
    int splitIndex = msg.indexOf("#");
    if (splitIndex == -1) {
        status = msg;
    } else {
        status = msg.substring(0,splitIndex);
        bright = msg.substring(splitIndex + 1);
        splitIndex = bright.indexOf("#");
        if (splitIndex != -1) {
            bright = bright.substring(0,splitIndex);
        }
    }

    Serial.printf("Topic: %s msg: %s\n",topic,msg.c_str());
    // Serial.printf("status: %s bright: %s\n",status.c_str(),bright.c_str());
    
    if (bright != "") {
        int val = bright.toInt();
        if (val > 0) {
            changeBright(val);
            return;
        }
    }
    switchLed(status == "on");
}

time_task_t mqtt_task;

void mqtt_connect_task(time_task_list_t *q, time_task_node_t *node, time_t timestamp)
{
    if (!client.connected() && WiFi.isConnected())
    {
        if (client.connect(ID_MQTT))
        {
            Serial.println("connected");
            Serial.print("subscribe:");
            Serial.println(topic);
            // 订阅主题，如果需要订阅多个主题，可发送多条订阅指令client.subscribe(topic2);client.subscribe(topic3);
            client.subscribe(topic);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
        }
    }
}

void mqtt_setup()
{
    client.setServer(mqtt_server, mqtt_server_port); // 设置mqtt服务器
    client.setCallback(callback);                    // mqtt消息处理
    mqtt_task.cb = mqtt_connect_task;
    mqtt_task.last_time = 0;
    mqtt_task.flag = TASK_LOOP_ON;
    mqtt_task.interval = 5000;

    time_task_list_add(&iot_tasks, &mqtt_task);
}
void mqtt_loop()
{
    yield();
    client.loop();
}
