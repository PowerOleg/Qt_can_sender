#ifndef CANCONTROLLER_H
#define CANCONTROLLER_H

#include <QObject>

class CanController : public QObject
{
    Q_OBJECT
public:
    explicit CanController(QObject *parent = nullptr);

signals:

public slots:
};

#endif // CANCONTROLLER_H