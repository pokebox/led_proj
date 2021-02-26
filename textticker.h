#ifndef TEXTTICKER_H
#define TEXTTICKER_H

#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QTimer>

class TextTicker : public QLabel
{
    Q_OBJECT


public:
    TextTicker(QWidget *parent = nullptr);
    ~TextTicker();


protected:
    void paintEvent(QPaintEvent *);
    void updateIndex();

private:
    int m_StringWidth;  //整个文本像素宽度
    int m_movePx;       //移动像素
    int mv_speed = 10;  //默认滚动速度
    QString m_showText; //需要显示的字符串
    QColor font_color;
    QTimer *timer;
public:

    QString getText()
    {
        return  m_showText;
    }
    void setSpeed(int speed) {
        mv_speed = speed;
    }

    void setText(QString text);
    void setColor(QColor color);
};

#endif // TEXTTICKER_H
