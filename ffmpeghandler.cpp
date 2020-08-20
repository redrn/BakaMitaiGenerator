#include "ffmpeghandler.h"
#include <QStringList>
#include <QDebug>
#include <QFile>

FFmpegHandler::FFmpegHandler()
{

}

void FFmpegHandler::addTemplateSoundToSource(QString templatePath, QString sourcePath, QString workingDir)
{
    QProcess *ffmpeg = new QProcess();

    // FIXME: Doesn't seem to work, still need to manually set working directory
    ffmpeg->setWorkingDirectory(workingDir);

    // Find an output file name that does not exist
    int nameIndex = 1;
    QString output = workingDir + "\\OUTPUT.mp4";
    while(QFile::exists(output))
    {
        output = workingDir + "\\OUTPUT_" + QString::number(nameIndex) + ".mp4";
        nameIndex++;
    }

    // Combine audio from template and video from source
    ffmpeg->start("ffmpeg", QStringList() << "-i" << templatePath << "-i" << sourcePath
                                            << "-map" << "0:a" << "-map" << "1:v"
                                            << "-c:v" << "copy" << "-c:a" << "copy"
                                            << "-n" // Aborts upon output name collision
                                            << output );
    ffmpeg->waitForFinished();

    // Remove source, which only serves as temporary file
    QFile::remove(sourcePath);
}
