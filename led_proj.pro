#-------------------------------------------------
#
# Project created by QtCreator 2019-05-15T16:38:41
#
#-------------------------------------------------

QT	+= core gui
QT	+= network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = led_proj
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
		mainwindow.cpp \
    textticker.cpp


HEADERS += \
		mainwindow.h \
		textticker.h

FORMS += \
		mainwindow.ui


unix {
message("USE I2C")
DEFINES+=LINUX
SOURCES += dht/common_dht_read.c \
		dht/Raspberry_Pi_2/pi_2_dht_read.c \
		dht/Raspberry_Pi_2/pi_2_mmio.c \
		i2c/bh1750.c \
		i2c/bmp180.c

HEADERS += dht/Raspberry_Pi_2/pi_2_dht_read.h \
        dht/Raspberry_Pi_2/pi_2_mmio.h \
        dht/common_dht_read.h \
        i2c/bh1750.h \
        i2c/bmp180.h
}
else
{
DEFINES+=WINDOWS
}

