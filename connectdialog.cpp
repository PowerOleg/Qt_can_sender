#include "connectdialog.h"
#include "ui_connectdialog.h"
#include "bitratebox.h"

#include <QCanBus>

ConnectDialog::ConnectDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ConnectDialog)
{
    ui->setupUi(this);
    ui->errorFilterEdit->setValidator(new QIntValidator(0, 0x1FFFFFFFU, this));

    ui->loopbackBox->addItem(tr("unspecified"), QVariant());
    ui->loopbackBox->addItem(tr("false"), QVariant(false));
    ui->loopbackBox->addItem(tr("true"), QVariant(true));
    ui->receiveOwnBox->addItem(tr("unspecified"), QVariant());
    ui->receiveOwnBox->addItem(tr("false"), QVariant(false));
    ui->receiveOwnBox->addItem(tr("true"), QVariant(true));
    ui->canFdBox->addItem(tr("false"), QVariant(false));
    ui->canFdBox->addItem(tr("true"), QVariant(true));

    ui->dataBitrateBox->setFlexibleDateRateEnabled(true);
    connect(ui->okButton, &QPushButton::clicked, this, &ConnectDialog::ok);
    connect(ui->cancelButton, &QPushButton::clicked, this, &ConnectDialog::cancel);
    connect(ui->useConfigurationBox, &QCheckBox::clicked, ui->configurationBox, &QGroupBox::setEnabled);
    connect(ui->pluginListBox, &QComboBox::currentTextChanged, this, &ConnectDialog::pluginChanged);
    connect(ui->interfaceListBox, &QComboBox::currentTextChanged, this, &ConnectDialog::interfaceChanged);
    ui->rawFilterEdit->hide();
    ui->rawFilterLabel->hide();
    ui->pluginListBox->addItems(QCanBus::instance()->plugins());
    updateSettings();
}



ConnectDialog::~ConnectDialog()
{
    delete ui;
}



void ConnectDialog::updateSettings()
{
    m_currentSettings.pluginName = ui->pluginListBox->currentText();
    m_currentSettings.deviceInterfaceName = ui->interfaceListBox->currentText();
    m_currentSettings.useConfigurationEnabled = ui->useConfigurationBox->isChecked();
    if (m_currentSettings.useConfigurationEnabled)
    {
        m_currentSettings.configurations.clear();
        // process LoopBack
        if (ui->loopbackBox->currentIndex() != 0)
        {
            ConfigurationItem item;
            item.first = QCanBusDevice::LoopbackKey;
            item.second = ui->loopbackBox->currentData();
            m_currentSettings.configurations.append(item);
        }
        // process ReceiveOwnKey
        if (ui->receiveOwnBox->currentIndex() != 0)
        {
            ConfigurationItem item;
            item.first = QCanBusDevice::ReceiveOwnKey;
            item.second = ui->receiveOwnBox->currentData();
            m_currentSettings.configurations.append(item);
        }
        // process error filter
        if (!ui->errorFilterEdit->text().isEmpty())
        {
            QString value = ui->errorFilterEdit->text();
            bool ok = false;
            int dec = value.toInt(&ok);
            if (ok)
            {
                ConfigurationItem item;
                item.first = QCanBusDevice::ErrorFilterKey;
                item.second = QVariant::fromValue(QCanBusFrame::FrameErrors(dec));
                m_currentSettings.configurations.append(item);
            }
        }
        // process raw filter list
        if (!ui->rawFilterEdit->text().isEmpty())
        {
        //191025_TODO current ui not sfficient to reflect this param
        }


        // process bitrate
        const int bitrate = ui->bitrateBox->bitRate();
        if (bitrate > 0)
        {
            const ConfigurationItem item(QCanBusDevice::BitRateKey, QVariant(bitrate));
            m_currentSettings.configurations.append(item);
        }

        // process CAN FD setting
        ConfigurationItem fdItem;
        fdItem.first = QCanBusDevice::CanFdKey;
        fdItem.second = ui->canFdBox->currentData();
        m_currentSettings.configurations.append(fdItem);

        // process data bitrate
        const int dataBitrate = ui->dataBitrateBox->bitRate();
        if (dataBitrate > 0)
        {
            const ConfigurationItem item(QCanBusDevice::DataBitRateKey, QVariant(dataBitrate));
            m_currentSettings.configurations.append(item);
        }
    }
}

void ConnectDialog::revertSettings()
{
    ui->pluginListBox->setCurrentText(m_currentSettings.pluginName);
    ui->interfaceListBox->setCurrentText(m_currentSettings.deviceInterfaceName);
    ui->useConfigurationBox->setChecked(m_currentSettings.useConfigurationEnabled);
    QString value = configurationValue(QCanBusDevice::LoopbackKey);
    ui->loopbackBox->setCurrentText(value);
    value = configurationValue(QCanBusDevice::ReceiveOwnKey);
    ui->receiveOwnBox->setCurrentText(value);
    value = configurationValue(QCanBusDevice::ErrorFilterKey);
    ui->errorFilterEdit->setText(value);
    value = configurationValue(QCanBusDevice::BitRateKey);
    ui->bitrateBox->setCurrentText(value);
    value = configurationValue(QCanBusDevice::CanFdKey);
    ui->canFdBox->setCurrentText(value);
    value = configurationValue(QCanBusDevice::DataBitRateKey);
    ui->dataBitrateBox->setCurrentText(value);
}

QString ConnectDialog::configurationValue(QCanBusDevice::ConfigurationKey key)
{
    QVariant result;
    for (const ConfigurationItem &item : qAsConst(m_currentSettings.configurations))
    {
        if (item.first == key)
        {
            result = item.second;
            break;
        }
    }
    if (result.isNull() && (
        key == QCanBusDevice::LoopbackKey ||
        key == QCanBusDevice::ReceiveOwnKey))
    {
        return tr("unspecified");
    }
    return result.toString();
}

void ConnectDialog::ok()
{
    updateSettings();
    accept();
}
void ConnectDialog::cancel()
{
    revertSettings();
    reject();
}

void ConnectDialog::pluginChanged(const QString &plugin)
{
    ui->interfaceListBox->clear();
    m_interfaces = QCanBus::instance()->availableDevices(plugin);
    for (const QCanBusDeviceInfo &info : qAsConst(m_interfaces))
    ui->interfaceListBox->addItem(info.name());
}

void ConnectDialog::interfaceChanged(const QString &interface)
{
    ui->isVirtual->setChecked(false);
    ui->isFlexibleDataRateCapable->setChecked(false);
    for (const QCanBusDeviceInfo &info : qAsConst(m_interfaces))
    {
        if (info.name() == interface)
        {
            ui->descriptionLabel->setText(info.description());
            QString serialNumber = info.serialNumber();
            if (serialNumber.isEmpty())
                serialNumber = tr("n/a");
            ui->serialNumberLabel->setText(tr("Serial: %1").arg(serialNumber));
            ui->channelLabel->setText(tr("Channel: %1").arg(info.channel()));
            ui->isVirtual->setChecked(info.isVirtual());
            ui->isFlexibleDataRateCapable->setChecked(info.hasFlexibleDataRate());
            break;
        }
    }
}

ConnectDialog::Settings ConnectDialog::settings() const
{
    return m_currentSettings;
}
