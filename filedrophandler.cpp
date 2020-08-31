#include "filedrophandler.h"

FileDropHandler::FileDropHandler(QObject *parent) : QObject(parent)
{

}

bool FileDropHandler::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::DragEnter)
    {
        QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent*>(event);
        if(dragEvent->mimeData()->hasUrls())
        {
            dragEvent->acceptProposedAction();
            return true;
        }
    }
    if(event->type() == QEvent::Drop)
    {
        QDropEvent *e = static_cast<QDropEvent*>(event);
        if(e->mimeData()->hasUrls())
        {
            QLineEdit *le = static_cast<QLineEdit*>(watched);
            QString filepath = e->mimeData()->urls().first().toLocalFile();
            le->setText(filepath);
            emit dropAccepted(filepath);
            return true;
        }
    }

    return QObject::eventFilter(watched, event);
}
