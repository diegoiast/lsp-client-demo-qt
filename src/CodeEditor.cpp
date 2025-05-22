#include "CodeEditor.hpp"
#include <QHelpEvent>
#include <QTextCursor>
#include <QToolTip>

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setMouseTracking(true);
}

bool CodeEditor::event(QEvent* e)
{
    if (e->type() == QEvent::ToolTip) {
        auto helpEvent = static_cast<QHelpEvent*>(e);
        auto cursor = cursorForPosition(helpEvent->pos());
        cursor.select(QTextCursor::WordUnderCursor);
        auto word = cursor.selectedText();
        
        if (lastWordHovered != word) {
            auto line = cursor.blockNumber();
            auto col = cursor.positionInBlock();
            lastWordHovered = word;
            emit hoveredWordTooltip(word, line, col, helpEvent->globalPos());
/*    
            if (!word.isEmpty())
                QToolTip::showText(helpEvent->globalPos(), QString("'%1' (Line %2, Col %3)").arg(word).arg(line).arg(col), this);
            else
                QToolTip::hideText();
*/  
            return true;
        }
    }
    return QPlainTextEdit::event(e);
}
