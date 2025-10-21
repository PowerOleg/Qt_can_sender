#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include <QCanBusDevice>
#include <QCanBusDeviceInfo>

namespace Ui
{
class ConnectDialog;
}

class ConnectDialog : public QDialog
{
    Q_OBJECT
public:
    typedef QPair<QCanBusDevice::ConfigurationKey, QVariant> ConfigurationItem;
    struct Settings
    {
        QString pluginName;
        QString deviceInterfaceName;
        QList<ConfigurationItem> configurations;
        bool useConfigurationEnabled = false;
    };

    explicit ConnectDialog(QWidget *parent = 0);
    ~ConnectDialog();

    Settings settings() const;

private slots:
    void pluginChanged(const QString &plugin);
    void interfaceChanged(const QString &interface);
    void ok();
    void cancel();
private:
    Ui::ConnectDialog *ui = nullptr;
    QString configurationValue(QCanBusDevice::ConfigurationKey key);
    void revertSettings();
    void updateSettings();
    Settings m_currentSettings;
    QList<QCanBusDeviceInfo> m_interfaces;
};

#endif // CONNECTDIALOG_H
