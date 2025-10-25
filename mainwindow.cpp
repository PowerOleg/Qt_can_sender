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
    axisX->setRange(0, 1);
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
    m_timeTimer = new QTimer(this);
    initActionsConnections();
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
    //connect(m_temperatureTimer, &QTimer::timeout, this, &MainWindow::adjustTemperatureValue);
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
        m_timeTimer->start(1000);
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
    m_status->setText(tr("Disconnected"));
}

void MainWindow::processFramesWritten(uint32_t count)
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


            const QString time = QString::fromLatin1("%1.%2 ")
            .arg(frame.timeStamp().seconds(), 10, 10, QLatin1Char(' '))
            .arg(frame.timeStamp().microSeconds() / 100, 4, 10, QLatin1Char('0'));

            const QString flags = frameFlags(frame);
//
            view = frame.toString();
            if (frameId == TEMPERATURE_FRAME_ID && !payload.isEmpty())
            {
                uint8_t temperature = static_cast< uint8_t >(payload[0]);
                uint64_t sec = static_cast< uint8_t >(payload[1]);
                uint8_t min = static_cast< uint8_t >(payload[2]);
                uint8_t hour = static_cast< uint8_t >(payload[3]);
                if (hour > 0)
                {
                    min += hour * 60;
                }
                if (min > 0)
                {
                    sec += min * 60;
                }
                m_chart->axisX()->setMax(QVariant::fromValue(sec + 60));
                if (!isInitSensor(TEMPERATURE_FRAME_ID, temperature))
                {
                    setTemperatureInChart(temperature, sec);
                }
            }
            if (frameId == HUMIDITY_FRAME_ID && !payload.isEmpty())
            {
                uint8_t humidity = static_cast< uint8_t >(payload[0]);
                if (!isInitSensor(HUMIDITY_FRAME_ID, humidity))
                {
                    setHumidity(humidity);
                }
            }
            m_ui->receivedMessagesEdit->append(time + flags + view);
        }

    }
}

bool MainWindow::isInitSensor(uint8_t frameId, uint8_t value)
{
    if (value == 255)
    {
        sendFrame(frameId, m_sensorInitValue);
        return true;
    }
    return false;
}

void MainWindow::setTemperatureInChart(const uint8_t temperature, const uint64_t seconds)
{
    //int8_t oldTemperature = m_ui->temperatureSpinBox->value();
    int8_t newTemperature = (temperature - 100) < -100 ? -100 : temperature - 100;
    newTemperature = newTemperature > 100 ? 100 : newTemperature;

    m_series->append(seconds, newTemperature);
}

void MainWindow::setHumidity(const uint8_t humidity)
{
    uint8_t zero = 0;
    uint8_t hundred = 100;
    uint8_t newHumidity = humidity < zero ? zero : humidity;
    newHumidity = newHumidity > hundred ? hundred : newHumidity;
    m_ui->humiditySpinBox->setValue(newHumidity);
}

void MainWindow::on_sendButton_clicked()
{
    if (!m_canDevice)
        return;
    const bool hasTemperature = !m_ui->temperatureSpinBox->text().isEmpty();
    if (hasTemperature)
    {
        int8_t temperatureValue = m_ui->temperatureSpinBox->value();
        if (temperatureValue != m_temperatureTargetValue)
        {
            QString temperatureHexValue = QString("%1").arg(temperatureValue + 100, 2, 16, QLatin1Char( '0' ));
            m_frameIds[TEMPERATURE_FRAME_ID] = temperatureHexValue;
            m_temperatureTargetValue = temperatureValue;
        }
    }

    const bool hasHumidity = !m_ui->humiditySpinBox->text().isEmpty();
    if (hasHumidity)
    {
        uint8_t humidityValue = m_ui->humiditySpinBox->value();
        if (humidityValue != m_humidityTargetValue)
        {
            QString humidityHexValue = QString("%1").arg(humidityValue, 2, 16, QLatin1Char( '0' ));
            m_frameIds[HUMIDITY_FRAME_ID] = humidityHexValue;
            m_humidityTargetValue = humidityValue;
        }
    }

    for (auto it : m_frameIds.toStdMap())
    {
        sendFrame(it.first, it.second);
        m_frameIds.remove(it.first);
    }
}

void MainWindow::sendFrame(const uint8_t frameId, QString &data) const
{
    const QByteArray payload = QByteArray::fromHex(data.remove(QLatin1Char(' ')).toLatin1());
    QCanBusFrame frame = QCanBusFrame(frameId, payload);
    m_canDevice->writeFrame(frame);
}
