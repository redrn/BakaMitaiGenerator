#ifndef FFMPEGHANDLER_H
#define FFMPEGHANDLER_H

#include <QObject>
#include <QString>
#include <QProcess>

class FFmpegHandler : QObject
{
    Q_OBJECT

public:
    explicit FFmpegHandler(QObject *parent = nullptr);

    void addTemplateSoundToSource(QString templatePath, QString SourcePath, QString workingDir);

private slots:
    void handleError(QProcess::ProcessError error);

private:
    QProcess *ffmpeg;
};

#endif // FFMPEGHANDLER_H
