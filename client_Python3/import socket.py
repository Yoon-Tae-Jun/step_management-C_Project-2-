import sys
import socket
from datetime import datetime
from xml.etree.ElementTree import tostring
from PyQt5 import QtCore
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from PyQt5 import uic
from PyQt5.QtCore import *
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
import calendar


#UI 파일 연결
login_UI = uic.loadUiType("login_UI.ui")[0]
main_UI = uic.loadUiType("main_UI.ui")[0]
client_socket =  socket.socket(socket.AF_INET,socket.SOCK_STREAM)
def exit_program():
    sendString("9")
    client_socket.close()

def sendString(text):
        client_socket.send(text.encode())
        print("send message: "+text)

class Window(QMainWindow, login_UI):

    def __init__(self):
        global client_socket
        super().__init__()
        self.setupUi(self)
        self.Start_Button.clicked.connect(self.f_Start_Button)
        
    def f_Start_Button(self):
        #IP, PORT 입력 받기
        IP = self.Input_IP.text()
        PORT = self.Input_PORT.text()
       
        if self.start_server(IP,PORT) ==1:
            self.Change_main_Window()
        else:
            self.main_Error.setText("접속 에러")
        

    def start_server(self, IP, PORT):
        try:
            client_socket.connect((IP,int(PORT)))
            print('연결된 소켓 정보')
            print(client_socket)
            return 1
        except:
            return 0
        
    def Change_main_Window(self):
        self.hide()
        self.main = main_Window()

    def sendString(self, text):
        client_socket.send(text.encode())
        print("send message: "+text)

    

class main_Window(QMainWindow, main_UI):
    def __init__(self):
        global client_socket
        super().__init__()

        self.setupUi(self)
        #self.setWindowFlag(QtCore.Qt.FramelessWindowHint)
        self.show()
        self.loadItem()
        self.qPixmapVar = QPixmap()
        self.qPixmapVar.load("plot.png")
        self.image1.setPixmap(self.qPixmapVar)
        self.exit.clicked.connect(self.exit_program)
        self.modify_step_bt.clicked.connect(self.modify_Step)
        self.add_step_bt.clicked.connect(self.add_Step)
        
    def sendString(self, text):
        client_socket.send(text.encode())
        print("send message: "+text)
        print()
    
    def recvString(self):
        text = client_socket.recv(1024).decode()
        print("recv message: "+text)
        print()
        return text
    
    def loadItem(self):
        date = self.recvString()
        today = self.recvString()
        week_average = self.recvString()
        total_average = self.recvString()

    
        self.date_text.setText(date)
        self.today_text.setText(today)
        self.week_text.setText(str(int(today)-round(float(week_average))))
        self.total_text.setText(str(int(today)-round(float(total_average))))
        self.plus_date_text.setText(datetime.today().strftime('%Y-%m-%d'))
        
        x = np.arange(3)
        years = ['total average', 'today', 'week average']
        values = [float(total_average),int(today), float(week_average)]
        plt.bar(x, values, color=['r','g','b'], width=0.3, edgecolor='lightgray')
        plt.xticks(x, years)

        #plt.show()
        plt.savefig('plot.png')
        img = Image.open('plot.png')
        img = img.resize((400,300))
        img.save('plot.png')

    def modify_Step(self):    
        dateVar =  self.dateEdit.date()
        date = dateVar.toString("yyyy-MM-dd")
        step = self.modify_step_text.text()
        if step == '':
            self.modify_error.setText("걸음수를 입력하세요")
            return 0
        sendString("5")
        sendString(date)
        i = self.recvString()
        if( i == '1'):
            self.sendString(step)
            self.modify_error.setText("수정 완료")
        else:
            self.modify_error.setText("해당 날짜에 데이터가 존재하지 않습니다.")

        
    def add_Step(self):
        step = self.add_step_text.text()
        if step == '':
            self.add_error.setText("걸음수를 입력하세요")
            return 0
        sendString("6")
        sendString(datetime.today().strftime('%Y-%m-%d'))
        i = self.recvString()
        if i == '1':
            sendString(step)
            self.add_error.setText("추가 완료")
        else:
            self.add_error.setText("이미 오늘 걸음수를 추가했습니다.")
        

    def exit_program(self):
        sendString("9")
        client_socket.close()
        sys.exit(app.exec_())

    def closeEvent(self,event):
        sendString("9")
        client_socket.close()
        event.accept()


#window 시작
app = QApplication(sys.argv)
ex = Window()
ex.show()


sys.exit(app.exec_())