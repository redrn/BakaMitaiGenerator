#ifndef FFMPEGHANDLER_H
#define FFMPEGHANDLER_H

#include <QString>
#include <QProcess>

class FFmpegHandler
{
public:
    FFmpegHandler();

    void addTemplateSoundToSource(QString templatePath, QString SourcePath, QString workingDir);
};

#endif // FFMPEGHANDLER_H
