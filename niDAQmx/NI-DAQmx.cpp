/* ********************************************************************
    Plugin "NI-DAQmx" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2020, Institut fuer Technische Optik (ITO),
    Universitaet Stuttgart, Germany

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

#define ITOM_IMPORT_API
#define ITOM_IMPORT_PLOTAPI

#include "NI-DAQmx.h"
#include "pluginVersion.h"
#include "gitVersion.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qplugin.h>
#include <qmessagebox.h>
#include "DataObject/dataobj.h"
#include <qvarlengtharray.h>
#include <qregexp.h>
#include <QRegularExpression>
#include <qelapsedtimer.h>
#include <iostream>
#include <qlist.h>
#include "NI-PeripheralClasses.h"

#include "dockWidgetNI-DAQmx.h"


//#include "dockWidgetniDAQmx.h"

/*callback function to receive an event when a task stops due to an error or when a finite acquisition 
task or finite generation task completes execution. A Done event does not occur 
when a task is stopped explicitly, such as by calling DAQmxStopTask.

This callback function is called synchronously, hence, in the thread that registered the event*/
int32 CVICALLBACK DoneEventCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
    NiDAQmx *instance = (NiDAQmx*)callbackData;
    if (instance && NiDAQmx::ActiveInstances.contains(instance))
    {
        instance->taskStopped(taskHandle, status);
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Constructor of Interface Class.
/*!
    \todo add necessary information about your plugin here.
*/
NiDAQmxInterface::NiDAQmxInterface()
{
    m_type = ito::typeDataIO | ito::typeADDA; //any grabber is a dataIO device AND its subtype grabber (bitmask -> therefore the OR-combination).
    setObjectName("NI-DAQmx");

    m_description = QObject::tr("NI-DAQmx");

    char pathSeparator[] =
    #if defined(__linux__) || defined(__APPLE__)
        "/";
    #else
        "\\";
    #endif

    char docstring[512];
    memset(&docstring, '\0', sizeof(docstring));
    std::string filepath (__FILE__);
    std::string dirpath = filepath.substr(0, filepath.rfind(pathSeparator));
    char first[] = "The plugin implements the DAQmx functions for analog-digital-converters from National Instruments. \n\
The installation needs the NI-DAQmx Library that can be downloaded from the NI website \n(https://www.ni.com/en-us/support/downloads/drivers/download.ni-daqmx.html).\n\n\
Basic plugin documentation is found in ";
    char last[] = ". \nOnline help is available through <plugin_ref>.exec('help').";
    strcpy (docstring, first);
    strcat (docstring, dirpath.c_str());
    strcat (docstring, pathSeparator);
    strcat (docstring, "doc");
    strcat (docstring, pathSeparator);
    strcat (docstring, "NiDAQmx.rst");
    strcat (docstring, last);

    m_detaildescription = QObject::tr(docstring);

    m_author = PLUGIN_AUTHOR;
    m_version = (PLUGIN_VERSION_MAJOR << 16) + (PLUGIN_VERSION_MINOR << 8) + PLUGIN_VERSION_PATCH;
    m_minItomVer = MINVERSION;
    m_maxItomVer = MAXVERSION;
    m_license = QObject::tr("licensed under LGPL");
    m_aboutThis = QObject::tr(GITVERSION); 
    
    ito::Param paramVal("taskType", ito::ParamBase::String, "analogInput", tr("type of the task related to this instance of the NI-DAQmx plugin (analogInput, digitalInput, analogOutput, digitalOutput)").toLatin1().data());
    ito::StringMeta *sm = new ito::StringMeta(ito::StringMeta::String);
    sm->addItem("analogInput");
    sm->addItem("digitalInput");
    sm->addItem("analogOutput");
    sm->addItem("digitalOutput");
    paramVal.setMeta(sm, true);
    m_initParamsMand << paramVal;

    m_initParamsOpt << ito::Param("taskName", ito::ParamBase::String, "", tr("desired name of the underlying NI task (this might be changed by the NI task creation method)").toLatin1().data());

    paramVal = ito::Param("taskMode", ito::ParamBase::String, "finite", tr("mode of the task recording / data generation: finite, continuous, onDemand").toLatin1().data());
    sm = new ito::StringMeta(ito::StringMeta::String);
    sm->addItem("finite");
    sm->addItem("continuous");
    //sm->addItem("onDemand");
    paramVal.setMeta(sm, true);
    m_initParamsOpt << paramVal;

    paramVal = ito::Param("samplingRate", ito::ParamBase::Double | ito::ParamBase::In, 0.0, 100000000.0, 100.0, tr("The sampling rate in samples per second per channel. If you use an external source for the Sample Clock, set this value to the maximum expected rate of that clock.").toLatin1().data());
    m_initParamsOpt << paramVal;

    paramVal = ito::Param("samplesPerChannel", ito::ParamBase::Int | ito::ParamBase::In, 0, std::numeric_limits<int>::max(),
        20000,
        tr("The number of samples to acquire or generate for each channel in the task (if taskMode is 'finite'). If taskMode is 'continuous', NI-DAQmx uses this value to determine the buffer size.").toLatin1().data());
    m_initParamsOpt << paramVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
NiDAQmxInterface::~NiDAQmxInterface()
{

}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmxInterface::getAddInInst(ito::AddInBase **addInInst)
{
    NEW_PLUGININSTANCE(NiDAQmx) //the argument of the macro is the classname of the plugin
    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmxInterface::closeThisInst(ito::AddInBase **addInInst)
{
   REMOVE_PLUGININSTANCE(NiDAQmx) //the argument of the macro is the classname of the plugin
   return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
/*static*/ int NiDAQmx::InstanceCounter = 0;
/*static*/ QVector<void*> NiDAQmx::ActiveInstances = QVector<void*>();

//----------------------------------------------------------------------------------------------------------------------------------
NiDAQmx::NiDAQmx() : 
    AddInDataIO(), 
    m_isgrabbing(false),
    m_taskHandle(NULL),
    m_deviceStartedCounter(0),
    m_taskStarted(false),
    m_digitalChannelDataType(ito::tUInt8)
{
    InstanceCounter++;
    ActiveInstances << this;

    // General Parameters
    ito::Param paramVal("name", ito::ParamBase::String | ito::ParamBase::Readonly | ito::ParamBase::In, "NI-DAQmx", NULL);
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::Wildcard, "*", "General"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("taskName", ito::ParamBase::String | ito::ParamBase::Readonly | ito::ParamBase::In, "", tr("name of the NI task that is related to this instance").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::Wildcard, "*", "General"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("availableDevices", ito::ParamBase::String | ito::ParamBase::Readonly | ito::ParamBase::In, "", tr("comma-separated list of all detected and available devices").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::Wildcard, "*", "General"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("availableTerminals", ito::ParamBase::String | ito::ParamBase::Readonly | ito::ParamBase::In, "", tr("comma-separated list of all detected and available terminals (e.g. for 'sampleClockSource' or 'startTriggerSource'). The standard sample clock source 'OnboardClock' is not contained in this list.").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::Wildcard, "*", "General"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("taskMode", ito::ParamBase::String | ito::ParamBase::In, "finite", tr("mode of the task recording / data generation: finite, continuous, onDemand").toLatin1().data());
    ito::StringMeta *sm = new ito::StringMeta(ito::StringMeta::String, "finite", "General");
    sm->addItem("continuous");
    sm->addItem("onDemand");
    paramVal.setMeta(sm, true);
    m_params.insert(paramVal.getName(), paramVal);
    m_taskMode = NiTaskModeFinite;

    paramVal = ito::Param("taskType", ito::ParamBase::String | ito::ParamBase::In | ito::ParamBase::Readonly, "analogInput", tr("task type: analogInput, analogOutput, digitalInput, digitalOutput").toLatin1().data());
    sm = new ito::StringMeta(ito::StringMeta::String, "analogInput", "General");
    sm->addItem("analogOutput");
    sm->addItem("digitalInput");
    sm->addItem("digitalOutput");
    paramVal.setMeta(sm, true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("loggingActive", ito::ParamBase::Int | ito::ParamBase::Readonly | ito::ParamBase::In, 0, 2, 0, tr("indicates if TDMS file logging has been enabled (see exec function 'configureLogging') for this input task.").toLatin1().data());
    paramVal.getMeta()->setCategory("Status");
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("taskStarted", ito::ParamBase::Int | ito::ParamBase::Readonly | ito::ParamBase::In, 0, 1, 0, tr("Indicates if the task is currently running (1) or stopped / inactive (0).").toLatin1().data());
    paramVal.getMeta()->setCategory("Status");
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("taskConfigured", ito::ParamBase::Int | ito::ParamBase::Readonly | ito::ParamBase::In, 0, 1, 0, tr("Indicates if the task is properly configured (1, all task related parameters where accepted) or not (0).").toLatin1().data());
    paramVal.getMeta()->setCategory("Status");
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("supportedChannels", ito::ParamBase::String | ito::ParamBase::Readonly | ito::ParamBase::In, "", tr("comma-separated list of all detected and supported channels with respect to the task type. Every item consists of the device name / channel name").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::Wildcard, "*", "Channels"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("channels", ito::ParamBase::String | ito::ParamBase::In, "", tr("semicolon-separated list of all channels that should be part of this task. Every item is a comma separated string that defines and parameterizes every channel.").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::Wildcard, "*", "Channels"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("samplingRate", ito::ParamBase::Double | ito::ParamBase::In, 0.0, 100000000.0, 100.0, tr("The sampling rate in samples per second per channel. If you use an external source for the Sample Clock, set this value to the maximum expected rate of that clock.").toLatin1().data());
    paramVal.getMeta()->setCategory("Acquisition/Write");
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("readTimeout", ito::ParamBase::Double | ito::ParamBase::In, -1.0, 100000.0, -1.0, 
        tr("Timeout when reading up to 'samplesPerChannel' values (per channel) in seconds. If -1.0 (default), the timeout is set to infinity (recommended for finite tasks). If 0.0, getVal/copyVal will return all values which have been recorded up to this call.").toLatin1().data());
    paramVal.getMeta()->setCategory("Acquisition/Write");
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("setValWaitForFinish", ito::ParamBase::Int | ito::ParamBase::In, 0, 1, 0,
        tr("If 1, the setVal call will block until all data has been written (only valid for finite tasks). If 0, setVal will return immediately, then use 'taskStarted' to verify if the operation has been finished.").toLatin1().data());
    paramVal.getMeta()->setCategory("Acquisition/Write");
    m_params.insert(paramVal.getName(), paramVal);
    
    paramVal = ito::Param("samplesPerChannel", ito::ParamBase::Int | ito::ParamBase::In, 0,std::numeric_limits<int>::max(), 
                                            20000, 
                                            tr("The number of samples to acquire or generate for each channel in the task (if taskMode is 'finite'). If taskMode is 'continuous', NI-DAQmx uses this value to determine the buffer size. This parameter is ignored for output tasks.").toLatin1().data());
    paramVal.getMeta()->setCategory("Acquisition/Write");
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("inputBufferSize", ito::ParamBase::Int | ito::ParamBase::In, -1, std::numeric_limits<int>::max(), -1,
        tr("Sets and changes the automatic input buffer allocation mode. If -1 (default), the automatic allocation is enabled. Else defines the number of samples the buffer can hold for each channel (only recommended for continuous acquisition). In automatic mode and continuous acquisition, the standard is a buffer size of 1 kS for a sampling rate < 100 S/s, 10 kS for 100-10000 S/s, 100 kS for 10-1000 kS/s and 1 MS else.").toLatin1().data());
    paramVal.getMeta()->setCategory("Acquisition/Write");
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("sampleClockSource", ito::ParamBase::String | ito::ParamBase::In, "OnboardClock", tr("The source terminal of the Sample Clock. To use the internal clock of the device, use an empty string or 'OnboardClock' (default). An example for an external clock source is 'PFI0' or PFI1'.").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::Wildcard, "*", "SampleClock"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("sampleClockRisingEdge", ito::ParamBase::Int | ito::ParamBase::In, 0, 1, 1, tr("If 1, samples are acquired on a rising edge of the sample clock (default), else they are acquired on a falling edge.").toLatin1().data());
    paramVal.getMeta()->setCategory("SampleClock");
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("startTriggerMode", ito::ParamBase::String | ito::ParamBase::In, "off", tr("Specifies the start trigger mode. 'off': software-based start trigger, 'digitalEdge': The start of acquiring or generating samples is given if the 'startTriggerSource' is activated (based on 'startTriggerRisingEdge'), 'analogEdge': similar to 'digitalEdge', but the analog input 'startTriggerSource' has to pass the value 'startTriggerLevel'.").toLatin1().data());
    sm = new ito::StringMeta(ito::StringMeta::String, "off", "StartTrigger");
    sm->addItem("digitalEdge");
    sm->addItem("analogEdge");
    paramVal.setMeta(sm, true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("startTriggerSource", ito::ParamBase::String | ito::ParamBase::In, "/Dev1/PFI0", tr("The source terminal of the trigger source (if 'startTriggerMode' is set to 'digitalEdge' or 'analogEdge').").toLatin1().data());
    paramVal.setMeta(new ito::StringMeta(ito::StringMeta::Wildcard, "*", "StartTrigger"), true);
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("startTriggerRisingEdge", ito::ParamBase::Int | ito::ParamBase::In, 0, 1, 1, tr("Specifies on which slope of the signal to start acquiring or generating samples. 1: rising edge (default), 0: falling edge.").toLatin1().data());
    paramVal.getMeta()->setCategory("StartTrigger");
    m_params.insert(paramVal.getName(), paramVal);
    
    paramVal = ito::Param("startTriggerLevel", ito::ParamBase::Double | ito::ParamBase::In, -1000.0, 1000.0, 1.0, tr("Only for 'startTriggerMode' == 'analogEdge': The threshold at which to start acquiring or generating samples. Specify this value in the units of the measurement or generation.").toLatin1().data());
    paramVal.getMeta()->setCategory("StartTrigger");
    m_params.insert(paramVal.getName(), paramVal);

    // Register Exec Functions
    QVector<ito::Param> pMand = QVector<ito::Param>();
    QVector<ito::Param> pOpt = QVector<ito::Param>();
    QVector<ito::Param> pOut = QVector<ito::Param>();

    pMand << ito::Param("loggingMode", ito::ParamBase::Int, 0, 2, 0, tr("0: logging is disabled, 1: logging is enabled with disabled read (fast, but no data can simultaneously read via getVal/copyVal), 2: logging is enabled with allowed reading of data.").toLatin1().data());
    pMand << ito::Param("filePath", ito::ParamBase::String, "", tr("path to the tdms file").toLatin1().data());
    pOpt << ito::Param("groupName", ito::ParamBase::String, "", tr("The name of the group to create within the TDMS file for data from this task.").toLatin1().data());
    paramVal = ito::Param("operation", ito::ParamBase::String, "createOrReplace", tr("Specifies how to open the TDMS file.").toLatin1().data());
    sm = new ito::StringMeta(ito::StringMeta::String);
    sm->addItem("open");
    sm->addItem("openOrCreate");
    sm->addItem("createOrReplace");
    sm->addItem("create");
    paramVal.setMeta(sm, true);
    pOpt << paramVal;

    registerExecFunc("configureLogging", pMand, pOpt, pOut, tr("Configures, en- or disables logging of input tasks to National Instruments tdms files. Can only be called if task is already configured."));

    //end register Exec Functions
    if (hasGuiSupport())
    {
        //now create dock widget for this plugin
        DockWidgetNIDAQmx *dw = new DockWidgetNIDAQmx(this);
        Qt::DockWidgetAreas areas = Qt::AllDockWidgetAreas;
        QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
        createDockWidget(QString(m_params["name"].getVal<char *>()), features, areas, dw);
    }

}

//----------------------------------------------------------------------------------------------------------------------------------
NiDAQmx::~NiDAQmx()
{
    
}



//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    QByteArray taskName = paramsOpt->at(0).getVal<const char*>();
    m_params["taskMode"].setVal<const char*>(paramsOpt->at(1).getVal<const char*>());

    switch (m_params["taskMode"].getVal<const char*>()[0])
    {
    case 'f': //finite
    {
        m_taskMode = NiTaskModeFinite;
        break;
    }
    case 'c': //continuous
    {
        m_taskMode = NiTaskModeContinuous;
        m_params["setValWaitForFinish"].setFlags(ito::ParamBase::Readonly | ito::ParamBase::In);
        break;
    }
    /*case 'o': // Hardware Timed Single Point
    {
        m_taskMode = NiTaskModeOnDemand;
        break;
    }*/
    default:
    {
        retValue += ito::RetVal::format(ito::retError, 0, "configure sample clock timing: Task mode '%s' is not supported.", m_params["taskMode"].getVal<const char*>());
        break;
    }
    }

    m_params["samplingRate"].setVal<double>(paramsOpt->at(2).getVal<double>());
    m_params["samplesPerChannel"].setVal<int>(paramsOpt->at(3).getVal<int>());

    QString taskType = paramsMand->at(0).getVal<const char*>();
    if (taskType == "analogInput")
    {
        m_taskType = TaskType::AnalogInput;
        if (taskName == "") { taskName = QString("AnalogInput %1").arg(InstanceCounter).toLatin1(); }
    }
    else if (taskType == "digitalInput")
    {
        m_taskType = TaskType::DigitalInput;
        if (taskName == "") { taskName = QString("DigitalInput %1").arg(InstanceCounter).toLatin1(); }
    }
    else if (taskType == "analogOutput")
    {
        m_taskType = TaskType::AnalogOutput;
        if (taskName == "") { taskName = QString("AnalogOutput %1").arg(InstanceCounter).toLatin1(); }
    }
    else if (taskType == "digitalOutput")
    {
        m_taskType = TaskType::DigitalOutput;
        if (taskName == "") { taskName = QString("DigitalOutput %1").arg(InstanceCounter).toLatin1(); }
    }
    else
    {
        retValue += ito::RetVal(ito::retError, 0, "invalid taskType");
    }

    m_params["taskName"].setVal<const char*>(taskName.constData());
    m_params["taskType"].setVal<const char*>(paramsMand->at(0).getVal<const char*>());

    if (!retValue.containsError())
    {
        retValue += scanForAvailableDevicesAndSupportedChannels();
    }

    setIdentifier(m_params["taskName"].getVal<const char*>());

    if (!retValue.containsError())
    {
        emit parametersChanged(m_params);
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    setInitialized(true); //init method has been finished (independent of retValue)
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::close(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);

    ito::RetVal retValue;

    if (m_deviceStartedCounter > 0)
    {
        m_deviceStartedCounter = 1;
        retValue += stopDevice(NULL);
    }

    //for safety only
    retValue += deleteTask();

    //if this instance is removed from ActiveInstances, the callback function DoneEventCallback becomes invalid!
    ActiveInstances.removeOne(this);

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
/* stops the task (if it is running without clearing it) */
ito::RetVal NiDAQmx::stopTask()
{
    ito::RetVal retValue;

    if (m_taskStarted && m_taskHandle)
    {
        retValue += checkError(DAQmxStopTask(m_taskHandle), "stopTask: DAQmxStopTask.");

        m_taskStarted = false;
        m_params["taskStarted"].setVal<int>(0);
        m_params["loggingActive"].setVal<int>(0);
        m_params["taskConfigured"].setVal<int>(0);

        emit parametersChanged(m_params);
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
void NiDAQmx::taskStopped(TaskHandle taskHandle, int32 status)
{
    if (taskHandle == m_taskHandle && m_params["taskStarted"].getVal<int>() > 0)
    {
        m_params["taskStarted"].setVal<int>(0);
        m_params["loggingActive"].setVal<int>(0);
        m_params["taskConfigured"].setVal<int>(0);

        emit parametersChanged(m_params);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::deleteTask()
{
    ito::RetVal retValue;

    retValue += stopTask();

    if (m_channels.size() > 0)
    {
        foreach(NiBaseChannel* nbc, m_channels)
        {
            delete nbc;
        }
        m_channels.clear();
    }
    
    if (m_taskHandle)
    {
        retValue += checkError(DAQmxClearTask(m_taskHandle), "deleteTask: DAQmxClearTask.");
        m_taskHandle = NULL;
    }
    
    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::createTask()
{
    ito::RetVal retValue;

    if (m_taskHandle)
    {
        retValue += stopTask();
        retValue += deleteTask();
    }

    retValue += checkError(DAQmxCreateTask(m_params["taskName"].getVal<const char*>(), &m_taskHandle), "Creating new task");

    if (!retValue.containsError() && m_taskHandle == NULL)
    {
        retValue += ito::RetVal(ito::retError, 0, "task could not be created (unknown reason)");
    }

    if (!retValue.containsError())
    {
        char buf[1024];
        if (DAQmxGetTaskName(m_taskHandle, buf, sizeof(buf)) == DAQmxSuccess)
        {
            m_params["taskName"].setVal<const char*>(buf);
        }

        retValue += checkError(DAQmxRegisterDoneEvent(m_taskHandle, DAQmx_Val_SynchronousEventCallbacks, DoneEventCallback, this), "DAQmxRegisterDoneEvent");

        if (!retValue.containsError())
        {
            emit parametersChanged(m_params);
        }
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::startTask()
{
    ito::RetVal retValue;

    if (m_channels.size() > 0 && m_taskHandle)
    {
        retValue += checkError(DAQmxStartTask(m_taskHandle), "startTask: DAQmxStartTask");

        if (!retValue.containsError())
        {
            m_taskStarted = true;
        }

        m_params["taskStarted"].setVal<int>(m_taskStarted ? 1 : 0);

        if (!retValue.containsError())
        {
            emit parametersChanged(m_params);
        }
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::createChannelsForTask()
{
    ito::RetVal retValue;

    if (!m_taskHandle)
    {
        retValue += ito::RetVal(ito::retError, 0, "no task created");
    }
    else
    {
        QStringList channels = QString(m_params["channels"].getVal<const char*>()).split(";");
        NiBaseChannel *nbc;
        
        foreach(const QString &channel, channels)
        {
            nbc = NULL;

            switch (m_taskType)
            {
            case TaskType::AnalogInput:
                nbc = NiAnalogInputChannel::fromConfigurationString(channel, retValue);
                break;
            case TaskType::AnalogOutput:
                nbc = NiAnalogOutputChannel::fromConfigurationString(channel, retValue);
                break;
            case TaskType::DigitalInput:
                nbc = NiDigitalInputChannel::fromConfigurationString(channel, retValue);
                break;
            case TaskType::DigitalOutput:
                nbc = NiDigitalOutputChannel::fromConfigurationString(channel, retValue);
                break;
            default:
                retValue += ito::RetVal(ito::retError, 0, "invalid task type");
                break;
            }

            if (!retValue.containsError() && nbc)
            {
                retValue += nbc->addChannelToTask(m_taskHandle);
                m_channels.append(nbc);
            }

            if (retValue.containsError())
            {
                break;
            }
        }

        const uInt32 bufferSize = 1024;
        char buffer[bufferSize];
        ito::RetVal retValue2 = checkError(DAQmxGetTaskChannels(m_taskHandle, buffer, bufferSize), "read all current task channels");

        QStringList availableChannels = QString(buffer).split(", ");

        QList<NiBaseChannel*>::iterator iter = m_channels.begin();
        uInt32 portWidth;
        int err;

        m_digitalChannelDataType = ito::tUInt8;

        while (iter != m_channels.end())
        {
            if (availableChannels.contains((*iter)->physicalName()))
            {
                if (m_taskType & Digital)
                {
                    err = DAQmxGetPhysicalChanDIPortWidth((*iter)->physicalName().toLatin1().data(), &portWidth);
                    if (err == DAQmxSuccess)
                    {
                        if (portWidth > 16 && m_digitalChannelDataType != ito::tInt32)
                        {
                            m_digitalChannelDataType = ito::tInt32;
                        }
                        else if (portWidth > 8 && portWidth <= 16 && m_digitalChannelDataType != ito::tInt32)
                        {
                            m_digitalChannelDataType = ito::tUInt16;
                        }
                    }
                }

                ++iter;
            }
            else
            {
                delete (*iter);
                iter = m_channels.erase(iter);
            }
        }

        //update parameters
        QStringList channelParams;
        foreach(NiBaseChannel* nbc, m_channels)
        {
            channelParams << nbc->getConfigurationString();
        }
        QByteArray channelsBytes = channelParams.join(";").toLatin1();
        m_params["channels"].setVal<const char*>(channelsBytes.constData());

        if (!retValue.containsError())
        {
            emit parametersChanged(m_params);
        }

        retValue += checkInternalData();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::configTask()
{
    ito::RetVal retValue;

    if (!m_taskHandle)
    {
        retValue += ito::RetVal(ito::retError, 0, "no task created");
    }
    else if (m_channels.size() > 0)
    {
        double rateHz = m_params["samplingRate"].getVal<double>();
        int samplesPerChannel = m_params["samplesPerChannel"].getVal<int>();
        QByteArray clockSignal = m_params["sampleClockSource"].getVal<const char*>();
        const char* clock = NULL; //default: OnboardClock
        if (clockSignal != "")
        {
            clock = clockSignal.constData();
        }

        int32 activeEdge = m_params["sampleClockRisingEdge"].getVal<int>() > 0 ? DAQmx_Val_Rising : DAQmx_Val_Falling;
        
        //configure task
        switch (m_taskMode)
        {
        case NiTaskModeFinite: //finite
        {
            // Notice that the onboardclock is not supported for DigitalIn. Either you implement it, or do not use it.
            retValue += checkError(DAQmxCfgSampClkTiming(m_taskHandle, clock /*OnboardClock*/, rateHz, activeEdge, DAQmx_Val_FiniteSamps, samplesPerChannel), "configure sample clock timing");
            break;
        }
        case NiTaskModeContinuous: //continuous
        {
            retValue += checkError(DAQmxCfgSampClkTiming(m_taskHandle, clock /*OnboardClock*/, rateHz, activeEdge, DAQmx_Val_ContSamps, samplesPerChannel), "configure sample clock timing");
            break;
        }
        //case NiTaskModeOnDemand: // Hardware Timed Single Point
        //{
        //    retValue += checkError(DAQmxCfgSampClkTiming(m_taskHandle, clock /*OnboardClock*/, rateHz, activeEdge, DAQmx_Val_HWTimedSinglePoint, samplesPerChannel), "configure sample clock timing");
        //    break;
        //}
        }

        int inputBufferSize = m_params["inputBufferSize"].getVal<int>();

        if (!retValue.containsError() && inputBufferSize >= 0)
        {
            retValue += checkError(DAQmxCfgInputBuffer(m_taskHandle, inputBufferSize), "DAQmxCfgInputBuffer");
        }

        if (!retValue.containsError())
        {
            QByteArray startTriggerMode = m_params["startTriggerMode"].getVal<const char*>();
            int32 edge = m_params["startTriggerRisingEdge"].getVal<int>() > 0 ? DAQmx_Val_Rising : DAQmx_Val_Falling;
            
            if (startTriggerMode == "off")
            {
                int32 err = DAQmxDisableStartTrig(m_taskHandle);
                if (err != -200452) //property not applicable
                {
                    retValue += checkError(err, "disable start trigger");
                }
            }
            else if (startTriggerMode == "digitalEdge")
            {
                retValue += checkError(DAQmxCfgDigEdgeStartTrig(m_taskHandle, m_params["startTriggerSource"].getVal<const char*>(), edge), "Create digital start trigger");
            }
            else //analogEdge
            {
                float64 triggerLevel = m_params["startTriggerLevel"].getVal<ito::float64>();
                retValue += checkError(DAQmxCfgAnlgEdgeStartTrig(m_taskHandle, m_params["startTriggerSource"].getVal<const char*>(), edge, triggerLevel), "Create analog start trigger");
            }
        }

        m_params["taskConfigured"].setVal<int>(retValue.containsError() ? 0 : 1);
        emit parametersChanged(m_params);
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;
    QString key;
    bool hasIndex = false;
    int index;
    QString suffix;
    ParamMapIterator it;

    retValue += apiParseParamName(val->getName(), key, hasIndex, index, suffix);

    if (retValue == ito::retOk)
    { //gets the parameter key from m_params map (read-only is allowed, since we only want to get the value).
        retValue += apiGetParamFromMapByKey(m_params, key, it, false);
    }
    if (!retValue.containsError())
    {
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
ito::RetVal NiDAQmx::setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    QString key;
    bool hasIndex;
    int index;
    QString suffix;
    ParamMapIterator it;
    bool restartDevice = false;

    ////parse the given parameter-name (if you support indexed or suffix-based parameters)
    retValue += apiParseParamName(val->getName(), key, hasIndex, index, suffix);

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
        if (key == "channels")
        {
            QString currentValue = it->getVal<const char*>();
            QString newValue = val->getVal<const char*>();

            retValue += it->copyValueFrom(&(*val));

            if (!retValue.containsError() && (currentValue != newValue))
            {
                restartDevice = true;
            }
        }
        else if (key == "samplingRate" || 
            key == "sampleClockSource" ||
            key == "samplesPerChannel" ||
            key == "sampleClockRisingEdge" ||
            key == "startTriggerMode" ||
            key == "startTriggerSource" ||
            key == "startTriggerRisingEdge" ||
            key == "startTriggerLevel" ||
            key == "continuousTaskBufferFactor")
        {
            retValue += it->copyValueFrom(&(*val));
            if (!retValue.containsError() && m_deviceStartedCounter > 0)
            {
                retValue += configTask();
            }
        }
        else if (key == "taskMode")
        {
            retValue += it->copyValueFrom(&(*val));

            NiTaskMode oldMode = m_taskMode;

            switch (it->getVal<const char*>()[0])
            {
                case 'f': //finite
                {
                    m_taskMode = NiTaskModeFinite;
                    m_params["setValWaitForFinish"].setFlags(ito::ParamBase::In);
                    break;
                }
                case 'c': //continuous
                {
                    m_taskMode = NiTaskModeContinuous;
                    m_params["setValWaitForFinish"].setFlags(ito::ParamBase::Readonly | ito::ParamBase::In);
                    break;
                }
                /*case 'o': // Hardware Timed Single Point
                {
                    m_taskMode = NiTaskModeOnDemand;
                    break;
                }*/
                default:
                {
                    retValue += ito::RetVal::format(ito::retError, 0, "configure sample clock timing: Task mode '%s' is not supported.", m_params["taskMode"].getVal<const char*>());
                    break;
                }
            }

            if (oldMode != m_taskMode)
            {
                restartDevice = true;
            }
        }
        else
        {
            //all parameters that don't need further checks can simply be assigned
            //to the value in m_params (the rest is already checked above)
            retValue += it->copyValueFrom(&(*val));
        }
    }

    if (restartDevice)
    {
        int deviceStartedCount = m_deviceStartedCounter;

        if (m_deviceStartedCounter > 0)
        {
            m_deviceStartedCounter = 1;
            retValue += stopDevice(NULL);

            if (!retValue.containsError())
            {
                retValue += startDevice(NULL);
            }
            
            if (!retValue.containsError())
            {
                m_deviceStartedCounter = deviceStartedCount;
            }
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
ito::RetVal NiDAQmx::startDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;

    if (m_deviceStartedCounter == 0)
    {
        //really start the device
        retValue += createTask();

        if (!retValue.containsError())
        {
            retValue += createChannelsForTask();
        }

        if (!retValue.containsError())
        {
            retValue += configTask();
        }

        retValue += checkInternalData();
    }

    if (!retValue.containsError())
    {
        m_deviceStartedCounter++;
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}
         
//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::stopDevice(ItomSharedSemaphore *waitCond)
{
    //ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;

    m_deviceStartedCounter--;

    if (m_deviceStartedCounter < 0)
    {
        m_deviceStartedCounter = 0;
    }
    else if (m_deviceStartedCounter == 0)
    {
        //really stop the device
        retValue += stopTask();
        retValue += deleteTask();
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}
         
//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::acquire(const int /*trigger*/, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;
    m_isgrabbing = false;

    if (m_deviceStartedCounter <= 0)
    {
        retValue += ito::RetVal(ito::retError, 0, "device not started.");
    }
    else if (!m_taskHandle)
    {
        retValue += ito::RetVal(ito::retError, 0, "No task started or no channels assigned to the task");
    }
    else if (m_taskType & TaskSubTypes::Output)
    {
        retValue += ito::RetVal(ito::retError, 0, "Task is an output task. No data acquisition is possible.");
    }
    else
    {
        if (m_taskMode == NiTaskMode::NiTaskModeFinite)
        {
            if (m_taskStarted)
            {
                retValue += stopTask();
            }

            retValue += startTask();
        }
        else if (m_taskMode == NiTaskMode::NiTaskModeContinuous)
        {
            if (!m_taskStarted)
            {
                retValue += startTask();
            }
            else
            {
                retValue += ito::RetVal(ito::retWarning, 0, "The continuous task has already been started.");
            }
        }
    }

    if (!retValue.containsError())
    {
        m_isgrabbing = true;
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
/*! Stops a running task (e.g. a continuous acquisition task)
*/
ito::RetVal NiDAQmx::stop(ItomSharedSemaphore *waitCond /*= NULL*/)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;

    if (!m_taskStarted)
    {
        retValue += ito::RetVal(ito::retError, 0, "Task not started.");
    }
    else if (!m_taskHandle)
    {
        retValue += ito::RetVal(ito::retError, 0, "Task not available.");
    }
    else
    {
        retValue += stopTask();
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Returns the grabbed NiDAQmx device data as reference.
/*!
    This method returns a reference to the recently acquired NiDAQmx data. Therefore this data must fit into the data structure of the 
    DataObject.
    
    This method returns a reference to the internal dataObject m_data of the NiDAQmx device where the currently acquired data is copied to (either
    in the acquire method or in retrieve data). Please remember, that the reference may directly change if a set of data is acquired.

    \param [in,out] vpdObj is the pointer to a given dataObject (this pointer should be cast to ito::DataObject*). After the call, the dataObject is a reference to the internal m_data dataObject of the NiDAQmx device.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError occurs when the NiDAQmx device has not been started or no data has been acquired by the method acquire.
    
    \sa retrieveImage, copyVal
*/
ito::RetVal NiDAQmx::getVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    int error = -1;
    ito::DataObject *dObj = reinterpret_cast<ito::DataObject *>(vpdObj);

    if (!retValue.containsError())
    {
        retValue += retrieveData();
    }

    if (!retValue.containsError())
    {
        *dObj = m_dataView; //dObj is now a shallow copy of the internal object m_data.
    }

    if (waitCond) 
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Returns the grabbed NiDAQmx device data as a deep copy.
/*!
    This method copies the recently grabbed NiDAQmx device data to the given DataObject. 
    
    The given dataObject must either have an empty size (then it is resized to the size and type of the NiDAQmx device data) or its size or adjusted region of
    interest must exactly fit to the size of the NiDAQmx device data. Then, the acquired data is copied inside of the given region of interest.

    \param [in,out] vpdObj is the pointer to a given dataObject (this pointer should be cast to ito::DataObject*) where the acquired data is deep copied to.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError is NiDAQmx device has not been started or no data has been acquired by the method acquire.
    
    \sa retrieveImage, getVal
*/
ito::RetVal NiDAQmx::copyVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    ito::DataObject *externalDataObject = reinterpret_cast<ito::DataObject *>(vpdObj);

    if (!externalDataObject)
    {
        retValue += ito::RetVal(ito::retError, 0, tr("NiDAQmx::copyVal - Empty object handle provided by caller").toLatin1().data());
    }

    if (!retValue.containsError())
    {
        retValue += retrieveData(); //retrieve data from device and store it in m_data
    }

    if (!retValue.containsError())
    {
        retValue += checkExternalDataToView(externalDataObject);
    }

    if (!retValue.containsError())
    {
        retValue += m_dataView.deepCopyPartial(*externalDataObject); //deeply copies the content of m_data to the current roi of externalDataObject

        //copy the scalings, offsets, axis descriptions, ... from the two last axes to the externalDataObject
        // (since the externalDataObject might have more dimensions than m_data)
        m_dataView.copyAxisTagsTo(*externalDataObject);
    }

    if (waitCond) 
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }
    
    return retValue;
}

//---------------------------------------------------------------------------------------------
/* retrieveData gets recently acquired data from the NI-card and stores them into the internal object m_data.
   Scalings, Offsets, axes descriptions etc. will be set to m_data accordingly.
*/
ito::RetVal NiDAQmx::retrieveData()
{
    ito::RetVal retValue;

    if (m_deviceStartedCounter <= 0)
    {
        retValue += ito::RetVal(ito::retError, 0, "device not started.");
    }
    else if (!m_taskHandle || m_channels.size() == 0)
    {
        retValue += ito::RetVal(ito::retError, 0, "No task started or no channels assigned to the task.");
    }
    else if (m_taskType & TaskSubTypes::Output)
    {
        retValue += ito::RetVal(ito::retError, 0, "Task is an output task. No data acquisition is possible.");
    }
    else if( !m_isgrabbing )
    {
        retValue += ito::RetVal(ito::retError, 0, tr("NiDAQmx::retrieveData - cannot retrieve data because none was acquired.").toLatin1().data());
    }
    else
    {
        int32 readNumSamples = -1;

        if (m_taskType & TaskSubTypes::Analog)
        {
            retValue += readAnalog(readNumSamples);
        }
        else if (m_taskType & TaskSubTypes::Digital)
        {
            retValue += readDigital(readNumSamples);
        }
        else if (m_taskType & TaskSubTypes::Counter)
        {
            retValue += readCounter(readNumSamples);
        }
        else
        {
            retValue += ito::RetVal(ito::retError, 0, "Invalid input task type");
        }

        if (!retValue.containsError() && readNumSamples >= 0)
        {
            if (readNumSamples == m_data.getSize(1))
            {
                m_dataView = m_data;
            }
            else
            {
                m_dataView = m_data.at(ito::Range::all(), ito::Range(0, readNumSamples));
            }
        }
    }

    if (m_taskMode == NiTaskMode::NiTaskModeFinite)
    {
        m_isgrabbing = false;
        retValue += stopTask();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::setVal(const char *data, const int /*length*/, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);

    const ito::DataObject *dObj = reinterpret_cast<const ito::DataObject*>(data);

    if (m_deviceStartedCounter <= 0)
    {
        retValue += ito::RetVal(ito::retError, 0, "device not started.");
    }
    else if (!m_taskHandle || m_channels.size() == 0)
    {
        retValue += ito::RetVal(ito::retError, 0, "No task started or no channels assigned to the task.");
    }
    else if (m_taskType & TaskSubTypes::Input)
    {
        retValue += ito::RetVal(ito::retError, 0, "Task is an input task. No data output is possible.");
    }

    int error = 0;

    if (!retValue.containsError())
    {
        if (m_taskStarted)
        {
            //if an output task is still running, we cannot only stop it using 'stopTask', but we have to clear it and
            //re-initialize it... (like calling startDevice)
            //really start the device
            retValue += createTask();

            if (!retValue.containsError())
            {
                retValue += createChannelsForTask();
            }

            if (!retValue.containsError())
            {
                retValue += configTask();
            }

            retValue += checkInternalData();
        }
    }

    if (!retValue.containsError())
    {
        if (m_taskType & TaskSubTypes::Analog)
        {
            retValue += writeAnalog(dObj);
        }
        else if (m_taskType & TaskSubTypes::Digital)
        {
            retValue += writeDigital(dObj);
        }
        else if (m_taskType & TaskSubTypes::Counter)
        {
            retValue += writeCounter(dObj);
        }
        else
        {
            retValue += ito::RetVal(ito::retError, 0, "Invalid input task type");
        }
    }

    if (!retValue.containsError())
    {
        retValue += startTask();
    }

    if (m_taskMode == NiTaskMode::NiTaskModeFinite && m_params["setValWaitForFinish"].getVal<int>() > 0)
    {
        int samples = m_params["samplesPerChannel"].getVal<int>();
        double samplingRate = m_params["samplingRate"].getVal<double>();

        //wait for task to be finished
        qint64 estimatedAcquisitionTimeMs = 1000.0 * samples / samplingRate;
        QElapsedTimer timer;
        timer.start();
        int err;

        //provide a set-alive feature for long acquisitions!
        while (1)
        {
            err = DAQmxWaitUntilTaskDone(m_taskHandle, 2);
            if (err == 0)
            {
                break;
            }
            else if (err != DAQmxErrorWaitUntilDoneDoesNotIndicateDone)
            {
                retValue += checkError(err, "Wait till task is done");
                break;
            }

            if (timer.elapsed() > 2 * estimatedAcquisitionTimeMs)
            {
                retValue += ito::RetVal(ito::retError, 0, "timeout while waiting for end of output operation");
            }
            else
            {
                setAlive();
            }
        }

        retValue += stopTask();
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! slot called if the dock widget of the plugin becomes (in)visible
/*!
    Overwrite this method if the plugin has a dock widget. If so, you can connect the parametersChanged signal of the plugin
    with the dock widget once its becomes visible such that no resources are used if the dock widget is not visible. Right after
    a re-connection emit parametersChanged(m_params) in order to send the current status of all plugin parameters to the dock widget.
*/
void NiDAQmx::dockWidgetVisibilityChanged(bool visible)
{
    if (getDockWidget())
    {
        QWidget *widget = getDockWidget()->widget();
        if (visible)
        {
            connect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), widget, SLOT(parametersChanged(QMap<QString, ito::Param>)));

            emit parametersChanged(m_params);
        }
        else
        {
            disconnect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), widget, SLOT(parametersChanged(QMap<QString, ito::Param>)));
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
//! method called to show the configuration dialog
/*!
    This method is called from the main thread from itom and should show the configuration dialog of the plugin.
    If the instance of the configuration dialog has been created, its slot 'parametersChanged' is connected to the signal 'parametersChanged'
    of the plugin. By invoking the slot sendParameterRequest of the plugin, the plugin's signal parametersChanged is immediately emitted with
    m_params as argument. Therefore the configuration dialog obtains the current set of parameters and can be adjusted to its values.
    
    The configuration dialog should emit reject() or accept() depending if the user wanted to close the dialog using the ok or cancel button.
    If ok has been clicked (accept()), this method calls applyParameters of the configuration dialog in order to force the dialog to send
    all changed parameters to the plugin. If the user clicks an apply button, the configuration dialog itsself must call applyParameters.
    
    If the configuration dialog is inherited from AbstractAddInConfigDialog, use the api-function apiShowConfigurationDialog that does all
    the things mentioned in this description.
    
    Remember that you need to implement hasConfDialog in your plugin and return 1 in order to signalize itom that the plugin
    has a configuration dialog.
    
    \sa hasConfDialog
*/
const ito::RetVal NiDAQmx::showConfDialog(void)
{
    return apiShowConfigurationDialog(this, new DialogNiDAQmx(this));
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::checkInternalData()
{
    int channels = m_channels.size();
    int samples = m_params["samplesPerChannel"].getVal<int>();

    ito::tDataType futureType = m_taskType & TaskSubTypes::Analog ? ito::tFloat64 : m_digitalChannelDataType;

    //check internal object m_data
    if (m_data.getDims() != 2 || m_data.getSize(0) != (unsigned int)channels || m_data.getSize(1) != (unsigned int)samples || m_data.getType() != futureType)
    {
        m_data = ito::DataObject(channels, samples, futureType);
    }


    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::checkExternalDataToView(ito::DataObject *externalData)
{
    int channels = m_channels.size();
    int samples = m_dataView.getSize(1);
    ito::tDataType futureType = m_taskType & TaskSubTypes::Analog ? ito::tFloat64 : m_digitalChannelDataType;

    int dims = externalData->getDims();
    if (externalData->getDims() == 0)
    {
        *externalData = ito::DataObject(channels, samples, futureType);
    }
    else if (externalData->calcNumMats() > 1)
    {
        return ito::RetVal(ito::retError, 0, tr("NiDAQmx::checkData - Error during check data, external dataObject invalid. Object has more than 1 plane. It must be of right size and type or a uninitilized image.").toLatin1().data());
    }
    else if (externalData->getSize(dims - 2) != (unsigned int)channels || externalData->getSize(dims - 1) != (unsigned int)samples || externalData->getType() != futureType)
    {
        return ito::RetVal(ito::retError, 0, tr("NiDAQmx::checkData - Error during check data, external dataObject invalid. Object must be of right size and type or a uninitilized image.").toLatin1().data());
    }

    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
/*
For analog tasks, every physical channel is mapped to one row in a getVal/copyVal/setVal dataObject
For digital tasks, it is either possible to have a port channel, such that every pin of the port is represented by one bit in one port channel
   or it is possible to select different lines (pins) of a port only.
*/
ito::RetVal NiDAQmx::scanForAvailableDevicesAndSupportedChannels()
{
    ito::RetVal retValue;
    ito::RetVal tempRetValue;
    m_availableDevices.clear();
    m_supportedChannels.clear();

    QStringList terminals;

    //scan for devices
    char buffer[5120];
    memset(buffer, 0, 5120 * sizeof(char));
    char buffer2[5120];
    memset(buffer2, 0, 5120 * sizeof(char));
    retValue += checkError(DAQmxGetSysDevNames(buffer, sizeof(buffer)), tr("Detect available devices."));

    if (!retValue.containsError())
    {
        QString deviceNames = QString(buffer);
        m_availableDevices = deviceNames.split(", ");
        if (m_availableDevices.size() == 0)
        {
            retValue += ito::RetVal(ito::retWarning, 0, "Could not detect any devices. Probably, no channels can be added to the task.");
        }
    }

    //scan for channels
    foreach(const QString &deviceName, m_availableDevices)
    {
        if (DAQmxGetDevTerminals(qPrintable(deviceName), buffer, sizeof(buffer)) == DAQmxSuccess)
        {
            terminals << QString(buffer).split(", ");
        }

        switch (m_taskType)
        {
        case AnalogInput:
            tempRetValue = checkError(DAQmxGetDevAIPhysicalChans(qPrintable(deviceName), buffer, sizeof(buffer)), QString("Detect supported analog input channels for device %1").arg(deviceName));
            buffer2[0] = '\0';
            break;
        case AnalogOutput:
            tempRetValue = checkError(DAQmxGetDevAOPhysicalChans(qPrintable(deviceName), buffer, sizeof(buffer)), QString("Detect supported analog output channels for device %1").arg(deviceName));
            buffer2[0] = '\0';
            break;
        case DigitalInput:
            tempRetValue = checkError(DAQmxGetDevDIPorts(qPrintable(deviceName), buffer, sizeof(buffer)), QString("Detect supported digital input ports for device %1").arg(deviceName));
            tempRetValue += checkError(DAQmxGetDevDILines(qPrintable(deviceName), buffer2, sizeof(buffer2)), QString("Detect supported digital input lines for device %1").arg(deviceName));
            break;
        case DigitalOutput:
            tempRetValue = checkError(DAQmxGetDevDOPorts(qPrintable(deviceName), buffer, sizeof(buffer)), QString("Detect supported digital output ports for device %1").arg(deviceName));
            tempRetValue += checkError(DAQmxGetDevDOLines(qPrintable(deviceName), buffer2, sizeof(buffer2)), QString("Detect supported digital output lines for device %1").arg(deviceName));
            break;
        default:
            buffer[0] = '\0';
            break;
        }

        if (!tempRetValue.containsError())
        {
            QStringList temp = QStringList() << QString(buffer).split(", ") << QString(buffer2).split(", ");

            foreach(const QString &t, temp)
            {
                if (t != "")
                {
                    m_supportedChannels << t;
                }
            } 
        }
        else
        {
            //convert an error into a warning
            tempRetValue = ito::RetVal(ito::retWarning, tempRetValue.errorCode(), tempRetValue.errorMessage());
        }

        retValue += tempRetValue;
    }

    if (m_supportedChannels.size() == 0)
    {
        retValue += ito::RetVal(ito::retWarning, 0, "Could not detect any supported channels, ports or lines. Probably, no channels can be added to the task.");
    }

    m_params["availableDevices"].setVal<char*>(m_availableDevices.join(",").toLatin1().data());
    m_params["supportedChannels"].setVal<char*>(m_supportedChannels.join(",").toLatin1().data());
    m_params["availableTerminals"].setVal<char*>(terminals.join(",").toLatin1().data());

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::readAnalog(int32 &readNumSamples)
{
    ito::RetVal retValue(ito::retOk);

    int channels = m_channels.size();
    int samples = m_params["samplesPerChannel"].getVal<int>();
    double samplingRate = m_params["samplingRate"].getVal<double>();
    double timeout = m_params["readTimeout"].getVal<double>();
    if (timeout < -std::numeric_limits<double>::epsilon())
    {
        timeout = DAQmx_Val_WaitInfinitely;
    }

    if (m_isgrabbing == false)
    {
        retValue += ito::RetVal(ito::retError, 0, tr("NiDAQmx::readAnalog - Tried to return data without first calling acquire").toLatin1().data());
    }
    else
    {
        //step 1: create and check m_data (if not yet available)
        //retValue += checkInternalData(NULL); //update external object or m_data

        if (!retValue.containsError())
        {
            if (m_taskMode == NiTaskModeFinite)
            {
                readNumSamples = -1;
                qint64 estimatedAcquisitionTimeMs = 1000.0 * samples / samplingRate;
                QElapsedTimer timer;
                timer.start();
                int err;

                //provide a set-alive feature for long acquisitions!
                while (1)
                {
                    err = DAQmxWaitUntilTaskDone(m_taskHandle, 2);

                    if (err == 0)
                    {
                        break;
                    }
                    else if (err != DAQmxErrorWaitUntilDoneDoesNotIndicateDone)
                    {
                        retValue += checkError(err, "Wait till task is done");
                        break;
                    }

                    if (timer.elapsed() > 2 * estimatedAcquisitionTimeMs)
                    {
                        retValue += ito::RetVal(ito::retError, 0, "timeout while waiting for finished acquisition");
                    }
                    else
                    {
                        setAlive();
                    }
                }

                if (!retValue.containsError())
                {
                    retValue += checkError(
                        DAQmxReadAnalogF64(m_taskHandle, samples, timeout, 
                            DAQmx_Val_GroupByChannel, m_data.rowPtr<ito::float64>(0, 0), 
                            m_data.getTotal(), &readNumSamples, NULL), 
                        "DAQmxReadAnalogF64"
                    );
                }
            }
            else if (m_taskMode == NiTaskModeContinuous)
            {
                readNumSamples = -1;
                int32 err;
                if (!retValue.containsError())
                {
                    err = DAQmxReadAnalogF64(m_taskHandle, samples, timeout, DAQmx_Val_GroupByChannel, m_data.rowPtr<ito::float64>(0, 0), m_data.getTotal(), &readNumSamples, NULL);

                    if (err == DAQmxErrorSamplesNotYetAvailable && timeout == 0.0)
                    {
                        //no timeout given, instantaneous return requested. If less samples available
                        //then requested, DAQmxErrorSamplesNotYetAvailable is returned. Ignore this.
                    }
                    else
                    {
                        //any other error
                        retValue += checkError(err, "DAQmxReadAnalogF64");
                    }
                }
            }
            /*else if (m_taskMode == NiTaskModeOnDemand)
            {
                retValue += ito::RetVal(ito::retError, 0, tr("NiDAQmx::readAnalog - On demand mode is not yet supported").toLatin1().data());
            }*/
            else
            {
                retValue += ito::RetVal(ito::retError, 0, tr("NiDAQmx::readAnalog - internal error invalid mode passed to readAnalog").toLatin1().data());
            }
        }

        m_data.setAxisScale(1, 1.0 / samplingRate);
        m_data.setAxisUnit(1, "s");
        m_data.setAxisDescription(1, "time");

        m_data.setValueUnit("V");
        m_data.setValueDescription("voltage");
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
// not supported yet, because an external clock source is neccesary, marc: todo: similar to readAnalog
ito::RetVal NiDAQmx::readDigital(int32 &readNumSamples)
{
    // right now I only support 8 line Ports! Some devices have 16 or 32 lines per port! 
    // just use: DAQmxReadDigitalU16, DAQmxReadDigitalU32
    ito::RetVal retValue(ito::retOk);

    int channels = m_channels.size();
    int samples = m_params["samplesPerChannel"].getVal<int>();
    double samplingRate = m_params["samplingRate"].getVal<double>();
    double timeout = m_params["readTimeout"].getVal<double>();
    if (timeout < -std::numeric_limits<double>::epsilon())
    {
        timeout = DAQmx_Val_WaitInfinitely;
    }

    if (m_isgrabbing == false)
    {
        retValue += ito::RetVal(ito::retError, 0, tr("NiDAQmx::readDigital - Tried to return data without first calling acquire").toLatin1().data());
    }
    else
    {
        if (!retValue.containsError())
        {
            if (m_taskMode == NiTaskModeFinite)
            {
                readNumSamples = -1;
                qint64 estimatedAcquisitionTimeMs = 1000.0 * samples / samplingRate;
                QElapsedTimer timer;
                timer.start();
                int err;

                //provide a set-alive feature for long acquisitions!
                while (1)
                {
                    err = DAQmxWaitUntilTaskDone(m_taskHandle, 2);
                    if (err == 0)
                    {
                        break;
                    }
                    else if (err != DAQmxErrorWaitUntilDoneDoesNotIndicateDone)
                    {
                        retValue += checkError(err, "Wait till task is done");
                        break;
                    }

                    if (timer.elapsed() > 2 * estimatedAcquisitionTimeMs)
                    {
                        retValue += ito::RetVal(ito::retError, 0, "timeout while waiting for finished acquisition");
                    }
                    else
                    {
                        setAlive();
                    }
                }

                if (!retValue.containsError())
                {
                    switch (m_digitalChannelDataType)
                    {
                    case ito::tUInt8:
                        retValue += checkError(DAQmxReadDigitalU8(m_taskHandle, samples, timeout, DAQmx_Val_GroupByChannel, m_data.rowPtr<ito::uint8>(0, 0), m_data.getTotal(), &readNumSamples, NULL), "DAQmxReadDigitalU8");
                        break;
                    case ito::tUInt16:
                        retValue += checkError(DAQmxReadDigitalU16(m_taskHandle, samples, timeout, DAQmx_Val_GroupByChannel, m_data.rowPtr<ito::uint16>(0, 0), m_data.getTotal(), &readNumSamples, NULL), "DAQmxReadDigitalU16");
                        break;
                    case ito::tInt32:
                        retValue += checkError(DAQmxReadDigitalU32(m_taskHandle, samples, timeout, DAQmx_Val_GroupByChannel, (uInt32*)m_data.rowPtr<ito::int32>(0, 0), m_data.getTotal(), &readNumSamples, NULL), "DAQmxReadDigitalU32");
                        break;
                    default:
                        retValue += ito::RetVal(ito::retError, 0, "unsupported datatype for digital channel");
                        break;
                    }
                    
                }
            }
            else if (m_taskMode == NiTaskModeContinuous)
            {
                readNumSamples = -1;
                int32 err;
                if (!retValue.containsError())
                {
                    switch (m_digitalChannelDataType)
                    {
                    case ito::tUInt8:
                        err = DAQmxReadDigitalU8(m_taskHandle, samples, timeout, DAQmx_Val_GroupByChannel, m_data.rowPtr<ito::uint8>(0, 0), m_data.getTotal(), &readNumSamples, NULL);
                        break;
                    case ito::tUInt16:
                        err = DAQmxReadDigitalU16(m_taskHandle, samples, timeout, DAQmx_Val_GroupByChannel, m_data.rowPtr<ito::uint16>(0, 0), m_data.getTotal(), &readNumSamples, NULL);
                        break;
                    case ito::tInt32:
                        err = DAQmxReadDigitalU32(m_taskHandle, samples, timeout, DAQmx_Val_GroupByChannel, (uInt32*)m_data.rowPtr<ito::int32>(0, 0), m_data.getTotal(), &readNumSamples, NULL);
                        break;
                    default:
                        retValue += ito::RetVal(ito::retError, 0, "unsupported datatype for digital channel");
                        break;
                    }

                    if (err == DAQmxErrorSamplesNotYetAvailable && timeout == 0.0)
                    {
                        //no timeout given, instantaneous return requested. If less samples available
                        //then requested, DAQmxErrorSamplesNotYetAvailable is returned. Ignore this.
                    }
                    else
                    {
                        //any other error
                        retValue += checkError(err, "DAQmxReadDigitalU8/16/32");
                    }
                    
                }
            }
            /*else if (m_taskMode == NiTaskModeOnDemand)
            {
                retValue += ito::RetVal(ito::retError, 0, tr("NiDAQmx::readDigital - On demand mode is not yet supported").toLatin1().data());
            }*/
            else
            {
                retValue += ito::RetVal(ito::retError, 0, tr("NiDAQmx::readDigital - internal error invalid mode passed to readDigital").toLatin1().data());
            }

            m_data.setAxisScale(1, 1.0 / samplingRate);
            m_data.setAxisUnit(1, "s");
            m_data.setAxisDescription(1, "time");

            m_data.setValueUnit("digit");
            m_data.setValueDescription("state");
        }
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::readCounter(int32 &readNumSamples)
{
    return ito::RetVal(ito::retError, 0, "read from counter task not implemented yet");
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::writeAnalog(const ito::DataObject *dataObj)
{
    ito::RetVal retValue(ito::retOk);
    int channels = m_channels.size();
    double samplingRate = m_params["samplingRate"].getVal<double>();
    int32 smplW = -1;

    if (dataObj->getDims() != 2 ||
        dataObj->getSize(0) != channels ||
        dataObj->getType() != ito::tFloat64)
    {
        retValue += ito::RetVal::format(ito::retError, 0, "%i x M, float64 dataObject required with M > 0.", channels);
    }
    else
    {
        int samples = dataObj->getSize(1);
        retValue += checkError(DAQmxWriteAnalogF64(m_taskHandle, samples, false, 0, DAQmx_Val_GroupByChannel, dataObj->rowPtr<ito::float64>(0, 0), &smplW, NULL), "DAQmxWriteAnalogF64");
    }

    //task must then be started

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::writeDigital(const ito::DataObject *dataObj)
{
    ito::RetVal retValue(ito::retOk);
    int channels = m_channels.size();
    double samplingRate = m_params["samplingRate"].getVal<double>();
    int32 smplW = -1;

    if (dataObj->getDims() != 2 ||
        dataObj->getSize(0) != channels ||
        dataObj->getType() != m_digitalChannelDataType)
    {
        switch (m_digitalChannelDataType)
        {
        case ito::tUInt8:
            retValue += ito::RetVal::format(ito::retError, 0, "%i x M, uint8 dataObject required with M <= %i", channels);
            break;
        case ito::tUInt16:
            retValue += ito::RetVal::format(ito::retError, 0, "%i x M, uint16 dataObject required with M <= %i", channels);
            break;
        case ito::tInt32:
            retValue += ito::RetVal::format(ito::retError, 0, "%i x M, int32 dataObject required with M <= %i", channels);
            break;
        }
        
    }
    else
    {
        int samples = dataObj->getSize(1);

        switch (m_digitalChannelDataType)
        {
        case ito::tUInt8:
            retValue += checkError(DAQmxWriteDigitalU8(m_taskHandle, samples, false, 0, DAQmx_Val_GroupByChannel, dataObj->rowPtr<const ito::uint8>(0, 0), &smplW, NULL), "DAQmxWriteDigitalU8");
            break;
        case ito::tUInt16:
            retValue += checkError(DAQmxWriteDigitalU16(m_taskHandle, samples, false, 0, DAQmx_Val_GroupByChannel, dataObj->rowPtr<const ito::uint16>(0, 0), &smplW, NULL), "DAQmxWriteDigitalU16");
            break;
        case ito::tInt32:
            retValue += checkError(DAQmxWriteDigitalU32(m_taskHandle, samples, false, 0, DAQmx_Val_GroupByChannel, (const uInt32*)dataObj->rowPtr<const ito::int32>(0, 0), &smplW, NULL), "DAQmxWriteDigitalU32");
            break;
        default:
            retValue += ito::RetVal(ito::retError, 0, "invalid digital channel data type.");
            break;
        }
    }

    //task must then be started

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::writeCounter(const ito::DataObject *dataObj)
{
    return ito::RetVal(ito::retError, 0, "write to counter task not implemented yet");
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal NiDAQmx::execFunc(const QString helpCommand, QSharedPointer<QVector<ito::ParamBase> > paramsMand, QSharedPointer<QVector<ito::ParamBase> > paramsOpt, QSharedPointer<QVector<ito::ParamBase> > /*paramsOut*/, ItomSharedSemaphore *waitCond)
{
    ito::RetVal retValue = ito::retOk;

    QStringList parts = helpCommand.split(":");
    QString function = parts[0];
    QString empty("");

    if (function == "configureLogging")
    {
        int mode = paramsMand->at(0).getVal<int>();
        QByteArray filePath = paramsMand->at(1).getVal<const char*>();
        QByteArray groupName = paramsOpt->at(0).getVal<const char*>();
        QByteArray operation = paramsOpt->at(1).getVal<const char*>();

        if (!m_taskHandle)
        {
            retValue += ito::RetVal(ito::retError, 0, "Task not available.");
        }
        else if (mode == 0)
        {
            m_params["loggingActive"].setVal<int>(0);

            if (m_taskType & Input)
            {
                retValue += checkError(DAQmxConfigureLogging(m_taskHandle, "", DAQmx_Val_Off, "", DAQmx_Val_CreateOrReplace), "DAQmxConfigureLogging");
            }
        }
        else if (m_taskType & Input)
        {
            int32 op = DAQmx_Val_CreateOrReplace;

            if (operation == "open")
            {
                op = DAQmx_Val_Open;
            }
            else if (operation == "openOrCreate")
            {
                op = DAQmx_Val_OpenOrCreate;
            }
            else if (operation == "create")
            {
                op = DAQmx_Val_Create;
            }

            int32 mode_ = mode == 1 ? DAQmx_Val_Log : DAQmx_Val_LogAndRead;

            retValue += checkError(DAQmxConfigureLogging(m_taskHandle, filePath.constData(), mode_, groupName.constData(), op), "DAQmxConfigureLogging");

            int32 mode = DAQmx_Val_Off;
            DAQmxGetLoggingMode(m_taskHandle, &mode);
            switch (mode)
            {
            case DAQmx_Val_Log:
                m_params["loggingActive"].setVal<int>(1);
                break;
            case DAQmx_Val_LogAndRead:
                m_params["loggingActive"].setVal<int>(2);
                break;
            default:
                m_params["loggingActive"].setVal<int>(0);
                break;
            }
            
        }
        else
        {
            retValue += ito::RetVal(ito::retError, 0, "Logging can only be enabled for input tasks");
        }

        emit parametersChanged(m_params);
    }
    else
    {
        retValue += ito::RetVal::format(ito::retError,0,tr("function name '%s' does not exist").toLatin1().data(), helpCommand.toLatin1().data());
    }

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
        waitCond->deleteSemaphore();
        waitCond = NULL;
    }

    return retValue;
}
