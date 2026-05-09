#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_selectFolderBtn_clicked();
    void on_scanBtn_clicked();
    void on_deleteBtn_clicked();

private:
    Ui::MainWindow *ui;
    QString selectedFolder;
    QMap<QString, QStringList> duplicates;
};

#endif // MAINWINDOW_H
