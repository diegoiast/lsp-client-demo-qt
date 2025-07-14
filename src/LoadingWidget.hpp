#pragma once

// SPD-license: MIT
// Diego Iastrubni diegoiast@gmail.com

#include <QPainter>
#include <QTimer>
#include <QWidget>

class LoadingWidget : public QWidget {
    Q_OBJECT
  public:
    explicit LoadingWidget(QWidget *parent = nullptr);

    void setDuration(int milliseconds);
    int duration() const;
    void start();
    void stop();
    void setLineWidth(int width);

  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    void updatePosition();

    QTimer timer;
    int durationMS = 2000;
    int lineHeight = 2;
    int lineWidth = 40;
    QColor lineColor;
    qreal position = 0;
    qreal velocity = 1.0;
};
