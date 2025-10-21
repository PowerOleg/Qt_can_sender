#ifndef BITRATEBOX_H
#define BITRATEBOX_H

#include <QComboBox>

class QIntValidator;

class BitRateBox : public QComboBox
{
public:
    explicit BitRateBox(QWidget *parent = nullptr);
    ~BitRateBox();

    int bitRate() const;
    bool isFlexibleDataRateEnabled() const;
    void setFlexibleDateRateEnabled(bool enabled);
private slots:
    void checkCustomSpeedPolicy(int idx);
private:
    void fillBitRates();
    int m_isFlexibleDataRateEnabled = false;
    QIntValidator *m_customSpeedValidator = nullptr;
};

#endif // BITRATEBOX_H
