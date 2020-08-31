#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "filedrophandler.h"
#include "ffmpeghandler.h"
#include <QDebug>
#include <QProcess>
#include <QCheckBox>
#include <QStandardPaths>
#include <QImageReader>
#include <QImageWriter>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialize Variables
    useDefaultTemplate = false;
    errorOccured = false;

    // Assign first order model directory
    firstOrderModelDir = QDir("C:\\Users\\RedRN\\Documents\\DamedaneGenerator\\BatchDamedane\\first-order-model");

    // Connect generate button to function
    connect(ui->generateButton, &QPushButton::clicked, this, &MainWindow::startGenerating);

    // Connect Use Default Template button
    connect(ui->defaultDrivingCheckbox, &QCheckBox::stateChanged, this, &MainWindow::defaultTemplateStateChanged);

    // Connect Select File button
    connect(ui->selectFileButton, &QPushButton::clicked, this, &MainWindow::selectFileOpen);

    // Install eventFilter to accept drop event
    FileDropHandler *filter = new FileDropHandler();
    ui->drivingVideoInput->installEventFilter(filter);
    ui->sourceImageInput->installEventFilter(filter);
    ui->outputDirInput->installEventFilter(filter);

    // Drop event also load image
    connect(filter, &FileDropHandler::dropAccepted, [this](QString filepath){
        ui->sourceImageViewLabel->loadImage(QImage(filepath));
    });
}

void MainWindow::startGenerating()
{
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

    // Convert path to QFileInfo and QDir
    // This should handle backward and forward slashes and other potential problems
    // This also makes extra functions available
    drivingVideoFile = QFileInfo(drivingVideoPath);
    sourceImageFile = QFileInfo(sourceImagePath);
    workingDir = QDir(workingDirectoryPath);

    // Reset Indication
    ui->generateProgressBar->setValue(0);
    ui->statusIndicatorLabel->setText("Status: Generating deepfake...");
    errorOccured = false;


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
    argv << firstOrderModelDir.absoluteFilePath("demo.py")
         << "--config" << firstOrderModelDir.absoluteFilePath("config/vox-256.yaml")
         << "--driving_video" << drivingVideoFile.absoluteFilePath()
         << "--source_image" << sourceImageFile.absoluteFilePath()
         << "--result_video" << workingDir.absoluteFilePath(sourceImageFile.baseName() + "_temp_deepfake.mp4")
         << "--checkpoint" << firstOrderModelDir.absoluteFilePath("vox-cpk.pth.tar")
         << "--relative"
         << "--adapt_scale";

    // Connects signals and slots for output reading
    // This python library uses standard error as output channel for some reasons
    connect(demo, SIGNAL(readyReadStandardError()), this, SLOT(processOutput()));
    demo->setReadChannel(QProcess::StandardError);

    // Prepare for error handling
    connect(demo, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(handleError(QProcess::ProcessError)));


    // Start external process
    demo->start(program, argv);

    // Add audio after process finished
    connect(demo, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(generateFinished(int, QProcess::ExitStatus)));
}

void MainWindow::processOutput()
{
    //FIXME: function not getting called
    if(!demo) return;
    QString output = demo->readAllStandardError().trimmed();
    qDebug() << output;

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
void MainWindow::generateFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Error handling
    if(errorOccured)
    {
        qDebug() << demo->errorString();
        return;
    }

    QString sourcePath = workingDir.absoluteFilePath(sourceImageFile.baseName() + "_temp_deepfake.mp4");

    // Set status indicator
    ui->statusIndicatorLabel->setText(ui->statusIndicatorLabel->text() + "Done. Remuxing...");

    FFmpegHandler *ffmpeg = new FFmpegHandler(this);
    ffmpeg->addTemplateSoundToSource(drivingVideoFile.absoluteFilePath(), sourcePath, workingDir.absolutePath());

    // Set status indicator
    ui->statusIndicatorLabel->setText(ui->statusIndicatorLabel->text() + "All Done");
}

// Handle Error from the process
// FIXME: Process does not raise error
// TODO: Use library instead of calling process
void MainWindow::handleError(QProcess::ProcessError error)
{
    errorOccured = true;
    qDebug() << error;
    qDebug() << demo->readAllStandardError();
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

// Select file for open
void MainWindow::selectFileOpen()
{
    QFileDialog dialog(this, "Select source file");
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

    dialog.exec();
    if(dialog.selectedFiles().isEmpty()) return;

    // Set label and FileInfo object
    sourceImageFile = QFile(dialog.selectedFiles().first());
    ui->sourceImageInput->setText(dialog.selectedFiles().first());
    ui->sourceImageViewLabel->loadImage(QImage(dialog.selectedFiles().first()));
    // Set ImageReader

}

// Copied from ImageViewer example from doc.qt.io
void MainWindow::initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    // Setup type filters
    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
        ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    for (const QByteArray &mimeTypeName : supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

MainWindow::~MainWindow()
{
    delete ui;
}
