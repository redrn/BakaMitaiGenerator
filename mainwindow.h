#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QProcess *demo;
    bool useDefaultTemplate;

    QDir firstOrderModelDir;
    QDir workingDir;
    QFileInfo drivingVideoFile;
    QFileInfo sourceImageFile;

    bool errorOccured;

private slots:
    void startGenerating();
    void processOutput();
    void generateFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void defaultTemplateStateChanged(int state);
    void handleError(QProcess::ProcessError error);
};
#endif // MAINWINDOW_H
