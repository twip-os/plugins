/* ********************************************************************
    Plugin "DummyMotor" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2015, Institut f�r Technische Optik (ITO),
    Universit�t Stuttgart, Germany

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

/* *\file DummyMotor.cpp
* \brief In this file the functions for the classes of the DummyMotor and its Interface are defined
*
*    The Dummymotor is a virtual device to test positining function and to give developers a template for the implementation of actuators and their GUI 
*    This functions are based on the DummyMotor.cpp which was implemented into the ITO M and ITO M++ measurement programm at ITO, university stuttgart.
*
*\sa DummyMotorInterface, DummyMotor, DummyMotor.h
*\author ITO
*\date    Oct2011
*/

#define ITOM_IMPORT_API
#define ITOM_IMPORT_PLOTAPI

#include "DummyMotor.h"

#include <math.h>
#include <qstring.h>
#include <qstringlist.h>
#include <QtCore/QtPlugin>
#include <qelapsedtimer.h>
#include <qwaitcondition.h>

#include "pluginVersion.h"

#include "common/helperCommon.h"

#ifdef WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#include <iostream>
#include <qdebug.h>

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotorInterface::getAddInInst(ito::AddInBase **addInInst)
{
    NEW_PLUGININSTANCE(DummyMotor)
    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotorInterface::closeThisInst(ito::AddInBase **addInInst)
{
    REMOVE_PLUGININSTANCE(DummyMotor)
    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
DummyMotorInterface::DummyMotorInterface(QObject * /*parent*/)
{
    m_autoLoadPolicy = ito::autoLoadKeywordDefined;
    m_autoSavePolicy = ito::autoSaveAlways;

    m_type = ito::typeActuator;
    setObjectName("DummyMotor");

    //for the docstring, please don't set any spaces at the beginning of the line.
    char docstring[] = \
"The DummyMotor is a virtual actuator plugin that emulates up to 10 linear axes. \n\
\n\
The real number of simulated axes is given by the initialization parameter 'numAxis'. Use this plugin \
to simulate or develop your measurement system at another computer. Whenever a position command is executed, \
this plugin sleeps until the time needed for the positioning (with respect to the speed of the axis) \
expired.";

    m_description = QObject::tr("A virtual motor to test real actuators.");
    m_detaildescription = QObject::tr(docstring);
    m_author = "W. Lyda, ITO, University Stuttgart";
    m_version = (PLUGIN_VERSION_MAJOR << 16) + (PLUGIN_VERSION_MINOR << 8) + PLUGIN_VERSION_PATCH;
    m_minItomVer = MINVERSION;
    m_maxItomVer = MAXVERSION;
    m_license = QObject::tr("Licensed under LPGL.");
    m_aboutThis = tr("N.A.");       
    
    ito::Param paramVal = ito::Param("numAxis", ito::ParamBase::Int, 6, new ito::IntMeta(1,6), tr("Number of axis for this motor").toLatin1().data());
    m_initParamsOpt.append(paramVal);

    paramVal = ito::Param("motorName", ito::ParamBase::String, "DummyMotor", tr("Name for this dummyMotor").toLatin1().data());
    m_initParamsOpt.append(paramVal);

    return;
}

//----------------------------------------------------------------------------------------------------------------------------------
DummyMotorInterface::~DummyMotorInterface()
{
    m_initParamsMand.clear();
    m_initParamsOpt.clear();
}

//----------------------------------------------------------------------------------------------------------------------------------
#if QT_VERSION < 0x050000
    Q_EXPORT_PLUGIN2(DummyMotorInterface, DummyMotorInterface)
#endif

//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
const ito::RetVal DummyMotor::showConfDialog(void)
{
    return apiShowConfigurationDialog(this, new DialogDummyMotor(this));
}

//----------------------------------------------------------------------------------------------------------------------------------
DummyMotor::DummyMotor() :
    AddInActuator(),
    m_async(0),
    m_scale(1),
    m_distance(0)
{
    qRegisterMetaType<QMap<QString, ito::Param> >("QMap<QString, ito::Param>");    // To enable the programm to transmit parameters via signals - slot connections

    //register exec functions
    QVector<ito::Param> pMand = QVector<ito::Param>() << ito::Param("AxisNumber", ito::ParamBase::Int, 0, new ito::IntMeta(0,10), tr("Axis number to plot").toLatin1().data());
    QVector<ito::Param> pOpt = QVector<ito::Param>() << ito::Param("AddName", ito::ParamBase::Int, 0, new ito::IntMeta(0,1), tr("Add motor name").toLatin1().data());
    QVector<ito::Param> pOut = QVector<ito::Param>();
    registerExecFunc("dummyExecFunction", pMand, pOpt, pOut, tr("Print the current positions of the specified axis to the consol"));
    pMand.clear();
    pOpt.clear();

    //end register exec functions

    ito::Param paramVal("name", ito::ParamBase::String | ito::ParamBase::Readonly, "DummyMotor", NULL);    // Set up the parameter list
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("numaxis", ito::ParamBase::Int | ito::ParamBase::Readonly, 1, 6, 1, tr("Number of Axis attached to this stage").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);
    m_numaxis = paramVal.getVal<int>();

    paramVal = ito::Param("speed", ito::ParamBase::Double, 0.1, 100000.0, 1.0, tr("Speed of the axis between 0.1 and 100000 mm/s").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("accel", ito::ParamBase::Double, 1.0, 10.0, 1.0, tr("Acceleration in mm/s^2, currently not implemented").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("async", ito::ParamBase::Int, 0, 1, m_async, tr("Toggles if motor has to wait until end of movement (0:sync) or not (1:async)").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    /*paramVal = ito::Param("array", ito::ParamBase::IntArray, NULL, tr("test").toLatin1().data());
    paramVal.setMeta( new ito::IntMeta(0,5),true);
    m_params.insert(paramVal.getName(), paramVal);*/

    m_currentPos = QVector<double>(10,0.0);
    m_currentStatus = QVector<int>(10, ito::actuatorAtTarget);

    // This is for the docking widged
    //now create dock widget for this plugin
   DockWidgetDummyMotor *dummyMotorWid = new DockWidgetDummyMotor(getID(), this);    // Create a new non-modal dialog

   Qt::DockWidgetAreas areas = Qt::AllDockWidgetAreas;
   QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
   createDockWidget(QString(m_params["name"].getVal<char *>()), features, areas, dummyMotorWid);    // Give the widget a name ..)
   
   // till here
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;
    QString key;
    bool hasIndex = false;
    int index;
    QString suffix;
    QMap<QString,ito::Param>::iterator it;
        
    //parse the given parameter-name (if you support indexed or suffix-based parameters)
    retValue += apiParseParamName(val->getName(), key, hasIndex, index, suffix);
        
    if (retValue == ito::retOk)
    {
        //gets the parameter key from m_params map (read-only is allowed, since we only want to get the value).
        retValue += apiGetParamFromMapByKey(m_params, key, it, false);
    }

    if (!retValue.containsError())
    {
        //finally, save the desired value in the argument val (this is a shared pointer!)
        *val = it.value();
    }

    if (waitCond) 
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

   return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    QString key;
    bool hasIndex;
    int index;
    QString suffix;
    QMap<QString, ito::Param>::iterator it;
        
    //parse the given parameter-name (if you support indexed or suffix-based parameters)
    retValue += apiParseParamName( val->getName(), key, hasIndex, index, suffix );
        
    if (isMotorMoving()) //this if-case is for actuators only.
    {
        retValue += ito::RetVal(ito::retError, 0, tr("any axis is moving. Parameters cannot be set").toLatin1().data());
    }
        
    if (!retValue.containsError())
    {
        //gets the parameter key from m_params map (read-only is not allowed and leads to ito::retError).
        retValue += apiGetParamFromMapByKey(m_params, key, it, true);
    }

    if (!retValue.containsError())
    {
        //here the new parameter is checked whether its type corresponds or can be cast into the
        // value in m_params and whether the new type fits to the requirements of any possible
        // meta structure.
        retValue += apiValidateParam(*it, *val, false, true);
    }
        
    if (!retValue.containsError())
    { 
        if (key == "async")
        {
            //check the new value and if ok, assign it to the internal parameter
            retValue += it->copyValueFrom( &(*val) );
            m_async = m_params["async"].getVal<int>();
        }
        else
        {
            //all parameters that don't need further checks can simply be assigned
            //to the value in m_params (the rest is already checked above)
            retValue += it->copyValueFrom( &(*val) );
        }
    }
        
    if (!retValue.containsError())
    {
        emit parametersChanged(m_params); //send changed parameters to any connected dialogs or dock-widgets
    }

    if (waitCond) 
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::init(QVector<ito::ParamBase> * /*paramsMand*/, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    m_numaxis =  (*paramsOpt)[0].getVal<int>(); // Get the number of axis
    m_params["numaxis"].setVal<int>(m_numaxis);

    QString name = paramsOpt->at(1).getVal<char*>();
    if (name != "")
    {
        setIdentifier(name);
    }

    int oldLength = m_currentPos.size();
    m_currentPos.resize(m_numaxis);
    m_currentStatus.resize(m_numaxis);
    m_targetPos.resize(m_numaxis);
    for (int i=oldLength-1;i<m_currentPos.size();i++)
    {
        m_currentPos[i]     = 0.0;
        m_currentStatus[i]  = ito::actuatorAtTarget;
        m_targetPos[i]      = 0.0;
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    if (!retValue.containsWarningOrError())
    {    
        emit parametersChanged(m_params);
    }

    setInitialized(true); //init method has been finished (independent on retval)
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::close(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    if (waitCond)
    {
        waitCond->release();
        waitCond->returnValue = retValue;
    }
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::execFunc(const QString funcName, QSharedPointer<QVector<ito::ParamBase> > paramsMand, QSharedPointer<QVector<ito::ParamBase> > paramsOpt, QSharedPointer<QVector<ito::ParamBase> > /*paramsOut*/, ItomSharedSemaphore *waitCond)
{
    ito::RetVal retValue = ito::retOk;
    ito::ParamBase *param1 = NULL;
    ito::ParamBase *param2 = NULL;

    if (funcName == "dummyExecFunction")
    {
        param1 = ito::getParamByName(&(*paramsMand), "AxisNumber", &retValue);
        param2 = ito::getParamByName(&(*paramsOpt), "AddName", &retValue);

        if (!retValue.containsError())
        {
            int axis = param1->getVal<int>();
            int val = param2->getVal<int>();

            if ((axis >= m_numaxis) || (axis >= 10) || axis < 0)
            {
                retValue = ito::RetVal(ito::retError, 1, tr("axis index is out of bound").toLatin1().data());
            }
            else
            {               
                if (val) std::cout << "\nAxis: " << axis << " on position " << m_currentPos[axis] << " at Stage:" << m_params["name"].getVal<char*>() << "\n";
                else    std::cout << "\nAxis: " << axis << " on position " << m_currentPos[axis] << "\n";
            }
        }
    }
    else if (funcName == "changeGrating")
    {    
        

        if (!retValue.containsError())
        {
            
        }
    }
    else
    {
        retValue += ito::RetVal(ito::retError, 0, tr("function name '%1' does not exist").arg(funcName.toLatin1().data()).toLatin1().data());
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
        waitCond->deleteSemaphore();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::calib(const int axis, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    retValue += calib(QVector<int>(1,axis));

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::calib(const QVector<int> axis, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    if (isMotorMoving())
    {
        retValue += ito::RetVal(ito::retError, 0, tr("Any motor axis is already moving").toLatin1().data());

        if (waitCond)
        {
            waitCond->release();
            waitCond->returnValue = retValue;
        }
    }
    else
    {

        setStatus(axis, ito::actuatorMoving, ito::actSwitchesMask | ito::actStatusMask);
        sendStatusUpdate();

        retValue += waitForDone(5000, axis); //should drop into timeout
        retValue = ito::retOk;

        foreach(const int& i, axis)
        {
            m_currentPos[i] = 0.0;
            setStatus(m_currentStatus[i], ito::actuatorAtTarget, ito::actSwitchesMask | ito::actStatusMask);
        }

        if (waitCond)
        {
            waitCond->release();
            waitCond->returnValue = retValue;
        }

        sendStatusUpdate();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::setOrigin(const int axis, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    retValue += setOrigin(QVector<int>(1,axis), NULL);

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::setOrigin(const QVector<int> axis, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    if (isMotorMoving())
    {
        retValue += ito::RetVal(ito::retError, 0, tr("Any motor axis is already moving").toLatin1().data());

        if (waitCond)
        {
            waitCond->release();
            waitCond->returnValue = retValue;
        }
    }
    else
    {
        for (int naxis = 0; naxis < axis.size(); naxis++)
        {
            if ((axis[naxis] >= m_numaxis) || (axis[naxis] >= 10))
            {
                retValue = ito::RetVal(ito::retError, 0, tr("axis number exceeds number of axis").toLatin1().data());
            }
            else
            {
                m_currentPos[axis[naxis]] = 0.0;
                m_targetPos[axis[naxis]] = 0.0;
                setStatus(m_currentStatus[axis[naxis]], ito::actuatorAtTarget, ito::actSwitchesMask | ito::actStatusMask);
            }
        }

        if (waitCond)
        {
            waitCond->returnValue = retValue;
            waitCond->release();
        }

        sendStatusUpdate();
        sendTargetUpdate();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::getStatus(QSharedPointer<QVector<int> > status, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    *status = m_currentStatus;

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::getPos(const int axis, QSharedPointer<double> pos, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    if ((axis >= m_numaxis) || (axis >= 10) || axis < 0)
    {
        retValue = ito::RetVal(ito::retError, 1, tr("axis index is out of bound").toLatin1().data());
    }
    else
    {
        *pos = m_currentPos[axis];
        retValue = ito::retOk;
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::getPos(const QVector<int> axis, QSharedPointer<QVector<double> > pos, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    for (int naxis = 0; naxis < axis.size(); naxis++)
    {
        if ((axis[naxis] >= m_numaxis) || (axis[naxis] >= 10) || axis[naxis] < 0)
        {
            retValue = ito::RetVal(ito::retError, 1, tr("at least one axis index is out of bound").toLatin1().data());
        }
        else
        {
            (*pos)[naxis] = m_currentPos[axis[naxis]];
            retValue = ito::retOk;
        }
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::setPosAbs(const int axis, const double pos, ItomSharedSemaphore *waitCond)
{
    return setPosAbs(QVector<int>(1,axis), QVector<double>(1,pos), waitCond);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::setPosRel(const int axis, const double pos, ItomSharedSemaphore *waitCond)
{
    return setPosRel(QVector<int>(1,axis), QVector<double>(1,pos), waitCond);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::setPosAbs(const QVector<int> axis, QVector<double> pos, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    if (isMotorMoving())
    {
        retValue += ito::RetVal(ito::retError, 0, tr("Any motor axis is moving. The motor is locked.").toLatin1().data());
        if (waitCond)
        {
            waitCond->returnValue = retValue;
            waitCond->release();
        }
    }
    else
    {
        //check axis
        foreach(const int &i, axis)
        {
            if (i < 0 || i >= m_numaxis)
            {
                retValue += ito::RetVal(ito::retError, 0, tr("axis number is out of boundary").toLatin1().data());
            }
        }

        if (retValue.containsError())
        {
            if (waitCond)
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }
        }
        else
        {
            setStatus(axis, ito::actuatorMoving, ito::actSwitchesMask | ito::actStatusMask);
            sendStatusUpdate();

            m_distance = 0;

            for (int naxis = 0; naxis < axis.size(); naxis++)
            {
                if ((axis[naxis] >= m_numaxis) || (axis[naxis] >= 10))
                {
                    retValue += ito::RetVal(ito::retError, 0, tr("Axis is out of range.").toLatin1().data());
                }
                else
                {

                    // REMOVE THIS IF COPIED! THIS IS JUST NEEDED FOR THE WAIT-FUNCTION
                    if (abs(m_currentPos[axis[naxis]] - pos[naxis])  > m_distance)
                    {
                        m_distance = abs(m_currentPos[axis[naxis]] - pos[naxis]);
                    }
                    // REMOVE TILL HERE

                    m_currentPos[axis[naxis]] = pos[naxis];
                    m_targetPos[axis[naxis]] = pos[naxis];
                    retValue = ito::retOk;
                }
            }

            sendTargetUpdate();

            if (m_async && waitCond)
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }

            //calc time
            double cur_speed = m_params["speed"].getVal<double>(); //mm/s 
            if (cur_speed==0) cur_speed = 0.01;
            double durationMS = (m_distance / cur_speed) * 1000;

            ito::RetVal temp = waitForDone(durationMS, axis); //drops into timeout
            if (temp.containsError() && temp.errorCode() != 9999) //anything else besides timeout
            {
                retValue += temp;
            }

            replaceStatus(axis, ito::actuatorMoving, ito::actuatorAtTarget);
            sendStatusUpdate();

            if (!m_async && waitCond)
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }
        }
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::setPosRel(const QVector<int> axis, QVector<double> pos, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    
    if (isMotorMoving())
    {
        retValue += ito::RetVal(ito::retError, 0, tr("Any motor axis is moving. The motor is locked.").toLatin1().data());
        if (waitCond)
        {
            waitCond->returnValue = retValue;
            waitCond->release();
        }
    }
    else
    {
        //check axis
        foreach(const int &i, axis)
        {
            if (i < 0 || i >= m_numaxis)
            {
                retValue += ito::RetVal(ito::retError, 0, tr("axis number is out of boundary").toLatin1().data());
            }
        }

        if (retValue.containsError())
        {
            if (waitCond)
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }
        }
        else
        {
            setStatus(axis, ito::actuatorMoving, ito::actSwitchesMask | ito::actStatusMask);
            sendStatusUpdate();

            m_distance = 0;

            for (int naxis = 0; naxis < axis.size(); naxis++)
            {
                if ((axis[naxis] >= m_numaxis) || (axis[naxis] >= 10))
                {
                    retValue += ito::RetVal(ito::retError, 0, tr("Axis is out of range.").toLatin1().data());
                }
                else
                {

                    // REMOVE THIS IF COPIED! THIS IS JUST NEEDED FOR THE WAIT-FUNCTION
                    if (abs(pos[naxis] * m_scale) > m_distance)
                        m_distance = abs(pos[naxis]);
                    // REMOVE TILL HERE

                    m_currentPos[axis[naxis]] += pos[naxis]; //mm
                    m_targetPos[axis[naxis]] = m_currentPos[axis[naxis]];
                    retValue = ito::retOk;
                }
            }

            sendTargetUpdate();

            if (m_async && waitCond)
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }

            //calc time
            double cur_speed = m_params["speed"].getVal<double>(); //mm/s  * m_scale;
            if (cur_speed==0) cur_speed = 0.01;
            double durationMS = (m_distance / cur_speed)*1000;

            ito::RetVal temp = waitForDone(durationMS,axis); //drops into timeout
            if (temp.containsError() && temp.errorCode() != 9999) //anything else besides timeout
            {
                retValue += temp;
            }

            replaceStatus(axis, ito::actuatorMoving, ito::actuatorAtTarget);
            sendStatusUpdate();

            if (!m_async && waitCond)
            {
                waitCond->returnValue = retValue;
                waitCond->release();
            }
        }
    }

    return retValue;
}

//---------------------------------------------------------------------------------------------------------------------------------- 
ito::RetVal DummyMotor::requestStatusAndPosition(bool sendCurrentPos, bool sendTargetPos)
{
    ito::RetVal retval(ito::retOk);

    //in real motor, call getStatus and getPos here

    sendStatusUpdate(!sendCurrentPos);
    if (sendTargetPos)
    {
        sendTargetUpdate();
    }

    return retval;
}

//---------------------------------------------------------------------------------------------------------------------------------- 
void DummyMotor::dockWidgetVisibilityChanged(bool visible)
{
    if (getDockWidget())
    {
        if (visible)
        {
            QObject::connect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), getDockWidget()->widget(), SLOT(parametersChanged(QMap<QString, ito::Param>)));
            QObject::connect(this, SIGNAL(actuatorStatusChanged(QVector<int>,QVector<double>)),getDockWidget()->widget(), SLOT(actuatorStatusChanged(QVector<int>,QVector<double>)));
            QObject::connect(this, SIGNAL(targetChanged(QVector<double>)), getDockWidget()->widget(), SLOT(targetChanged(QVector<double>)));
            emit parametersChanged(m_params);
            requestStatusAndPosition(true,true);
        }
        else
        {
            QObject::disconnect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), getDockWidget()->widget(), SLOT(parametersChanged(QMap<QString, ito::Param>)));
            QObject::disconnect(this, SIGNAL(actuatorStatusChanged(QVector<int>,QVector<double>)),getDockWidget()->widget(), SLOT(actuatorStatusChanged(QVector<int>,QVector<double>)));
            QObject::disconnect(this, SIGNAL(targetChanged(QVector<double>)), getDockWidget()->widget(), SLOT(targetChanged(QVector<double>)));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------- 
ito::RetVal DummyMotor::waitForDone(const int timeoutMS, const QVector<int> axis /*if empty -> all axis*/, const int /*flags*/ /*for your use*/)
{
    ito::RetVal retVal(ito::retOk);
    bool done = false;
    bool timeout = false;
    QVector<int> _axis = axis;
    if (_axis.size() == 0) //all axis
    {
        for (int i=0;i<m_numaxis;i++) _axis.append(i);
    }
    
    QElapsedTimer timer;
    timer.start();
    QMutex waitMutex;
    QWaitCondition waitCondition;
    long delay = 10; //[ms]

    while (!done && !timeout)
    {
        if (!done && isInterrupted())
        {
            replaceStatus(_axis, ito::actuatorMoving, ito::actuatorInterrupted);
            retVal += ito::RetVal(ito::retError, 0, tr("interrupt occurred").toLatin1().data());
            done = true;
            return retVal;
        }

        //QCoreApplication::processEvents();

        //short delay
        waitMutex.lock();
        waitCondition.wait(&waitMutex, delay);
        waitMutex.unlock();
        setAlive();

        if (timeoutMS > -1)
        {
            if (timer.elapsed() > timeoutMS) timeout = true;
        }
    }

    if (timeout)
    {
        replaceStatus(_axis, ito::actuatorMoving, ito::actuatorAtTarget); //this is special for dummymotor, since timeout is a normal behaviour. Usually you should set the following status: ito::actuatorTimeout);
        retVal += ito::RetVal(ito::retError, 9999, tr("timeout occurred").toLatin1().data());
    }

    return retVal;
}
//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal DummyMotor::startJoyStickMovement(QVector<int> axis, QVector<double> vel)
{
    if (axis.size() != vel.size())
    {
        //qDebug()<< "Theoretically error with the \"Spass-Stecken\"\n";
        return ito::RetVal(ito::retError, 0, tr("Axis and velocity vector differ in size.").toLatin1().data());
    }
    if (axis.size() == 1)
    {
        if (abs(vel[0]) > 0.0001)
        {
            //qDebug() << "Theoretically started jogging due to the \"Spass-Stecken\"\n";
        }
        else
        {
            //qDebug() << "Theoretically stopped jogging due to the \"Spass-Stecken\"\n";
        }
    }
    else
    {
        //qDebug() << "Theoretically error with the \"Spass-Stecken\"\n";
        return ito::RetVal(ito::retError, 0, tr("Joystick movement failed somehow.").toLatin1().data());
    }

    return ito::retOk;
}
//----------------------------------------------------------------------------------------------------------------------------------
