#include "dockWidgetDispWindow.h"
#include "dispWindow.h"

//#include "common/addInInterface.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------
DockWidgetDispWindow::DockWidgetDispWindow(const QString &identifier, PrjWindow *prjWindow, DispWindow *dispWindow) :
    m_pPrjWindow(prjWindow),
    m_pDispWindow(dispWindow),
    m_curNumPhaseShifts(-1),
    m_curNumGrayCodes(-1),
    m_numimgChangeInProgress(false)
{
    ui.setupUi(this); 
    ui.lblID->setText(identifier);
 }

//-------------------------------------------------------------------------------------------------------------------------------------------------
void DockWidgetDispWindow::valuesChanged(QMap<QString, ito::Param> params)
{
    int x = 0;
    int tempNumPhaseShifts = m_pPrjWindow->getPhaseShift();
    int tempNumGrayCodes = m_pPrjWindow->getNumGrayImages();

    if(tempNumPhaseShifts != m_curNumPhaseShifts || tempNumGrayCodes != m_curNumGrayCodes)
    {
        ui.comboBox->clear();

        for (x = 0; x < tempNumPhaseShifts; x++)
        {
            ui.comboBox->addItem(tr("phase shift %1").arg(x + 1), 0);
        }

        ui.comboBox->addItem(tr("black"), 0);
        ui.comboBox->addItem(tr("white"), 0);

        for (x = 0; x < tempNumGrayCodes; x++)
        {
            ui.comboBox->addItem(tr("gray images %1").arg(x + 1), 0);
        }

        m_curNumPhaseShifts = tempNumPhaseShifts ;
        m_curNumGrayCodes = tempNumGrayCodes;
    }

    if (params.keys().contains("numimg"))
    {
        m_numimgChangeInProgress = true;
        ui.comboBox->setCurrentIndex(params["numimg"].getVal<int>());
        m_numimgChangeInProgress = false;
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
void DockWidgetDispWindow::on_comboBox_currentIndexChanged(int index)
{
    if (!m_numimgChangeInProgress)
    {
        ito::RetVal retval;
        QSharedPointer<ito::ParamBase> numimg(new ito::ParamBase("numimg", ito::ParamBase::Int | ito::ParamBase::In, index));
        ItomSharedSemaphoreLocker locker(new ItomSharedSemaphore());

        QMetaObject::invokeMethod(m_pDispWindow, "setParam", Q_ARG(QSharedPointer<ito::ParamBase>, numimg), Q_ARG(ItomSharedSemaphore*, locker.getSemaphore()));

        if (!locker.getSemaphore()->wait(5000))
        {
            retval += ito::RetVal(ito::retError, 0, tr("Dropped to time-out").toAscii().data());
        }
        else
        {
            retval += locker.getSemaphore()->returnValue;
        }

        if (retval != ito::retOk)
        {
            QString text;
            if (retval.errorMessage()) text = QString("\n%1").arg(retval.errorMessage());
            if (retval.containsError())
            {
                text.prepend(tr("An error occured while taking the dark reference."));
                QMessageBox::critical(this, tr("take dark reference"), text);
            }
            else if (retval.containsWarning())
            {
                text.prepend(tr("A warning occured while taking the dark reference."));
                QMessageBox::warning(this, tr("take dark reference"), text);
            }
        }
    }
}