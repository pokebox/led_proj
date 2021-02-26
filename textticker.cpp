#include "textticker.h"

TextTicker::TextTicker(QWidget *parent)
    : QLabel(parent)
{
    m_showText = this->text();

    m_StringWidth = fontMetrics().width(m_showText);
    m_movePx=0;

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TextTicker::updateIndex);
    //timer->start(10);
}


TextTicker::~TextTicker()
{
}

void TextTicker::setColor(QColor color) {
    font_color = color;
}

void TextTicker::setText(QString text) {
    m_showText = text;
    update();
}

void TextTicker::paintEvent(QPaintEvent *)
{
    QPen pen;
    //pen.setStyle(Qt::DashDotDotLine);
    pen.setColor(font_color);

    QPainter painter(this);
    painter.setPen(pen);
    painter.setFont(this->font());
    m_StringWidth = painter.fontMetrics().width(m_showText);

    if (m_StringWidth > this->width()) {
        timer->start(mv_speed);
    }
    else {
        timer->stop();
        m_movePx=this->width();
    }
    painter.drawText(this->width() - m_movePx, this->height()-int(this->height()*0.1), m_showText); //painter.fontMetrics().height()-6
}


void TextTicker::updateIndex()
{
    update();
    m_movePx++;
    if ((m_movePx > width()*2) and (m_movePx > m_StringWidth+width()))
        m_movePx=0;
}
