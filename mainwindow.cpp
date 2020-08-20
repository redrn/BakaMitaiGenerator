#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "filedrophandler.h"
#include "ffmpeghandler.h"
#include <QDebug>
#include <QProcess>
#include <QCheckBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialize Variables
    useDefaultTemplate = false;

    // Connect generate button to function
    connect(ui->generateButton, SIGNAL(clicked()), this, SLOT(startGenerating()));

    // Connect Use Default Template button
    connect(ui->defaultDrivingCheckbox, SIGNAL(stateChanged(int)), this, SLOT(defaultTemplateStateChanged(int)));

    // Install eventFilter
    FileDropHandler *filter = new FileDropHandler();
    ui->drivingVideoInput->installEventFilter(filter);
    ui->sourceImageInput->installEventFilter(filter);
    ui->outputDirInput->installEventFilter(filter);
}

void MainWindow::startGenerating()
{
    // Reset Indication
    ui->generateProgressBar->setValue(0);
    ui->statusIndicatorLabel->setText("Status: Generating deepfake...");

    // Read user input
    // Trim the string to get rid of any \r, \n or whitespaces
    QString drivingVideoPath = ui->drivingVideoInput->text().trimmed();
    QString sourceImagePath = ui->sourceImageInput->text().trimmed();
    QString workingDirectoryPath = ui->outputDirInput->text().trimmed();

    // Use default template if checked
    if(useDefaultTemplate)
    {
        drivingVideoPath = "C:\\Users\\RedRN\\Documents\\DamedaneGenerator\\bakamitai_template.mp4";
    }

    // Check if any string is empty
    if(drivingVideoPath.isEmpty() || sourceImagePath.isEmpty() || workingDirectoryPath.isEmpty())
    {
        qDebug() << "One of the inputs is empty!";
        return;
    }

    // Create Process
    demo = new QProcess(this);

    // Set working directory
    demo->setWorkingDirectory(workingDirectoryPath);

    // Setup python and cmd arguments
    // TODO: Use QFile system
    // FIXME: result.mp4 as filename could have unintentional overwrite
    // TODO: Choose unused filename
    QString program = "python.exe";
    QStringList argv;
    argv << "C:\\Users\\RedRN\\Documents\\DamedaneGenerator\\BatchDamedane\\first-order-model\\demo.py"
         << "--config" << "C:\\Users\\RedRN\\Documents\\DamedaneGenerator\\BatchDamedane\\first-order-model\\config\\vox-256.yaml"
         << "--driving_video" << drivingVideoPath
         << "--source_image" << sourceImagePath
         << "--result_video" << workingDirectoryPath.append("\\result.mp4")
         << "--checkpoint" << "C:\\Users\\RedRN\\Documents\\DamedaneGenerator\\vox-cpk.pth.tar"
         << "--relative"
         << "--adapt_scale";

    // Connects signals and slots for output reading
    // This python library uses standard error as output channel for some reasons
    connect(demo, SIGNAL(readyReadStandardError()), this, SLOT(processOutput()));
    demo->setReadChannel(QProcess::StandardError);

    // Start external process
    demo->start(program, argv);

    // Add audio after process finished
    connect(demo, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(generateFinished()));
}

void MainWindow::processOutput()
{
    //FIXME: function not getting called
    if(!demo) return;
    QString output = demo->readAllStandardError().trimmed();

    // Try to extract progress from output
    bool ok;
    output.truncate(output.indexOf("%"));

    // Convert QString to int
    int progress = output.toInt(&ok);
    if(ok)
    {
        ui->generateProgressBar->setValue(progress);
    }
}


// TODO: Handle when process exited with error
void MainWindow::generateFinished()
{
    QString templatePath = ui->drivingVideoInput->text().trimmed();
    QString workingDir = ui->outputDirInput->text().trimmed();
    // FIXME: forward and backward slashes could be different in different scenarios
    QString sourcePath = ui->outputDirInput->text().trimmed().append("\\result.mp4");

    // Use default template if checked
    if(useDefaultTemplate)
    {
        templatePath = "C:\\Users\\RedRN\\Documents\\DamedaneGenerator\\bakamitai_template.mp4";
    }

    // Set status indicator
    ui->statusIndicatorLabel->setText(ui->statusIndicatorLabel->text() + "Done. Remuxing...");

    FFmpegHandler ffmpeg = FFmpegHandler();
    ffmpeg.addTemplateSoundToSource(templatePath, sourcePath, workingDir);

    // Set status indicator
    ui->statusIndicatorLabel->setText(ui->statusIndicatorLabel->text() + "All Done");
}

void MainWindow::defaultTemplateStateChanged(int state)
{
    switch (state)
    {
    case Qt::Checked:
        useDefaultTemplate = true;
        ui->drivingVideoInput->setDisabled(true);
        break;

    case Qt::Unchecked:
        useDefaultTemplate = false;
        ui->drivingVideoInput->setDisabled(false);
        break;

    default:
        useDefaultTemplate = false;
        ui->drivingVideoInput->setDisabled(false);
    }
}

//TODO: Add Batch Generate function
//TODO: Add feature for cropping source image

MainWindow::~MainWindow()
{
    delete ui;
}
