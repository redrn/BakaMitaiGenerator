#include "ffmpeghandler.h"
#include <QStringList>
#include <QDebug>
#include <QFile>
#include <QDir>

FFmpegHandler::FFmpegHandler(QObject *parent) : QObject(parent)
{

}


void FFmpegHandler::addTemplateSoundToSource(QString templatePath, QString sourcePath, QString workingDir)
{
    ffmpeg = new QProcess();

    // FIXME: Doesn't seem to work, still need to manually set working directory
    ffmpeg->setWorkingDirectory(workingDir);

    // Find an output file name that does not exist
    int nameIndex = 1;
    QString output = workingDir + QDir::separator() + "OUTPUT.mp4";
    while(QFile::exists(output))
    {
        output = workingDir + QDir::separator() + "OUTPUT_" + QString::number(nameIndex) + ".mp4";
        nameIndex++;
    }

    // Prepare for error handling
    connect(ffmpeg, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(handleError(QProcess::ProcessError)));

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

// Error Handling
void FFmpegHandler::handleError(QProcess::ProcessError error)
{
    qDebug() << error;
    qDebug() << ffmpeg->readAllStandardError();
}
