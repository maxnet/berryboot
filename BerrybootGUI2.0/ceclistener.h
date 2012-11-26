#ifndef CECLISTENER_H
#define CECLISTENER_H

#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <stdint.h>

class CecListener : public QThread
{
    Q_OBJECT
public:
    explicit CecListener(QObject *parent = 0);
    virtual ~CecListener();

signals:
    void keyPress(int key);
public slots:

protected:
    virtual void run();
    static void _cec_callback(void *userptr, uint32_t reason, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4);
    void cec_callback(uint32_t reason, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4);

    QWaitCondition _waitcond;
};

#endif // CECLISTENER_H
