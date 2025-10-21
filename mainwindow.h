#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define TEMPERATURE_FRAME_ID 0x36
#define HUMIDITY_FRAME_ID 0x50
#include <QMainWindow>
#include <QCanBusDevice>
#include "sendframebox.h"

class ConnectDialog;
class QCanBusFrame;
class QLabel;

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

private slots:
    void sendFrame(const QCanBusFrame &frame) const;
    void processReceivedFrames();
    void processErrors(QCanBusDevice::CanBusError) const;
    void connectDevice();
    void disconnectDevice();
    void processFramesWritten(qint64);
    void adjustTemperatureValue();//qint8//uint8_t

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void initActionsConnections();
    void setTemperature(const int temperature);
    void setHumidity(const int humidity);
    qint64 m_numberFramesWritten = 0;
    Ui::MainWindow *m_ui = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_written = nullptr;
    ConnectDialog *m_connectDialog = nullptr;
    QCanBusDevice *m_canDevice = nullptr;
    QTimer* m_temperatureTimer;
    int m_temperatureTargetValue = 0;

};

#endif // MAINWINDOW_H
