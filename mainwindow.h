#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define TEMPERATURE_FRAME_ID 0x36
#define HUMIDITY_FRAME_ID 0x50
#include <QMainWindow>
#include <QCanBusDevice>
#include <QMap>
#include <QtCharts>// using namespace QtCharts

class ConnectDialog;
class QCanBusFrame;
class QLabel;

class QAbstractItemModel;
class QAbstractItemView;
class QItemSelectionModel;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString m_sensorInitValue = "FF";
    
private slots:
    void processReceivedFrames();
    void connectDevice();
    void disconnectDevice();
    void processFramesWritten(uint32_t);
    void on_sendButton_clicked();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void initActionsConnections();
    void setTemperatureInChart(const uint8_t temperature, const uint64_t seconds);
    void setHumidity(const uint8_t humidity);
    bool isInitSensor(uint8_t frameId, uint8_t value);
    void sendFrame(const uint8_t frameId, QString &data) const;
    void initChart();
    int8_t m_temperatureTargetValue = 0;
    uint8_t m_humidityTargetValue = 0;
    uint32_t m_numberFramesWritten = 0;
    Ui::MainWindow *m_ui = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_written = nullptr;
    ConnectDialog *m_connectDialog = nullptr;
    QCanBusDevice *m_canDevice = nullptr;
    QTimer* m_timeTimer = nullptr;
    QMap<uint8_t, QString> m_frameIds;
    QLineSeries* m_series = nullptr;
    QChart* m_chart = nullptr;
};

#endif // MAINWINDOW_H
