#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define TEMPERATURE_FRAME_ID 0x36
#define HUMIDITY_FRAME_ID 0x50
#include <QMainWindow>
#include <QCanBusDevice>
#include <QMap>
#include <QtCharts> using namespace QtCharts
#include "sendframebox.h"

class ConnectDialog;
class QCanBusFrame;
class QLabel;

class QAbstractItemModel;//231025
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
    void processErrors(QCanBusDevice::CanBusError) const;
    void connectDevice();
    void disconnectDevice();
    void processFramesWritten(qint64);
//    void adjustTemperatureValue();
    void on_sendButton_clicked();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void initActionsConnections();
    void setTemperatureInChart(const int temperature, const int time);
    void setHumidity(const int humidity);
    bool isInitSensor(int frameId, int value);
    void sendFrame(const int frameId, QString &data) const;
    void initChart();
    qint64 m_numberFramesWritten = 0;
    Ui::MainWindow *m_ui = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_written = nullptr;
    ConnectDialog *m_connectDialog = nullptr;
    QCanBusDevice *m_canDevice = nullptr;
    QTimer* m_timeTimer;

//    int m_temperatureTargetValue = 0;
    QMap<quint32, QString> m_frameIds;
    QSplineSeries* m_series;
    QChart* m_chart;
};

#endif // MAINWINDOW_H
