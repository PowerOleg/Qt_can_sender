#include "sendframebox.h"
#include "ui_sendframebox.h"

enum {
    MaxStandardId = 0x7FF,
    MaxExtendedId = 0x10000000
};

enum {
    MaxPayload = 8,
    MaxPayloadFd = 64
};

HexIntegerValidator::HexIntegerValidator(QObject *parent) : QValidator(parent), m_maximum(MaxStandardId)
{}

QValidator::State HexIntegerValidator::validate(QString &input, int &) const
{
    bool ok;
    uint value = input.toUInt(&ok, 16);
    if (!value)
        return Intermediate;
    if (!ok || value > m_maximum)
        return Invalid;

    return Acceptable;
}

void HexIntegerValidator::setMaximum(uint maximum)
{
    m_maximum = maximum;
}

HexStringValidator::HexStringValidator(QObject *parent) : QValidator(parent), m_maxLength(MaxPayload)
{}

QValidator::State HexStringValidator::validate(QString &input, int &pos) const
{
    const int maxSize = 2 * m_maxLength;
    const QChar space = QLatin1Char(' ');
    QString data = input;
    data.remove(space);
    if (data.isEmpty())
        return Intermediate;
    // limit maximum size and forbid trailing spaces
    if ((data.size() > maxSize) || (data.size() == maxSize && input.endsWith(space)))
        return Invalid;
    // check if all input is valid
    const QRegularExpression re(QStringLiteral("^[[:xdigit:]]*$"));
    if (!re.match(data).hasMatch())
        return Invalid;
    // insert a space after every two hex nibbles
    const QRegularExpression insertSpace(QStringLiteral("(?:[[:xdigit:]]{2} )*[[:xdigit:]]{3}"));
    if (insertSpace.match(input).hasMatch())
    {
        input.insert(input.size() - 1, space);
        pos = input.size();
    }

    return Acceptable;
}

void HexStringValidator::setMaxLength(int maxLength)
{
    m_maxLength = maxLength;
}

SendFrameBox::SendFrameBox(QWidget *parent) : QGroupBox(parent), m_ui(new Ui::SendFrameBox)
{
    m_ui->setupUi(this);
    m_hexIntegerValidator = new HexIntegerValidator(this);
    m_ui->frameIdEdit->setValidator(m_hexIntegerValidator);
    m_hexStringValidator = new HexStringValidator(this);
    m_ui->payloadEdit->setValidator(m_hexStringValidator);
    connect(m_ui->dataFrame, &QRadioButton::toggled, [this](bool set) {
    if (set)
        m_ui->flexibleDataRateBox->setEnabled(true);
    });
    connect(m_ui->remoteFrame, &QRadioButton::toggled, [this](bool set) {
        if (set)
        {
            m_ui->flexibleDataRateBox->setEnabled(false);
            m_ui->flexibleDataRateBox->setChecked(false);
        }
    });
    connect(m_ui->errorFrame, &QRadioButton::toggled, [this](bool set) {
        if (set)
        {
            m_ui->flexibleDataRateBox->setEnabled(false);
            m_ui->flexibleDataRateBox->setChecked(false);
        }
    });
    connect(m_ui->extendedFormatBox, &QCheckBox::toggled, [this](bool set) {
        m_hexIntegerValidator->setMaximum(set ? MaxExtendedId : MaxStandardId);
    });
    connect(m_ui->flexibleDataRateBox, &QCheckBox::toggled, [this](bool set) {
        m_hexStringValidator->setMaxLength(set ? MaxPayloadFd : MaxPayload);
        m_ui->bitrateSwitchBox->setEnabled(set);
        if (!set)
            m_ui->bitrateSwitchBox->setChecked(false);
    });

    auto frameIdTextChanged = [this]() {
        const bool hasFrameId = !m_ui->frameIdEdit->text().isEmpty();
        m_ui->sendButton->setEnabled(hasFrameId);
        m_ui->sendButton->setToolTip(hasFrameId ? QString() : tr("Cannot send because no Frame ID was given."));
    };
    connect(m_ui->frameIdEdit, &QLineEdit::textChanged, frameIdTextChanged);
    frameIdTextChanged();
    connect(m_ui->sendButton, &QPushButton::clicked, [this]() {
        const uint frameId = m_ui->frameIdEdit->text().toUInt(nullptr, 16);
        QString data = m_ui->payloadEdit->text();
        const QByteArray payload = QByteArray::fromHex(data.remove(QLatin1Char(' ')).toLatin1());

        QCanBusFrame frame = QCanBusFrame(frameId, payload);
        frame.setExtendedFrameFormat(m_ui->extendedFormatBox->isChecked());
        frame.setFlexibleDataRateFormat(m_ui->flexibleDataRateBox->isChecked());
        frame.setBitrateSwitch(m_ui->bitrateSwitchBox->isChecked());
        if (m_ui->errorFrame->isChecked())
            frame.setFrameType(QCanBusFrame::ErrorFrame);
        else if (m_ui->remoteFrame->isChecked())
            frame.setFrameType(QCanBusFrame::RemoteRequestFrame);
        emit sendFrame(frame);
    });
}

SendFrameBox::~SendFrameBox()
{
    delete m_ui;
}
