#include "ceclistener.h"
#include <QDebug>

extern "C" {
#include <interface/vmcs_host/vc_cecservice.h>
#include <interface/vchiq_arm/vchiq_if.h>
#include <interface/vmcs_host/vc_tvservice.h>
}

CecListener::CecListener(QObject *parent) :
    QThread(parent)
{

}

CecListener::~CecListener()
{
    _waitcond.wakeAll();
    if (!wait(2000))
    {
        qDebug() << "CEC thread did not stop gracefully";
        terminate();
    }
}

void CecListener::run()
{
    VCHI_INSTANCE_T vchiq;
    VCHI_CONNECTION_T *conn;
    QMutex mutex;
    mutex.lock();

    qDebug() << "CecListener thread started";
    if (vchi_initialise(&vchiq))
    {
        qDebug() << "Error during vchi_initialise()";
        return;
    }
    if (vchi_connect(NULL, 0, vchiq))
    {
        qDebug() << "Error connecting to vchi";
        return;
    }
    if (vc_vchi_tv_init(vchiq, &conn, 1))
    {
        qDebug() << "Error during vc_vchi_tv_init()";
        return;
    }
    TV_GET_STATE_RESP_T tvstate;
    vc_tv_get_state(&tvstate);

    if (!(tvstate.state & (VC_HDMI_STANDBY | VC_HDMI_HDMI)))
    {
        qDebug() << "Not in HDMI mode";
        return;
    }

    vc_vchi_cec_init(vchiq, &conn, 1);
    vc_cec_set_osd_name("BerryBoot");
    vc_cec_register_callback(_cec_callback, this);

    qDebug() << "CecListener done initializing";
    /* Wait until we are signaled to quit */
    _waitcond.wait(&mutex);

    vc_vchi_cec_stop();
}

/*static*/ void CecListener::_cec_callback(void *userptr, uint32_t reason, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4)
{
    static_cast<CecListener *>(userptr)->cec_callback(reason, param1, param2, param3, param4);
}

void CecListener::cec_callback(uint32_t reason, uint32_t param1, uint32_t, uint32_t, uint32_t)
{
    qDebug() << "CEC:" << reason << param1;

    if (CEC_CB_REASON(reason) == VC_CEC_BUTTON_PRESSED)
    {
        int c = 0;
        int cec_buttoncode = CEC_CB_OPERAND1(param1);

        qDebug() << "CEC button press code:" << cec_buttoncode;

        /* Translate to Qt key value */
        switch (cec_buttoncode)
        {
            case CEC_User_Control_Select:
                c = Qt::Key_Enter;
                break;
            case CEC_User_Control_Up:
                c = Qt::Key_Up;
                break;
            case CEC_User_Control_Down:
                c = Qt::Key_Down;
                break;
            case CEC_User_Control_Left:
                c = Qt::Key_Left;
                break;
            case CEC_User_Control_Right:
                c = Qt::Key_Right;
                break;
            case CEC_User_Control_Exit:
                c = Qt::Key_Escape;
                break;
            default:
                break;
        }

        if (c)
            emit keyPress(c);
    }
}

