// MainWindow.cpp
#include "MainWindow.h"
#include <QFileDialog>    // Added for QFileDialog
#include <QMessageBox>    // Added for QMessageBox
#include <QDebug>
#include <opencv2/opencv.hpp>
#include <QPixmap>
#include <QImage>
#include <QVBoxLayout>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      scanner(new QRCodeScanner()),
      generator(new QRCodeGenerator()),
      harvester(new DataHarvester()),
      evaluator(new SafetyEvaluator())
{
    qDebug() << "MainWindow constructor started";

    // Set Window Title and Icon
    setWindowTitle("QRGuardPro");
    setWindowIcon(QIcon(":/resources/icons/app_icon.png"));

    // Initialize Central Widget and Layout
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);

    // ----- QR Code Scanning Section -----
    QLabel *scanSectionLabel = new QLabel("<h2>Scan QR Code</h2>", this);
    mainLayout->addWidget(scanSectionLabel);

    scanButton = new QPushButton("Select Image to Scan", this);
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanButtonClicked);
    mainLayout->addWidget(scanButton);

    scannedDataLabel = new QLabel("Scanned Data: ", this);
    mainLayout->addWidget(scannedDataLabel);

    scannedImageLabel = new QLabel(this);
    scannedImageLabel->setFixedSize(300, 300);
    scannedImageLabel->setStyleSheet("border: 1px solid black;");
    mainLayout->addWidget(scannedImageLabel);

    // ----- QR Code Generation Section -----
    QLabel *generateSectionLabel = new QLabel("<h2>Generate QR Code</h2>", this);
    mainLayout->addWidget(generateSectionLabel);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    QLabel *inputLabel = new QLabel("Data: ", this);
    inputDataLineEdit = new QLineEdit(this);
    inputLayout->addWidget(inputLabel);
    inputLayout->addWidget(inputDataLineEdit);
    mainLayout->addLayout(inputLayout);

    generateButton = new QPushButton("Generate QR Code", this);
    connect(generateButton, &QPushButton::clicked, this, &MainWindow::onGenerateButtonClicked);
    mainLayout->addWidget(generateButton);

    generatedImageLabel = new QLabel(this);
    generatedImageLabel->setFixedSize(300, 300);
    generatedImageLabel->setStyleSheet("border: 1px solid black;");
    mainLayout->addWidget(generatedImageLabel);

    // Set Central Widget
    setCentralWidget(centralWidget);

    // Debug Message Box
    QMessageBox::information(this, "Debug", "MainWindow Initialized Successfully");

    qDebug() << "MainWindow constructor finished";
}

MainWindow::~MainWindow() {
    qDebug() << "MainWindow destructor called";
    delete scanner;
    delete generator;
    delete harvester;
    delete evaluator;
}

void MainWindow::onScanButtonClicked() {
    qDebug() << "Scan button clicked";

    QString fileName = QFileDialog::getOpenFileName(this, "Select QR Code Image", "", "Images (*.png *.jpg *.bmp)");
    if(fileName.isEmpty()) {
        qDebug() << "No file selected for scanning.";
        return;
    }

    // Display the selected image
    QPixmap pixmap(fileName);
    pixmap = pixmap.scaled(scannedImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    scannedImageLabel->setPixmap(pixmap);

    // Convert QPixmap to cv::Mat
    cv::Mat image = scanner->qimageToMat(pixmap.toImage());

    // Detect and Decode QR Code
    std::string decodedData = scanner->detectAndDecode(image);
    qDebug() << "Decoded Data:" << QString::fromStdString(decodedData);

    if(decodedData.empty()) {
        scannedDataLabel->setText("Scanned Data: (No QR Code detected)");
    } else {
        scannedDataLabel->setText(QString("Scanned Data: %1").arg(QString::fromStdString(decodedData)));

        // Harvest Data
        std::string harvestedData = harvester->harvest(decodedData);
        qDebug() << "Harvested Data:" << QString::fromStdString(harvestedData);

        // Evaluate Safety
        bool isSafe = evaluator->evaluate(harvestedData); // Ensure 'evaluate' exists
        QString safetyStatus = isSafe ? "Safe" : "Unsafe";
        qDebug() << "Safety Status:" << safetyStatus;

        QMessageBox::information(this, "Safety Evaluation", QString("The scanned URL is %1.").arg(safetyStatus));
    }
}

void MainWindow::onGenerateButtonClicked() {
    qDebug() << "Generate button clicked";

    QString inputData = inputDataLineEdit->text();
    if(inputData.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter data to generate QR Code.");
        qDebug() << "No input data provided for QR Code generation.";
        return;
    }

    try {
        // Generate QR Code
        cv::Mat qrImage = generator->generateQRCode(inputData.toStdString(), 300);
        qDebug() << "QR Code generated successfully.";

        // Convert cv::Mat to QPixmap and display
        QImage qimg = generator->matToQImage(qrImage);
        QPixmap pixmap = QPixmap::fromImage(qimg);
        pixmap = pixmap.scaled(generatedImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        generatedImageLabel->setPixmap(pixmap);

        // Save QR Code Image
        QString savePath = QFileDialog::getSaveFileName(this, "Save QR Code Image", "generated_qrcode.png", "PNG Images (*.png)");
        if(!savePath.isEmpty()) {
            generator->saveQRCode(qrImage, savePath.toStdString());
            QMessageBox::information(this, "Success", "QR Code generated and saved successfully.");
            qDebug() << "QR Code saved to:" << savePath;
        } else {
            qDebug() << "QR Code save path was not provided.";
        }
    }
    catch(const std::exception &e) {
        QMessageBox::critical(this, "Error", QString("Failed to generate QR Code: %1").arg(e.what()));
        qDebug() << "Exception during QR Code generation:" << e.what();
    }
}