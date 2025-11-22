#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDirIterator>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QFile>
#include <QDebug>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Setup table
    ui->tableWidget->setColumnCount(3);
    QStringList headers;
    headers << "File Name" << "File Path" << "Size (KB)";
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    ui->tableWidget->horizontalHeader()->setVisible(true);
    ui->tableWidget->verticalHeader()->setVisible(true);

    ui->tableWidget->setColumnWidth(0, 200);
    ui->tableWidget->setColumnWidth(1, 400);
    ui->tableWidget->setColumnWidth(2, 100);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Compute MD5 hash of a file
QString getFileHash(const QString &filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return "";

    QByteArray fileData = file.readAll();

    // Normalize line endings for text files (\r\n -> \n)
    fileData.replace("\r\n", "\n");
    fileData.replace("\r", "\n");

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(fileData);
    return hash.result().toHex();
}

// Select folder
void MainWindow::on_selectFolderBtn_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "Select Folder");
    if(folderPath.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No folder selected!");
        return;
    }
    selectedFolder = folderPath;
    QMessageBox::information(this, "Folder Selected", folderPath);
}

// Scan folder in background using QtConcurrent
void MainWindow::on_scanBtn_clicked()
{
    if(selectedFolder.isEmpty()) {
        QMessageBox::warning(this, "Error", "Select a folder first!");
        return;
    }

    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    duplicates.clear();

    // Disable buttons while scanning
    ui->selectFolderBtn->setEnabled(false);
    ui->scanBtn->setEnabled(false);
    ui->deleteBtn->setEnabled(false);

    // Run scan in background thread
    QFuture<void> future = QtConcurrent::run([=](){
        QMap<QString, QStringList> tempDuplicates;
        QDirIterator it(selectedFolder, QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext()) {
            QString filePath = it.next();
            QString hash = getFileHash(filePath);
            qDebug() << "File:" << filePath << "Hash:" << hash;
            if(!hash.isEmpty())
                tempDuplicates[hash].append(filePath);
        }

        // Update GUI in main thread
        QMetaObject::invokeMethod(this, [=](){

            // Debug: check duplicates map before populating table
            qDebug() << "Total unique hashes found:" << tempDuplicates.size();
            for(auto it = tempDuplicates.begin(); it != tempDuplicates.end(); ++it) {
                if(it.value().size() > 1)
                    qDebug() << "Duplicate hash:" << it.key() << "Files:" << it.value();
            }

            int row = 0;
            for(auto it = tempDuplicates.begin(); it != tempDuplicates.end(); ++it) {
                if(it.value().size() > 1) { // only duplicates
                    for(const QString &filePath : it.value()) {
                        QFileInfo fi(filePath);
                        ui->tableWidget->insertRow(row);
                        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(fi.fileName()));
                        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(filePath));
                        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(fi.size()/1024)));
                        row++;
                    }
                }
            }

            duplicates = tempDuplicates;

            // Enable buttons again
            ui->selectFolderBtn->setEnabled(true);
            ui->scanBtn->setEnabled(true);
            ui->deleteBtn->setEnabled(true);

            if(row == 0)
                QMessageBox::information(this, "Result", "No duplicates found!");
            else
                QMessageBox::information(this, "Result", QString::number(row) + " duplicate files found.");
        });
    });
}

// Delete duplicates (keep first file)
void MainWindow::on_deleteBtn_clicked()
{
    if(duplicates.isEmpty()) {
        QMessageBox::warning(this, "Error", "No duplicates to delete!");
        return;
    }

    int deletedCount = 0;
    for(auto it = duplicates.begin(); it != duplicates.end(); ++it) {
        if(it.value().size() > 1) {
            for(int i = 1; i < it.value().size(); i++) {
                if(QFile::remove(it.value()[i]))
                    deletedCount++;
            }
        }
    }

    QMessageBox::information(this, "Deleted", QString::number(deletedCount) + " duplicate files deleted!");
    on_scanBtn_clicked(); // refresh table
}
