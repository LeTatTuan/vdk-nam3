#define BLYNK_TEMPLATE_ID "TMPL6CL2hWjcK"
#define BLYNK_TEMPLATE_NAME "Rain Detection"

#define BLYNK_FIRMWARE_VERSION "1.3.2"
#define BLYNK_PRINT Serial
#define APP_DEBUG
#define USE_NODE_MCU_BOARD

#include "BlynkEdgent.h"
#include <Servo.h>
Servo myServo;
BlynkTimer timer;
int timerID1, timerID2;

int rainValue;
int mucCanhBao = 750;
boolean runMode = 1;                     // 1/0: bật/tắt chế độ cảnh báo
boolean isAlert = 0;                     // 1/0: hoạt động, ko hoạt động
boolean isOpenCeilingCover = 1;          // 1/0: trạng thái mở/đóng màn che
WidgetLED led(V0);

#define BUTTON_WARNING D1                // button tắt bật chế độ cảnh bảo
#define BUTTON_CONTROL_CEILING_COVER D2  // điều khiển màn che thủ công
#define LED_MODE D4                      // Đèn báo hiệu khi chế độ cảnh báo được bật
#define BUZZER D6                        // Loa cảnh báo
#define POWER_PIN_RAIN_SENSOR D7         // Chân cấp nguồn cho cảm biến mưa
#define AO_PIN A0                        // Chân đọc giá trị từ cảm biến mưa 0->1024
#define SERVO_PIN D8                     // Chân điều khiển servo

#define RAINVALUE V1
#define MUCCANHBAO V2
#define RUNMODE V3
#define SERVO V4

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(BUTTON_WARNING, INPUT_PULLUP);
  pinMode(BUTTON_CONTROL_CEILING_COVER, INPUT_PULLUP);
  pinMode(LED_MODE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(POWER_PIN_RAIN_SENSOR, OUTPUT);
  digitalWrite(BUZZER, LOW);  //Tắt buzzer
  digitalWrite(LED_MODE, HIGH);

  myServo.attach(SERVO_PIN);

  attachInterrupt(digitalPinToInterrupt(BUTTON_WARNING), buttonWarningISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_CONTROL_CEILING_COVER), buttonCeilingCoverISR, FALLING);

  BlynkEdgent.begin();  //bắt đầu quá trình kết nối và cấu hình Blynk Edgent, cho phép thiết bị kết nối và làm việc với Blynk IoT Platform.
  timerID1 = timer.setInterval(1000L, handleTimerID1);
  //Sau mỗi 1s thì hàm handleTimerID1 sẽ được gọi
}

void buttonWarningISR() {
  runMode = !runMode;

  digitalWrite(LED_MODE, runMode);
  Serial.println("Run mode: " + String(runMode));
  Blynk.virtualWrite(RUNMODE, runMode);

  if (runMode) {
    Serial.println("Chế độ cảnh báo đang bật.");
  } else {
    Serial.println("Chế độ cảnh báo đang tắt.");
  }
}

void buttonCeilingCoverISR() {
  isOpenCeilingCover = !isOpenCeilingCover;
  controlCeilingCover();
  Blynk.virtualWrite(SERVO, isOpenCeilingCover);
  if (isOpenCeilingCover) {
    Serial.println("Màn chắn đang mở, không có mưa.");
  } else {
    Serial.println("Màn chắn đang đóng, có mưa.");
  }
}
void loop() {
  BlynkEdgent.run();  //để xử lý các sự kiện liên quan đến kết nối và giao tiếp với Blynk IoT Platform,
  timer.run();        //để quản lý và thực hiện các hành động được đặt lịch trình bằng Blynk Timer.
}

void handleTimerID1() {
  digitalWrite(POWER_PIN_RAIN_SENSOR, HIGH);  // cấp nguồn cho rain cảm biến mưa
  delay(10);
  rainValue = analogRead(AO_PIN);
  Serial.print("Rain value: ");
  Serial.println(rainValue);
  digitalWrite(POWER_PIN_RAIN_SENSOR, LOW);
  Blynk.virtualWrite(RAINVALUE, rainValue);

  if (led.getValue()) {
    led.off();
  } else {
    led.on();
  }

  if (runMode == 1) {
    onAlert();
  } else {
    offAlert();
  }
}

void onAlert() {
  if (rainValue < mucCanhBao) {
    if (isAlert == 0) {
      isAlert = 1;
      Blynk.logEvent("canhbao", String("Cảnh báo! Lượng mưa = " + String(rainValue) + " vượt quá mức cho phép!"));
      digitalWrite(BUZZER, HIGH);
      Serial.println("Đã bật cảnh báo!");

      //Thiết lập một timer với ID là timerID2 để kiểm tra lại lượng mưa sau một khoảng thời gian 10 giây (10,000 ms) và gọi hàm handleTimerID2() để xử lý.
      timerID2 = timer.setInterval(10000L, handleTimerID2);
    }

    if (isOpenCeilingCover == 1) {
      for (int pos = 0; pos <= 180; pos += 5) {
        myServo.write(pos);
        delay(0);
      }
      isOpenCeilingCover = 0;
    }

    Blynk.virtualWrite(SERVO, isOpenCeilingCover);
  }
}

void offAlert() {
  digitalWrite(BUZZER, LOW);
  Serial.println("Chế độ cảnh báo đang tắt!");
  digitalWrite(LED_MODE, LOW);
  isAlert = 0;
}
void handleTimerID2() {
  if (rainValue > mucCanhBao) {  // ko mưa
    if (isAlert == 1) {          // nếu cảnh báo đang hoạt động thì
      isAlert = 0;
      digitalWrite(BUZZER, LOW);
      Serial.println("Đã tắt loa cảnh báo!");
    }

    if (isOpenCeilingCover == 0) {  // nếu màn che đang đóng thì mở màn che ra
      isOpenCeilingCover = 1;
      for (int pos = 180; pos >= 0; pos -= 5) {
        myServo.write(pos);
        delay(0);
      }
    }
    Blynk.virtualWrite(SERVO, isOpenCeilingCover);
  }
}

//Được sử dụng để đồng bộ hóa trạng thái của các nút ảo trên ứng dụng Blynk với trạng thái hiện tại của thiết bị.
BLYNK_CONNECTED() {
  Blynk.syncVirtual(RUNMODE, MUCCANHBAO, SERVO);
}

//hàm này được sử dụng để cập nhật giá trị của biến mucCanhbao và in ra giá trị mới đó.
BLYNK_WRITE(MUCCANHBAO) {
  mucCanhBao = param.asInt();
  Serial.println("Mức cảnh báo hiện tại: " + String(mucCanhBao));
}

BLYNK_WRITE(RUNMODE) {
  runMode = param.asInt();
  Serial.print("runMode: ");
  Serial.println(runMode);
  digitalWrite(LED_MODE, LOW);
}

//cập nhật giá trị của biến isOpenCeilingCover dựa trên giá trị của nút ảo SERVO từ ứng dụng Blynk
BLYNK_WRITE(SERVO) {
  isOpenCeilingCover = !param.asInt();
  controlCeilingCover();
}

void controlCeilingCover() {
  if (isOpenCeilingCover == 1) {  // kiểm tra trạng thái của màn che trước khi đóng màn che
    for (int pos = 0; pos <= 180; pos += 5) {
      myServo.write(pos);
      delay(0);
    }
    isOpenCeilingCover = 0;
  } else {  // kiểm tra trạng thái trước khi mở màn che
    for (int pos = 180; pos >= 0; pos -= 5) {
      myServo.write(pos);
      delay(0);
    }
    isOpenCeilingCover = 1;
  }
}