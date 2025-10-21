#ifndef SENDFRAMEBOX_H
#define SENDFRAMEBOX_H

#include <QCanBusFrame>
#include <QGroupBox>
#include <QRegularExpression>
#include <QValidator>
namespace Ui
{
class SendFrameBox;
}

class HexIntegerValidator : public QValidator
{
    Q_OBJECT
public:
    explicit HexIntegerValidator(QObject *parent = nullptr);
    QValidator::State validate(QString &input, int &) const;
    void setMaximum(uint maximum);
private:
    uint m_maximum = 0;
};

class HexStringValidator : public QValidator
{
    Q_OBJECT
public:
    explicit HexStringValidator(QObject *parent = nullptr);
    QValidator::State validate(QString &input, int &pos) const;
    void setMaxLength(int maxLength);
private:
    int m_maxLength = 0;
};

class SendFrameBox : public QGroupBox
{
    Q_OBJECT
public:
    explicit SendFrameBox(QWidget *parent = nullptr);
    ~SendFrameBox();
signals:
    void sendFrame(const QCanBusFrame &frame);
private:
    Ui::SendFrameBox *m_ui = nullptr;
    HexIntegerValidator *m_hexIntegerValidator = nullptr;
    HexStringValidator *m_hexStringValidator = nullptr;
};

#endif // SENDFRAMEBOX_H
