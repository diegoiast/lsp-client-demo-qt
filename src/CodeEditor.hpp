#pragma once
#include <QPlainTextEdit>
#include <QString>

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget* parent = nullptr);

signals:
    void hoveredWordTooltip(const QString& word, int line, int column, const QPoint& globalPos);

protected:
    bool event(QEvent* e) override;
    QString lastWordHovered;
};
