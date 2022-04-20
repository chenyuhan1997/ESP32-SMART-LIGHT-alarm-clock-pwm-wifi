#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "ArduinoJson.h"
//人工语音合成库
#include "SoundData.h"
#include "XT_DAC_Audio.h"
// 引入驱动OLED0.91所需的库
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//舵机使用的不是直接的舵机库
#include <Arduino.h>
#include "time.h"
#include <soc/rtc_cntl_reg.h>
























/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//cpu分配
#define pro_cpu   0   
#define app_cpu   1
//定义的是OLED的长和宽
#define SCREEN_WIDTH 128 // 设置OLED宽度,单位:像素
#define SCREEN_HEIGHT 32 // 设置OLED高度,单位:像素
//宏定义部分引脚
#define buzzer 15        //蜂鸣器
#define dismiss_button 13    //关闭按钮
//cpu分配
#define OLED_RESET 4  //声明RST的引脚
#define Yuyin_IO 25





























/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//人工语音函数
XT_Wav_Class ForceWithYou(rawData);     //引言的XT库中对FORCE的处理                                      
XT_DAC_Audio_Class DacAudio(Yuyin_IO,0);      //设置引脚
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);//ssd设置规范化
//设置类型变量声明
const  char  *ssid = "ccc2021"; //你的网络名称
const  char *password = "cyh1217552389"; //你的网络密码
const  char  *ntpServer = "pool.ntp.org"; //ntpserver的类型
const  long  gmtOffset_sec = 8 * 3600; //声明变量
const int daylightOffset_sec = 0;
//html输入文本

const char* TEXT_INPUT2 = "HTML_INT_INPUT2";// Integer type input

//PWM灯光设置
const int output = 12;// Connected to D2 Pin of ESP32
String sliderValue = "0";//initial Slider value
// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;
const char* PARAM_INPUT = "value";



















/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
AsyncWebServer server(80);//将WEB服务器的端口pnning到80，就是服务器初始化的标准





















/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//定时器详情
static const uint16_t timer_divider = 80;//所有的定时器频率
static uint64_t alarm_timer_exact_count;//实际的闹钟计


















/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//这里的STATIC以及后面的函数，使用多任务进程
//任务处理的程序相关定义，也是声明一些内存任务的意思
static TaskHandle_t clock_task_handler;//主时钟任务
static TaskHandle_t alarm_task_handler;//普通时钟任务
static TaskHandle_t web_server_task_handler;//网页任务
static TaskHandle_t interrupt_set_task_handler;//中断设置任务 
static TaskHandle_t oled_task_handler;//oled网页任务


















/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//定时器程序的相关定义，这里主要是声明的意思
static hw_timer_t* sync_timer_handler = NULL; //固定声明一个定时器
//全局标志位的定义，主要是初始化和相关声明
//闹钟参数
char present_time[6];//存储当前的小时和分钟，当缓冲区
String _date, _time, _date_formatted;//读取数据的字符串
uint32_t DemoCounter=0;
char timeHour[10];
String inputMessage;
String inputParam;
String alarm_time;
String myString;









/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
// HTML web page界面的设置
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
 <head>
   <meta name="viewport" charset="utf-8" content="width=device-width, initial-scale=1">
   <title>ESP32 智能 闹钟</title>
   <style>
     html {font-family: Arial; display: inline-block; text-align: center;}
     h2 {font-size: 2.3rem;}
     p {font-size: 1.9rem;}
     body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
     .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
       outline: none; -webkit-transition: .2s; transition: opacity .2s;}
     .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
     .slider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; } 
   </style>
 </head>
<body>
  <h2><u>ESP32 智能 闹钟</u></h2>
  <p>PWM VALUE = <span id="textSliderValue">%SLIDERVALUE%</span></p>
  <p><input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min="0" max="255" value="%SLIDERVALUE%" step="1" class="slider"></p>
<script>
function updateSliderPWM(element) 
{
  var sliderValue = document.getElementById("pwmSlider").value;
  document.getElementById("textSliderValue").innerHTML = sliderValue;
  console.log(sliderValue);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider?value="+sliderValue, true);
  xhr.send();
}
</script>
  <div style="display: block; margin: 0 auto; width: 50%; background: #ccc;">  
    <form action="/get">
    <br>
      <h2>输入闹钟( XX:XX:XX ) : </h2>
      <input type="text" name="HTML_INT_INPUT2" id = "alarm_inset">
      <input type="submit" value="Submit">
    </form><br>
    <br> 
    <button onclick="showUname()" align="center">显示上一次设定时间</button>
    <div id="uname-show"></div>
    <script>
       function showUname(){
            document.getElementById("uname-show").innerHTML = "<h1>" + document.getElementById("alarm_inset").value + "</h1>";
        }
    </script>
    <br>
    </div>
</body>
</html>
)rawliteral";



void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//实时时间获取
//实时时间获取
//实时时间获取
void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    display.println("Failed to obtain time");
    return;
  }
  display.println(&timeinfo, "%F"); // 格式化输出
  display.println(&timeinfo, "%T"); // 格式化输出
  display.println(&timeinfo, "%A"); // 格式化输出
}

void OLED_Function(void *pvParameters){
  for(;;){
  vTaskDelay(1000);
  //清除屏幕
  display.clearDisplay();
  //设置光标位置
  display.setCursor(0, 0);
  printLocalTime();
  display.display();
  Serial.println("DISPLAY ONCE AGAIN!");
    }
  }







//Replaces placeholder with button section in your web page
String processor(const String& var)
{
  //Serial.println(var);
  if (var == "SLIDERVALUE")
  {
    return sliderValue;
  }
  return String();
}








  
//语音函数
//语音函数
//语音函数
void YuyinBB(){
int forceCounter=0;
for(;;){
  DacAudio.FillBuffer();
  if(ForceWithYou.Playing==false)       // if not playing,
  DacAudio.Play(&ForceWithYou);
  forceCounter++;
  Serial.println(forceCounter);
  if (forceCounter>4500)
      break;
    }
}
















//这个函数是本章节最牛逼的函数，主要的目的是实现时钟显示和读取数据的功能
//从最内层的函数开始查看
//实现时钟显示和读取数据的功能
void clock_task_function(void* parameters)
{
  Serial.println("clock function start!");
  while(1)
  {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
    Serial.println("Failed to obtain time");
    return;
    }
    Serial.println("start print");
    strftime(timeHour, 10, "%T", &timeinfo);
    myString = String(timeHour);
    Serial.println(myString);
    Serial.println("TIME WRITE success!");
    vTaskDelay(1000/portTICK_PERIOD_MS);//这里设置的是无任务时间
  }
}
//修罗场
//修罗场
//修罗场
//这里问题很大
void alarm_task_function(void* parameters)
{
     Serial.println("alarm function start!");
     while(1)
     {
         Serial.println("alarm is setting!");
         vTaskDelay(1000/portTICK_PERIOD_MS);
         if(alarm_time == myString)//这里使用了memcmp函数，这个函数是将两个内存的字节进行比较，当时间校正一致时，才能发生循环
        {
         
          Serial.println("Time Right!");
          while(1)//循环
          {
          
             for(int i=0; i<2; i++)//蜂鸣器循环叫3轮
             {
                digitalWrite(buzzer, HIGH);
                vTaskDelay(500/portTICK_PERIOD_MS);
                digitalWrite(buzzer, LOW);
                vTaskDelay(500/portTICK_PERIOD_MS);
                Serial.println("alarm is sound loundly");
                YuyinBB();
              }

              
             if(!digitalRead(dismiss_button))//如果按了关闭按钮，两个标志位变False,并且串口打印闹钟关闭提醒
             { 
                Serial.println("clock is close!");              
                break;
              }
             vTaskDelay(1000/portTICK_PERIOD_MS);//经典延时
           }                                                                                                    
       }
  }
}
//网页服务
//网页服务
//网页服务
void web_server_task_function(void* paramters)
{
  Serial.println("Setting up the HTTP server");//串口打印设置HTTP服务器
 // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
    Serial.println("HTTP START");
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    if (request->hasParam(TEXT_INPUT2)) {
      inputMessage = request->getParam(TEXT_INPUT2)->value();
      inputParam = TEXT_INPUT2;
      alarm_time = inputMessage;
      Serial.println("alarm clock : ");
    }
    else{
      inputMessage = "No Input Text sent on ESP32";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });

  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sliderValue = inputMessage;
      ledcWrite(ledChannel, sliderValue.toInt());
    }
    else 
    {
      inputMessage = "No message was sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  
  server.begin();//starting the server
  Serial.println("HTTP server setup completed");
  vTaskDelete(NULL);
  
}



void setup() {
  //串口监视器
  Serial.begin(115200);
    while(!Serial) 
  {
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
  vTaskDelay(100/portTICK_PERIOD_MS);
  Serial.println("executing the setup task..");
  Serial.println("('_')7");
  //设置I2C的两个引脚SDA和SCL，这里用到的是17作为SDA，16作为SCL
  Wire.setPins(/*SDA*/21,/*SCL*/22);
  //初始化OLED并设置其IIC地址为 0x3C
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  //清除屏幕
  display.clearDisplay();
  //设置字体颜色,白色可见
  display.setTextColor(WHITE);
  //设置字体大小
  display.setTextSize(1.6);
  //设置光标位置
  display.setCursor(0, 0);
  //wifi设置
  Serial.printf("Connecting to %s ", ssid);

  //configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);  
  //attach the channel to the GPIO to be controlled
  ledcAttachPin(output, ledChannel);
  ledcWrite(ledChannel, sliderValue.toInt());
  
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    vTaskDelay(250/portTICK_PERIOD_MS);
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  Serial.printf("WiFi: %s\n", ssid); 
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  WiFi.mode(WIFI_STA);
 //GPIO初始化
  pinMode(buzzer, OUTPUT);
  pinMode(dismiss_button, INPUT_PULLUP);
  pinMode(Yuyin_IO, OUTPUT);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);//关闭低电压检测,避免无限重启

  //中断源只想cpu0 
  xTaskCreatePinnedToCore(clock_task_function, "clock_task", 2048, NULL, 1, &clock_task_handler, pro_cpu);//clock task
  xTaskCreatePinnedToCore(alarm_task_function, "alarm_task", 2048, NULL, 1, &alarm_task_handler, app_cpu);//alarm task
  xTaskCreatePinnedToCore(web_server_task_function, "server_task", 6000, NULL, 1, &web_server_task_handler, pro_cpu);//web server tasks
  xTaskCreatePinnedToCore(OLED_Function, "oled_Task", 10000, NULL, 1, NULL, app_cpu);
  //NTPClient初始化
  vTaskDelay(100/portTICK_PERIOD_MS);
  vTaskDelete(NULL);//删除主要任务    


  
}


void loop() {

}
