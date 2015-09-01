/* ********************************************************************
    Plugin "SuperlumBL" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2013, Institut für Technische Optik (ITO),
    Universität Stuttgart, Germany

    This file is part of a plugin for the measurement software itom.
  
    This itom-plugin is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public Licence as published by
    the Free Software Foundation; either version 2 of the Licence, or (at
    your option) any later version.

    itom and its plugins are distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
    General Public Licence for more details.

    You should have received a copy of the GNU Library General Public License
    along with itom. If not, see <http://www.gnu.org/licenses/>.
*********************************************************************** */

#ifndef SUPERLUMBL_H
#define SUPERLUMBL_h    

#include "common/addInInterface.h"
#include "common/abstractAddInDockWidget.h"

#include "dialogSuperlumBL.h"
#include "dockWidgetSuperlumBL.h"

#include <qsharedpointer.h>
#include <qvector.h>
#include <qpair.h>
#include <qbytearray.h>

//----------------------------------------------------------------------------------------------------------------------------------
 /**
  *\class    SuperlumBL
  *\brief    ToDo
  *
  *    \sa    AddInActuator, SuperlumBL, 
  *    \date    31.08.2015
  *    \author    J. Krauter, T. Boettcher
  * \warning    NA
  *
  */
class SuperlumBL : public ito::AddInActuator 
{
    Q_OBJECT

    protected:
        ~SuperlumBL() {}
        SuperlumBL();

    public:
        friend class SuperlumBLInterface;
        const ito::RetVal showConfDialog(void);
        int hasConfDialog(void) { return 1; }        //!< indicates that this plugin has got a configuration dialog
        
    private:

        ito::AddInDataIO *m_pSer;
        int m_delayAfterSendCommandMS;

        enum DeviceType {S_840_B_I_20, Unknown};

        DeviceType m_deviceType;

        DockWidgetSuperlumBL *m_dockWidget;
        //ito::RetVal SerialDummyRead(void); /*!< reads buffer of serial port without delay in order to clear it */
        ito::RetVal SerialSendCommand(QByteArray command);
        ito::RetVal readString(QByteArray &command, QByteArray &result, int &len, int timeoutMS);
        ito::RetVal SendQuestionWithAnswerString(QByteArray questionCommand, QByteArray &answer, int timeoutMS);
        //ito::RetVal SetPos(const int axis, const double posMM, bool relNotAbs, ItomSharedSemaphore *waitCond = NULL);    /*!< Set a position (absolute or relative) */
        //ito::RetVal CheckStatus(void);
        ito::RetVal waitForDone(const int timeoutMS = -1, const QVector<int> axis = QVector<int>() /*if empty -> all axis*/, const int flags = 0 /*for your use*/);      
        ito::RetVal IdentifyAndInitializeSystem();

    public slots:
        
        ito::RetVal getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond = NULL);

        ito::RetVal init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal close(ItomSharedSemaphore *waitCond);

        //! Starts calibration for a single axis
        ito::RetVal calib(const int axis, ItomSharedSemaphore *waitCond = NULL);
        //! Starts calibration for all axis -> calls single axis version
        ito::RetVal calib(const QVector<int> axis, ItomSharedSemaphore *waitCond = NULL);
        //! Not implelemted yet
        ito::RetVal setOrigin(const int axis, ItomSharedSemaphore *waitCond = NULL);
        //! Not implelemted yet
        ito::RetVal setOrigin(const QVector<int> axis, ItomSharedSemaphore *waitCond = NULL);
        //! Reads out status request answer and gives back ito::retOk or ito::retError
        ito::RetVal getStatus(QSharedPointer<QVector<int> > status, ItomSharedSemaphore *waitCond);
        //! Get the position of a single axis
        ito::RetVal getPos(const int axis, QSharedPointer<double> pos, ItomSharedSemaphore *waitCond);
        //! Get the position of a all axis -> calls single axis version
        ito::RetVal getPos(const QVector<int> axis, QSharedPointer<QVector<double> > pos, ItomSharedSemaphore *waitCond);
        //! Set an absolut position and go thier. Waits if m_async=0. Calls PISetPos of axis=0 else ito::retError
        ito::RetVal setPosAbs(const int axis, const double pos, ItomSharedSemaphore *waitCond = NULL);
        //! Set an absolut position and go thier. Waits if m_async=0. Calls PISetPos of axis[0]=0 && axis.size()=1 else ito::retError
        ito::RetVal setPosAbs(const QVector<int> axis, QVector<double> pos, ItomSharedSemaphore *waitCond = NULL);
        //! Set a relativ offset of current position and go thier. Waits if m_async=0. Calls PISetPos of axis=0 else ito::retError
        ito::RetVal setPosRel(const int axis, const double pos, ItomSharedSemaphore *waitCond = NULL);
        //! Set a relativ offset of current position and go thier. Waits if m_async=0. Calls PISetPos of axis[0]=0 && axis.size()=1 else ito::retError
        ito::RetVal setPosRel(const QVector<int> axis, QVector<double> pos, ItomSharedSemaphore *waitCond = NULL);
        
        //! Emits status and position if triggered. Used form the dockingwidget
        ito::RetVal requestStatusAndPosition(bool sendCurrentPos, bool sendTargetPos);

    private slots:
        void dockWidgetVisibilityChanged( bool visible );
};

//----------------------------------------------------------------------------------------------------------------------------------
 /**
  *\class    SuperlumBLInterface 
  *
  *\brief    Interface-Class for SuperlumBLInterface-Class
  *
  *    \sa    AddInActuator, SuperlumBL
  *    \date    31.08.2015
  *    \author    J. Krauter, T.Boettcher
  * \warning    NA
  *
  */
class SuperlumBLInterface : public ito::AddInInterfaceBase
{
    Q_OBJECT
#if QT_VERSION >=  QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "ito.AddInInterfaceBase" )
#endif
    Q_INTERFACES(ito::AddInInterfaceBase)
    PLUGIN_ITOM_API

    protected:

    public:
        SuperlumBLInterface();
        ~SuperlumBLInterface() {};
        ito::RetVal getAddInInst(ito::AddInBase **addInInst);

    private:
        ito::RetVal closeThisInst(ito::AddInBase **addInInst);

    signals:
        

    public slots:
};

//----------------------------------------------------------------------------------------------------------------------------------

#endif // SUPERLUMBL_H