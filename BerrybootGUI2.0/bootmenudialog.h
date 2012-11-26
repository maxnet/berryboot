#ifndef BOOTMENUDIALOG_H
#define BOOTMENUDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QModelIndex>

namespace Ui {
class BootMenuDialog;
}
class Installer;

class BootMenuDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit BootMenuDialog(Installer *i, QWidget *parent = 0);
    ~BootMenuDialog();
    virtual bool eventFilter(QObject *obj, QEvent *event);

protected:
    void bootImage(const QString &name);
    void startInstaller();
    void startISCSI();
    void loadModule(const QByteArray &name);
    void initializeA10();

    /* Utility functions copied from old BerrybootUI application */
    QByteArray file_get_contents(const QString &filename);
    void file_put_contents(const QString &filename, const QByteArray &data);
    QByteArray getBootOptions();
    bool mountDataPartition(const QString &dev);
    bool waitForDevice(const QString &dev);
    QByteArray getPartitionByLabel(const QString &label = "berryboot");
    void mountSystemPartition();
    void startNetworking();
    void umountSystemPartition();
    int currentMemsplit();
    int imageNeedsMemsplit(const QString &name);
    QByteArray memsplitParameter(int memsplit);
    bool isRaspberry();
    void reboot();

    Ui::BootMenuDialog *ui;
    Installer *_i;
    int _countdown;
    QTimer _countdownTimer;

protected slots:
    void on_bootButton_clicked();
    void on_settingsButton_clicked();
    void on_list_activated(const QModelIndex &index);
    void autoBootTimeout();
    void stopCountdown();
    void initialize();

};

#endif // BOOTMENUDIALOG_H
