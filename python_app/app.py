from flask import Flask, render_template, Response, request
import cv2
import serial
import threading
import time
import json
import argparse

app = Flask(__name__)
camera = cv2.VideoCapture(1)  # веб камера

RX, RY = 0, 0  # глобальные переменные положения правого джойстика
LX, LY = 0, 0  # глобальные переменные положения левого джойстика

def getFramesGenerator():
    """ Генератор фреймов для вывода в веб-страницу, тут же можно поиграть с openCV"""
    while True:
        time.sleep(0.01)    # ограничение fps (если видео тупит, можно убрать)
        success, frame = camera.read()  # Получаем фрейм с камеры
        if success:
            # уменьшаем разрешение кадров (если видео тупит, можно уменьшить еще больше)
            frame = cv2.resize(frame, (320, 240), interpolation=cv2.INTER_AREA)
            # frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)   # перевод изображения в градации серого
            # _, frame = cv2.threshold(frame, 127, 255, cv2.THRESH_BINARY)  # бинаризуем изображение
            _, buffer = cv2.imencode('.jpg', frame)
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')

@app.route('/video_feed')
def video_feed():
    """ Генерируем и отправляем изображения с камеры"""
    return Response(getFramesGenerator(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/')
def index():
    """ Крутим html страницу """
    return render_template('index.html')

@app.route('/control')
def control():
    """ Пришел запрос на управление роботом """
    global RX, RY, LX, LY
    joy = request.args.get('joy')
    x = float(request.args.get('x'))
    y = float(request.args.get('y'))
    if joy == "joystick1":
        RX, RY = x, y
    elif joy == "joystick2":
        LX, LY = x, y
    return '', 200, {'Content-Type': 'text/plain'}

if __name__ == '__main__':
    # пакет, посылаемый на робота
    msg = {
        "RX": 0,  # в пакете посылается значения джостиков
        "RY": 0,
        "LX": 0,
        "LY": 0
    }

    # параметры робота
    sendFreq = 10  # слать 10 пакетов в секунду

    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', type=int,
                        default=5000, help="Running port")
    parser.add_argument("-i", "--ip", type=str,
                        default='172.20.10.4', help="Ip address")
    parser.add_argument('-s', '--serial', type=str,
                        default='/dev/ttyS0', help="Serial port")
    args = parser.parse_args()

    serialPort = serial.Serial(args.serial, 9600)   # открываем uart

    def sender():
        """ функция цикличной отправки пакетов по uart """
        global RX, RY, LX, LY
        while True:
            msg["RX"], msg["RY"], msg["LX"], msg["LY"] = RX, RY, LX, LY  # упаковываем

            print(f'Sending: {msg}')  # Отладочное сообщение
            serialPort.write(json.dumps(msg, ensure_ascii=False).encode("utf8"))  # отправляем пакет в виде json файла
            time.sleep(1 / sendFreq)

    def receiver():
        """ функция цикличной обработки пакетов от Arduino """
        while True:
            if serialPort.in_waiting > 0:
                line = serialPort.readline().decode('utf-8').rstrip()
                print(f'Received from Arduino: {line}')

    # запускаем треды отправки и получения пакетов по uart с демоном
    threading.Thread(target=sender, daemon=True).start()
    threading.Thread(target=receiver, daemon=True).start()

    # запускаем flask приложение
    app.run(debug=False, host=args.ip, port=args.port)
