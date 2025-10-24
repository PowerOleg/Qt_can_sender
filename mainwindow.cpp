#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectdialog.h"

#include <QCanBus>
#include <QCanBusFrame>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QTimer>

void MainWindow::initChart()
{
    m_series = new QSplineSeries();
    m_chart = new QChart();
    m_chart->legend()->hide();
    m_chart->setTitle("Инерционное изменение отображаемой температуры");
    m_chart->addSeries(m_series);
    m_ui->chartView->setChart(m_chart);
    m_ui->chartView->setRenderHint(QPainter::Antialiasing);

    QValueAxis *axisX = new QValueAxis();
    axisX->setRange(0, 60);
    axisX->setTickCount(5);
    axisX->setLabelFormat("%g");//axisX->setLabelFormat("%.2f");
    axisX->setLineVisible();
    axisX->setTitleText("Время (секунды)");
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(-100, 100);
    axisY->setTickCount(11);
    axisY->setLabelFormat("%g");
    axisY->setTitleText("Температура (℃)");
    m_chart->setAxisX(axisX, m_series);
    m_chart->setAxisY(axisY, m_series);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    m_connectDialog = new ConnectDialog;
    m_status = new QLabel;
    m_ui->statusBar->addPermanentWidget(m_status);
    m_written = new QLabel;
    m_status->setText("Please set connection");
    m_ui->statusBar->addWidget(m_written);
    m_temperatureTimer = new QTimer(this);
    initActionsConnections();
//    QTimer::singleShot(50, m_connectDialog, &ConnectDialog::show);//no need
    m_ui->warningBox->setVisible(false);
    initChart();
}

MainWindow::~MainWindow()
{
    delete m_canDevice;
    delete m_connectDialog;
    delete m_ui;
}

void MainWindow::initActionsConnections()
{
    m_ui->actionDisconnect->setEnabled(false);
    connect(m_ui->actionConnect, &QAction::triggered, m_connectDialog, &ConnectDialog::show);
    connect(m_connectDialog, &QDialog::accepted, this, &MainWindow::connectDevice);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow::disconnectDevice);
    connect(m_ui->actionQuit, &QAction::triggered, this, &QWidget::close);
    connect(m_ui->actionClearLog, &QAction::triggered, m_ui->receivedMessagesEdit, &QTextEdit::clear);
    connect(m_temperatureTimer, &QTimer::timeout, this, &MainWindow::adjustTemperatureValue);
}

void MainWindow::processErrors(QCanBusDevice::CanBusError error) const
{
    switch (error)
    {
        case QCanBusDevice::ReadError:
        case QCanBusDevice::WriteError:
        case QCanBusDevice::ConnectionError:
        case QCanBusDevice::ConfigurationError:
        case QCanBusDevice::UnknownError:
            m_status->setText(m_canDevice->errorString());
            break;
        default:
            break;
    }
}

void MainWindow::connectDevice()
{
    const ConnectDialog::Settings p = m_connectDialog->settings();
    QString errorString;
    m_canDevice = QCanBus::instance()->createDevice(p.pluginName,
                                                    p.deviceInterfaceName,
                                                    &errorString);//("socketcan", "vcan0");

    if (!m_canDevice)
    {
        m_status->setText(tr("Error creating device '%1', reason: '%2'").arg(p.pluginName).arg(errorString));
        return;
    }
    m_numberFramesWritten = 0;
    connect(m_canDevice, &QCanBusDevice::framesWritten, this, &MainWindow::processFramesWritten);
    connect(m_canDevice, &QCanBusDevice::errorOccurred, this, &MainWindow::processErrors);
    connect(m_canDevice, &QCanBusDevice::framesReceived, this, &MainWindow::processReceivedFrames);

    if (p.useConfigurationEnabled)
    {
        for (const ConnectDialog::ConfigurationItem &item : p.configurations)
            m_canDevice->setConfigurationParameter(item.first, item.second);
    }

    if (!m_canDevice->connectDevice())
    {
        m_status->setText(tr("Connection error: %1").arg(m_canDevice->errorString()));
        delete m_canDevice;
        m_canDevice = nullptr;
    }
    else
    {
        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);
//        m_ui->sendFrameBox->setEnabled(true);
        const QVariant bitRate = m_canDevice->configurationParameter(QCanBusDevice::BitRateKey);
        if (bitRate.isValid())
        {
            const bool isCanFd = m_canDevice->configurationParameter(QCanBusDevice::CanFdKey).toBool();
            const QVariant dataBitRate = m_canDevice->configurationParameter(QCanBusDevice::DataBitRateKey);
            if (isCanFd && dataBitRate.isValid())
            {
                m_status->setText(tr("Plugin: %1, connected to %2 at %3 / %4 kBit/s")
                .arg(p.pluginName).arg(p.deviceInterfaceName)
                .arg(bitRate.toInt() / 1000).arg(dataBitRate.toInt() / 1000));
            }
            else
            {
                m_status->setText(tr("Plugin: %1, connected to %2 at %3 kBit/s").arg(p.pluginName).arg(p.deviceInterfaceName)
                .arg(bitRate.toInt() / 1000));
            }
        }
        else
        {
            m_status->setText(tr("Plugin: %1, connected to %2").arg(p.pluginName).arg(p.deviceInterfaceName));
        }
    }
}

void MainWindow::disconnectDevice()
{
    if (!m_canDevice)
        return;
    m_canDevice->disconnectDevice();
    delete m_canDevice;
    m_canDevice = nullptr;
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
//    m_ui->sendFrameBox->setEnabled(false);
    m_status->setText(tr("Disconnected"));
}

void MainWindow::processFramesWritten(qint64 count)
{
    m_numberFramesWritten += count;
    m_written->setText(tr("%1 frames written").arg(m_numberFramesWritten));
}



void MainWindow::closeEvent(QCloseEvent *event)
{
    m_connectDialog->close();
    event->accept();
}

static QString frameFlags(const QCanBusFrame &frame)
{
    QString result = QLatin1String(" --- ");
    if (frame.hasBitrateSwitch())
        result[1] = QLatin1Char('B');
    if (frame.hasErrorStateIndicator())
        result[2] = QLatin1Char('E');
    if (frame.hasLocalEcho())
        result[3] = QLatin1Char('L');
    return result;
}

void MainWindow::processReceivedFrames()
{
    if (!m_canDevice)
        return;
    while (m_canDevice->framesAvailable())
    {
        const QCanBusFrame frame = m_canDevice->readFrame();
        QString view;
        if (frame.frameType() == QCanBusFrame::ErrorFrame)
            view = m_canDevice->interpretErrorFrame(frame);
        else
        {
            auto frameId = frame.frameId();
            QByteArray payload = frame.payload();
            view = frame.toString();

            const QString time = QString::fromLatin1("%1.%2 ")//211025
            .arg(frame.timeStamp().seconds(), 10, 10, QLatin1Char(' '))
            .arg(frame.timeStamp().microSeconds() / 100, 4, 10, QLatin1Char('0'));
            const QString flags = frameFlags(frame);
            m_ui->receivedMessagesEdit->append(time + flags + view);

            if (frameId == TEMPERATURE_FRAME_ID && !payload.isEmpty())
            {
                int temperature = static_cast< uint8_t >(payload[0]);
                if (!isInitSensor(TEMPERATURE_FRAME_ID, temperature))
                {
                    setTemperature(temperature);
                }
            }
            if (frameId == HUMIDITY_FRAME_ID && !payload.isEmpty())
            {
                int humidity = static_cast< uint8_t >(payload[0]);
                if (!isInitSensor(HUMIDITY_FRAME_ID, humidity))
                {
                    setHumidity(humidity);
                }
            }

        }

    }
}

bool MainWindow::isInitSensor(int frameId, int value)
{
    if (value == 255)
    {
        sendFrame(frameId, m_sensorInitValue);
        return true;
    }
    return false;
}

void MainWindow::setTemperature(const int temperature)
{
    int oldTemperature = m_ui->temperatureSpinBox->value();
    int newTemperature = (temperature - 100) < -100 ? -100 : temperature - 100;
    newTemperature = newTemperature > 100 ? 100 : newTemperature;
    m_temperatureTargetValue = newTemperature;

    if (qAbs(temperature - (oldTemperature + 100)) >= 30)
        m_temperatureTimer->start(500);
    else
        m_ui->temperatureSpinBox->setValue(newTemperature);
}

void MainWindow::adjustTemperatureValue()
{
    if (qAbs((m_ui->temperatureSpinBox->value() + 100) - (m_temperatureTargetValue + 100)) < 30)
    {
        m_temperatureTimer->stop();
        m_ui->temperatureSpinBox->setValue(m_temperatureTargetValue);
    }
    else
    {
        int oldTemperature = m_ui->temperatureSpinBox->value();
        int delta = 5;
        if (m_temperatureTargetValue > oldTemperature)
        {
            m_ui->temperatureSpinBox->setValue(oldTemperature + delta);
        }
        else
        {
            m_ui->temperatureSpinBox->setValue(oldTemperature - delta);
        }
    }
}

void MainWindow::setHumidity(const int humidity)
{
    int newHumidity = humidity < 0 ? 0 : humidity;
    newHumidity = newHumidity > 100 ? 100 : newHumidity;
    m_ui->humiditySpinBox->setValue(newHumidity);
}

void MainWindow::on_sendButton_clicked()
{
    if (!m_canDevice)
        return;
    const bool hasTemperature = !m_ui->temperatureSpinBox->text().isEmpty();
    if (hasTemperature)
    {
        int temperatureValue = m_ui->temperatureSpinBox->value();
        QString temperatureHexValue = QString("%1").arg(temperatureValue + 100, 2, 16, QLatin1Char( '0' ));
        m_frameIds[TEMPERATURE_FRAME_ID] = temperatureHexValue;
    }

    const bool hasHumidity = !m_ui->humiditySpinBox->text().isEmpty();
    if (hasHumidity)
    {
        QString temperatureHexValue = QString("%1").arg(m_ui->humiditySpinBox->value(), 2, 16, QLatin1Char( '0' ));
        m_frameIds[HUMIDITY_FRAME_ID] = temperatureHexValue;
    }

    for (auto it : m_frameIds.toStdMap())
    {
        sendFrame(it.first, it.second);
    }
    //        if (m_ui->errorFrame->isChecked())
    //            frame.setFrameType(QCanBusFrame::ErrorFrame);
}


void MainWindow::sendFrame(const int frameId, QString &data) const
{
    const QByteArray payload = QByteArray::fromHex(data.remove(QLatin1Char(' ')).toLatin1());
    QCanBusFrame frame = QCanBusFrame(frameId, payload);
    m_canDevice->writeFrame(frame);
}
