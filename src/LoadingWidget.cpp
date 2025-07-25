// SPD-license: MIT
// Diego Iastrubni diegoiast@gmail.com

#include "LoadingWidget.hpp"
#include <QStyle>

LoadingWidget::LoadingWidget(QWidget *parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(lineHeight);

    lineColor = style()->standardPalette().highlight().color();
    connect(&timer, &QTimer::timeout, this, &LoadingWidget::updatePosition);
}

void LoadingWidget::setDuration(int milliseconds) { durationMS = qMax(100, milliseconds); }

int LoadingWidget::duration() const { return durationMS; }

void LoadingWidget::start() {
    timer.start(25);
    position = 0;
    velocity = 5;
}

void LoadingWidget::stop() {
    timer.stop();
    update();
}

void LoadingWidget::setLineWidth(int width) {
    lineWidth = qMax(10, width);
    update();
}

void LoadingWidget::updatePosition() {
    auto maxPosition = width() - lineWidth;
    position += velocity;
    if (velocity > 0) {
        if (position >= maxPosition) {
            position = maxPosition;
            velocity *= -1;
        }
    } else {
        if (position <= 0) {
            position = 0;
            velocity *= -1;
        }
    }
    update();
}

void LoadingWidget::paintEvent(QPaintEvent *event) {
    if (!timer.isActive()) {
        QWidget::paintEvent(event);
        return;
    }

    auto painter = QPainter(this);
    auto yPos = (height() - lineHeight) / 2.0;
    auto lineRect = QRectF(position, yPos, lineWidth, lineHeight);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(lineColor);
    painter.drawRoundedRect(lineRect, lineHeight / 2.0, lineHeight / 2.0);
}
