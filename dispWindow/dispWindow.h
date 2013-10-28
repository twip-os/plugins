#ifndef DISPWINDOW_H
#define DISPWINDOW_H

#include "common/addInInterface.h"
#include "DataObject/dataobj.h"
#include "dialogDispWindow.h"
#include "dockWidgetDispWindow.h"
#include "projWindow.h"

//----------------------------------------------------------------------------------------------------------------------------------
class DispWindowInterface : public ito::AddInInterfaceBase
{
    Q_OBJECT
        Q_INTERFACES(ito::AddInInterfaceBase)

    protected:

    public:
        DispWindowInterface();
        ~DispWindowInterface();
        ito::RetVal getAddInInst(ito::AddInBase **addInInst);

    private:
        ito::RetVal closeThisInst(ito::AddInBase **addInInst);

    signals:

    public slots:
};

//----------------------------------------------------------------------------------------------------------------------------------
class DispWindow : public ito::AddInDataIO //, public DummyGrabberInterface
{
    Q_OBJECT

    private:
        PrjWindow *m_pWindow;

    protected:
        ~DispWindow();
        DispWindow();

    public:
        friend class DispWindowInterface;
        const ito::RetVal showConfDialog(void);
        int hasConfDialog(void) { return 1; }; //!< indicates that this plugin has got a configuration dialog
        void * getPrjWindow(void) { return (void*)m_pWindow; }

    signals:

    public slots:
        ito::RetVal getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond = NULL);

        ito::RetVal init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal close(ItomSharedSemaphore *waitConde = NULL);

        ito::RetVal getVal(void *data, ItomSharedSemaphore *waitCond);
        ito::RetVal setVal(const void *data, ItomSharedSemaphore *waitCond);
        void numberOfImagesChanged(int numImg, int numGray, int numCos);    /** Slot to archieve data from the child-GL-window*/

        ito::RetVal execFunc(const QString funcName, QSharedPointer<QVector<ito::ParamBase> > paramsMand, QSharedPointer<QVector<ito::ParamBase> > paramsOpt, QSharedPointer<QVector<ito::ParamBase> > paramsOut, ItomSharedSemaphore *waitCond = NULL);

    private slots:
        void dockWidgetVisibilityChanged(bool visible);
};



//----------------------------------------------------------------------------------------------------------------------------------

#endif // DISPWINDOW_H