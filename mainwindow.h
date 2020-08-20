#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>

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

private slots:
    void startGenerating();
    void processOutput();
    void generateFinished();
    void defaultTemplateStateChanged(int state);
};
#endif // MAINWINDOW_H
