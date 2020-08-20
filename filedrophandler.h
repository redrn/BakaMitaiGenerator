#ifndef FILEDROPHANDLER_H
#define FILEDROPHANDLER_H

#include <QObject>
#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QLineEdit>
#include <QUrl>

class FileDropHandler : public QObject
{
    Q_OBJECT
public:
    explicit FileDropHandler(QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // FILEDROPHANDLER_H
