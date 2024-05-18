#define MAX_SPEED 170   // максимальная скорость моторов (0-255)

#define MOTOR_TEST 0    // тест моторов
// при запуске крутятся ВПЕРЁД по очереди:
// FL - передний левый
// FR - передний правый
// BL - задний левый
// BR - задний правый

// пины драйверов (_B должен быть ШИМ)
#define MOTOR1_IN2 2
#define MOTOR1_IN1 3  // ШИМ!
#define MOTOR2_IN4 4
#define MOTOR2_IN3 5  // ШИМ!
#define MOTOR3_IN2 7
#define MOTOR3_IN1 6  // ШИМ!
#define MOTOR4_IN4 8
#define MOTOR4_IN3 9  // ШИМ!

#include <GyverMotor.h>
// тут можно поменять моторы местами
GMotor motorBL(DRIVER2WIRE, MOTOR1_IN2, MOTOR1_IN1, HIGH);
GMotor motorFL(DRIVER2WIRE, MOTOR2_IN4, MOTOR2_IN3, HIGH);
GMotor motorFR(DRIVER2WIRE, MOTOR3_IN2, MOTOR3_IN1, HIGH);
GMotor motorBR(DRIVER2WIRE, MOTOR4_IN4, MOTOR4_IN3, HIGH);

#include "ArduinoJson.h"
// json буфер для пакетов от orange pi
StaticJsonDocument<200> jsondoc;

void setup() {
  Serial.begin(9600);
  // чуть подразгоним ШИМ https://alexgyver.ru/lessons/pwm-overclock/
  // Пины D3 и D11 - 980 Гц
  TCCR2B = 0b00000100;  // x64
  TCCR2A = 0b00000011;  // fast pwm

  // Пины D9 и D10 - 976 Гц
  TCCR1A = 0b00000001;  // 8bit
  TCCR1B = 0b00001011;  // x64 fast pwm

  // тест моторов
#if (MOTOR_TEST == 1)
  Serial.println("front left");
  motorFL.run(FORWARD, 100);
  delay(3000);
  motorFL.run(STOP);
  delay(1000);
  Serial.println("front right");
  motorFR.run(FORWARD, 100);
  delay(3000);
  motorFR.run(STOP);
  delay(1000);
  Serial.println("back left");
  motorBL.run(FORWARD, 100);
  delay(3000);
  motorBL.run(STOP);
  delay(1000);
  Serial.println("back right");
  motorBR.run(FORWARD, 100);
  delay(3000);
  motorBR.run(STOP);
#endif
  // минимальный сигнал на мотор
  motorFR.setMinDuty(0);
  motorBR.setMinDuty(0);
  motorFL.setMinDuty(0);
  motorBL.setMinDuty(0);

  // режим мотора в АВТО
  motorFR.setMode(AUTO);
  motorBR.setMode(AUTO);
  motorFL.setMode(AUTO);
  motorBL.setMode(AUTO);

  // скорость плавности
  motorFR.setSmoothSpeed(60);
  motorBR.setSmoothSpeed(60);
  motorFL.setSmoothSpeed(60);
  motorBL.setSmoothSpeed(60);
}

void loop() {
  DeserializationError err = deserializeJson(jsondoc, Serial);  // получаем сообщение от orange pi через uart
  if (err == DeserializationError::Ok) {  // если сообщение принято

    int valRX = map((float)jsondoc["RX"], -100, 100, -MAX_SPEED, MAX_SPEED); // переводим диапазон -100..100 в - MAX_SPEED..MAX_SPEED
    int valRY = map((float)jsondoc["RY"], 100, -100, -MAX_SPEED, MAX_SPEED); // инвертируем
    int valLX = map((float)jsondoc["LX"], -100, 100, -MAX_SPEED, MAX_SPEED);
    int valLY = map((float)jsondoc["LY"], 100, -100, -MAX_SPEED, MAX_SPEED); // инвертируем

    int dutyFR = valLY + valLX;
    int dutyFL = valLY - valLX;
    int dutyBR = valLY - valLX;
    int dutyBL = valLY + valLX;

    dutyFR += valRY - valRX;
    dutyFL += valRY + valRX;
    dutyBR += valRY - valRX;
    dutyBL += valRY + valRX;

    // ПЛАВНЫЙ контроль скорости, защита от рывков
    motorFR.smoothTick(dutyFR);
    motorBR.smoothTick(dutyBR);
    motorFL.smoothTick(dutyFL);
    motorBL.smoothTick(dutyBL);
  } else {
    while (Serial.available() > 0) Serial.read(); // чистим буфер
    // страшно, вырубай
    motorFR.setSpeed(0);
    motorBR.setSpeed(0);
    motorFL.setSpeed(0);
    motorBL.setSpeed(0);
  }
  delay(10);
}