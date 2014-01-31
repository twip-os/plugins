#include "Vistek.h"
#include "VistekInterface.h"
#include "VistekContainer.h"

#define _USE_MATH_DEFINES  // needs to be defined to enable standard declartions of PI constant
#include "math.h"

#include <qstring.h>
#include <qstringlist.h>
#include <QtCore/QtPlugin>
#include <qmetaobject.h>

#include <qdockwidget.h>
#include <qpushbutton.h>
#include <qmetaobject.h>
#include "dockWidgetVistek.h"
#include <qelapsedtimer.h>


VistekContainer* VistekContainer::m_pVistekContainer = NULL;

Q_DECLARE_METATYPE(ito::DataObject)

//----------------------------------------------------------------------------------------------------------------------------------
/*!
    \class Vistek
    \brief Class that can handle SVS Vistek GigE Cameras.

    Usually every method in this class can be executed in an own thread. Only the constructor, destructor, showConfDialog will be executed by the 
    main (GUI) thread.
*/

//----------------------------------------------------------------------------------------------------------------------------------
//! Shows the configuration dialog. This method must be executed in the main (GUI) thread and is usually called by the addIn-Manager.
/*!
    Creates new instance of dialogVistek, calls the method setVals of dialogVistek, starts the execution loop and if the dialog
    is closed, reads the new parameter set and deletes the dialog.

    \return ito::RetVal retOk
    \sa dialogVistek
*/
const ito::RetVal Vistek::showConfDialog(void)
{
    ito::RetVal retValue(ito::retOk);

    DialogVistek *confDialog = new DialogVistek();
    connect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), confDialog, SLOT(valuesChanged(QMap<QString, ito::Param>)));
    QMetaObject::invokeMethod(this, "sendParameterRequest");

    if (confDialog->exec())
    {
        disconnect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), confDialog, SLOT(valuesChanged(QMap<QString, ito::Param>)));
        confDialog->sendVals(this);
    }
    else
    {
        disconnect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), confDialog, SLOT(valuesChanged(QMap<QString, ito::Param>)));
    }
    delete confDialog;

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! constructor for Vistek
/*!
    In the constructor the m_params-vector with all parameters, which are accessible by getParam or setParam, is built.
    Additionally the optional docking widget for the Vistek's toolbar is instantiated and created by createDockWidget.

    \param [in] uniqueID is an unique identifier for this Vistek-instance
    \param [in] parent is the parent object for this grabber (default: NULL)
    \sa ito::tParam, createDockWidget, setParam, getParam
*/
Vistek::Vistek(QObject *parent) : 
    AddInGrabber(),
    m_pVistekContainer(NULL),
    m_cam(SVGigE_NO_CAMERA),
    TriggerViolationCount(0),
    m_streamingChannel(SVGigE_NO_STREAMING_CHANNEL),
    m_eventID(SVGigE_NO_EVENT)
{
    m_acquiredImage.status = asNoImageAcquired;

    ito::Param paramVal("name", ito::ParamBase::String, "Vistek", NULL);
    m_params.insert(paramVal.getName(), paramVal);

    // Camera specific information
    paramVal = ito::Param("cameraModel", ito::ParamBase::String | ito::ParamBase::Readonly, "", tr("Camera Model ID").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("cameraManufacturer", ito::ParamBase::String | ito::ParamBase::Readonly, "", tr("Camera manufacturer").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("cameraVersion", ito::ParamBase::String | ito::ParamBase::Readonly, "", tr("Camera firmware version").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("cameraSerialNo", ito::ParamBase::String | ito::ParamBase::Readonly, "", tr("Serial number of the camera (see camera housing)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("cameraIP", ito::ParamBase::String | ito::ParamBase::Readonly, "", tr("IP adress of the camera").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("camnum", ito::ParamBase::Int, 0, 63, 0, tr("Camera Number").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("exposure", ito::ParamBase::Double, 0.00001, 2.0, 0.0, tr("Exposure time in [s]").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("gain", ito::ParamBase::Double, 0.0, 18.0, 0.0, tr("Gain [0..18 dB]").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("binning", ito::ParamBase::Int, 0, 5, 0, tr("Binning mode (OFF = 0 [default], HORIZONTAL = 1, VERTICAL = 2,  2x2 = 3, 3x3 = 4, 4x4 = 5").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("sizex", ito::ParamBase::Int | ito::ParamBase::Readonly, 1, 4096, 1024, tr("Width of current camera frame").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("sizey", ito::ParamBase::Int | ito::ParamBase::Readonly, 1, 4096, 1024, tr("Height of current camera frame").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("bpp", ito::ParamBase::Int, 8, 64, 8, tr("bit-depth for camera buffer").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("timestamp", ito::ParamBase::Double | ito::ParamBase::Readonly, 0.0, 10000000.0, 0.0, tr("Time in ms since last image (end of exposure)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("streamingPacketSize", ito::ParamBase::Int | ito::ParamBase::Readonly, 0, 16000, 1500, tr("Used streaming packet size (in bytes, more than 1500 usually only possible if you enable jumbo-frames at your network adapter)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    //now create dock widget for this plugin
    DockWidgetVistek *dw = new DockWidgetVistek();
    
    Qt::DockWidgetAreas areas = Qt::AllDockWidgetAreas;
    QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
    createDockWidget(QString(m_params["name"].getVal<char *>()), features, areas, dw);   

}

//----------------------------------------------------------------------------------------------------------------------------------
//! This is the destructor of the Vistek class.
/*!
    \sa ~AddInBase
*/
Vistek::~Vistek()
{
   m_params.clear();
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Returns parameter of m_params with key name.
/*!
    This method copies the string of the corresponding parameter to val with a maximum length of maxLen.

    \param [in,out] val is a shared-pointer pointing to a Param.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return ito::RetVal retOk in case that everything is ok, else retError
    \sa ito::Param, setParam ItomSharedSemaphore
*/
ito::RetVal Vistek::getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond)
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

    if(retValue == ito::retOk)
    {
        //gets the parameter key from m_params map (read-only is allowed, since we only want to get the value).
        retValue += apiGetParamFromMapByKey(m_params, key, it, false);
    }

    if(!retValue.containsError())
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
//! Sets parameter of m_params with key name. 
/*!
    This method copies the given string of the char pointer val with a size of len to the m_params-parameter.

    \param [in] val is the pointer to the ParamBase.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return ito::RetVal retOk in case that everything is ok, else retError
    \sa ito::Param, ItomSharedSemaphore
*/
ito::RetVal Vistek::setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond)
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
    
    if(!retValue.containsError())
    {
        //gets the parameter key from m_params map (read-only is not allowed and leads to ito::retError).
        retValue += apiGetParamFromMapByKey(m_params, key, it, true);
    }

    if(!retValue.containsError())
    {
        //here the new parameter is checked whether its type corresponds or can be cast into the
        // value in m_params and whether the new type fits to the requirements of any possible
        // meta structure.
        retValue += apiValidateParam(*it, *val, false, true);
    }

    if(!retValue.containsError())
    {
        bool set = false;
        if(!key.compare("exposure"))
        {
            // Camera_setExposureTime expects �s, so multiply by 10^6
            retValue += checkError("set exposure time",Camera_setExposureTime(m_cam, val->getVal<double>()*1.e6));
        }
        else if (!key.compare("gain"))
        {
            // Camera_setGain expects a value between 0 and 18 in dB
            retValue += checkError("set gain", Camera_setGain(m_cam, val->getVal<double>()));
        }
        else if(!key.compare("binning"))
        {
            BINNING_MODE mode;
            BINNING_MODE currentMode;
            Camera_getBinningMode(m_cam, &currentMode);

            switch (val->getVal<int>())
            {
            case 0:
                mode = BINNING_MODE_OFF;
                break;
            case 1:
                mode = BINNING_MODE_HORIZONTAL;
                break;
            case 2:
                mode = BINNING_MODE_VERTICAL;
                break;
            case 3:
                mode = BINNING_MODE_2x2;
                break;
            case 4:
                mode = BINNING_MODE_3x3;
                break;
            case 5:
                mode = BINNING_MODE_4x4;
                break;
            default:
                retValue += ito::RetVal(ito::retError,0,"binning invalid: Accepted values are OFF = 0 [default], HORIZONTAL = 1, VERTICAL = 2,  2x2 = 3, 3x3 = 4, 4x4 = 5");
                break;
            }

            if (!retValue.containsError() && mode != currentMode)
            {
                m_binningMode = mode;
                // Set new binning mode if neccessary. On failure set curval to 101, as the camera might not accept the new binning.
                // The API has no function to check if a binning mode is valid other than failing to set it...
                retValue += stopStreamAndDeleteCallbacks();
                retValue += checkError("set binning",Camera_setBinningMode(m_cam, mode));
                retValue += startStreamAndRegisterCallbacks();
                
                set = true;
            }      
        }
        else if(!key.compare("bpp"))
        {
            SVGIGE_PIXEL_DEPTH depth;
            switch (val->getVal<int>())
            {
            case 8:
                if (m_features.has8bit == false)
                {
                    retValue += ito::RetVal(ito::retError,0,"8bit not supported by this camera");
                }
                depth = SVGIGE_PIXEL_DEPTH_8;
                break;
            case 10:
                if (m_features.has10bit == false)
                {
                    retValue += ito::RetVal(ito::retError,0,"10bit not supported by this camera");
                }
                retValue += ito::RetVal(ito::retError,0,"10bit not supported by this driver version");
                //depth = SVGIGE_PIXEL_DEPTH_10;
                break;
            case 12:
                if (m_features.has12bit == false)
                {
                    retValue += ito::RetVal(ito::retError,0,"12bit not supported by this camera");
                }
                depth = SVGIGE_PIXEL_DEPTH_12;
                break;
            case 16:
                if (m_features.has16bit == false)
                {
                    retValue += ito::RetVal(ito::retError,0,"16bit not supported by this camera");
                }
                depth = SVGIGE_PIXEL_DEPTH_16;
                break;
            default:
                retValue += ito::RetVal(ito::retError,0,"unknown bpp value (use 8bit, 10bit, 12bit or 16bit)");
                break;
            }

            if (!retValue.containsError())
            {
                retValue += stopStreamAndDeleteCallbacks();
                retValue += checkError("set pixel depth",Camera_setPixelDepth(m_cam, depth));
                retValue += startStreamAndRegisterCallbacks();
            }      
        }

        if (!retValue.containsError() && !set) //binning is already set earlier
        {
            retValue += it->copyValueFrom( &(*val) );
        }
    }

    if(!retValue.containsError())
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
//! The init method is called by the addInManager after the initiation of a new instance of Vistek.
/*!
    This init method gets the mandatory and optional parameter vectors of type tParam and must copy these given parameters to the
    internal m_params-vector. Notice that this method is called after that this instance has been moved to its own (non-gui) thread.

    \param [in] paramsMand is a pointer to the vector of mandatory ParamBase.
    \param [in] paramsOpt is a pointer to the vector of optional ParamBase.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return ito::RetVal retOk
*/
ito::RetVal Vistek::init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond)
{
    int NumberOfCams = 0;
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal ReturnValue = ito::retOk;
    int camNum = -1;
    VistekCam TempCam;

    //*********************************************
    m_params["cameraSerialNo"].setVal<char*>( paramsOpt->at(0).getVal<char*>() );

    m_params["streamingPacketSize"].setVal<int>( paramsOpt->at(1).getVal<int>() );

    m_pVistekContainer = VistekContainer::getInstance();
    ReturnValue += m_pVistekContainer->initCameraContainer();

    if (!ReturnValue.containsError())
    {
        // Check if a valid Serial number is specified
        if (m_params["cameraSerialNo"].getVal<char*>() != NULL && m_params["cameraSerialNo"].getVal<char*>() != "")
        {
            camNum = m_pVistekContainer->getCameraBySN(m_params["cameraSerialNo"].getVal<char*>());
        }

        if ( camNum < 0)
        {
            camNum = m_pVistekContainer->getNextFreeCam();
        }

        if ( camNum >= 0)
        {
            m_params["camnum"].setVal<int>(camNum);

            // Get camera info
            TempCam = m_pVistekContainer->getCamInfo(camNum);
            m_params["cameraModel"].setVal<char*>(TempCam.camModel.toAscii().data());
            m_params["cameraSerialNo"].setVal<char*>(TempCam.camSerialNo.toAscii().data());
            m_params["cameraVersion"].setVal<char*>(TempCam.camVersion.toAscii().data());
            m_params["cameraIP"].setVal<char*>(TempCam.camIP.toAscii().data());
            m_params["cameraManufacturer"].setVal<char*>(TempCam.camManufacturer.toAscii().data());
            m_identifier = TempCam.camSerialNo;

            ReturnValue += initCamera(camNum);

            /*if (!ReturnValue.containsError())
            {
                setParam( QSharedPointer<ito::ParamBase>(new ito::ParamBase("exposure",m_params["exposure"].getType(), 0.06)));
                setParam( QSharedPointer<ito::ParamBase>(new ito::ParamBase("gain", m_params["gain"].getType(), 0.0)));
                setParam( QSharedPointer<ito::ParamBase>(new ito::ParamBase("binning", m_params["binning"].getType(), 101)));
            }*/
        }
        else
        {
            ReturnValue += ito::RetVal(ito::retError,0,tr("No free camera found").toAscii().data());
        }
        //*********************************************
    }

    if(waitCond)
    {
        waitCond->returnValue = ReturnValue;
        waitCond->release();
    }


    if(!ReturnValue.containsError())
    {
        emit parametersChanged(m_params); //send changed parameters to any connected dialogs or dock-widgets
    }

    setInitialized(true); //init method has been finished (independent on retval)

    return ReturnValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! The close method is called before an instance is deleted by the VistekInterface
/*!
    Notice that this method is called in the actual thread of the instance.

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return ito::RetVal retOk
    \sa ItomSharedSemaphore
*/
ito::RetVal Vistek::close(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);

    if(m_timerID > 0)
    {
        killTimer(m_timerID);
        m_timerID = 0;
    }

    // ONE TIME DEINIT HERE (Holger)
    //*********************************************
    if( m_cam != SVGigE_NO_CAMERA)
    {
        std::cout << "Forcing camera shutdown.\n" << std::endl;

        //stop camera
        Camera_setAcquisitionMode(m_cam, ACQUISITION_MODE_NO_ACQUISITION);
        //invalidate image buffer
        m_acquiredImage.status = asNoImageAcquired;
        // Unregister callbacks
        Stream_unregisterEventCallback(m_streamingChannel, m_eventID, &MessageCallback);
        Stream_closeEvent(m_streamingChannel, m_eventID);
        m_eventID = SVGigE_NO_EVENT;

        // delete stream
        StreamingChannel_delete(m_streamingChannel);
        m_streamingChannel = SVGigE_NO_STREAMING_CHANNEL;

        Camera_closeConnection(m_cam); // This is necessary to prevent an access violation if m gets terminated with an open cam connection
        m_cam = SVGigE_NO_CAMERA;
        m_pVistekContainer->freeCameraStatus(m_params["camnum"].getVal<int>());
    }
    //*********************************************

    if(waitCond)
    {
        waitCond->returnValue = ito::retOk;
        waitCond->release();

        return waitCond->returnValue;
    }
    else
    {
        return ito::retOk;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Error conversion between SVGigE_RETURN and ito::RetVal
/*!
    Returns retOk if returnCode is SVGigE_SUCCESS, else retError with appropriate error message
    \return ito::RetVal
*/
ito::RetVal Vistek::checkError(const char *prependStr, SVGigE_RETURN returnCode)
{
    ito::RetVal retval;
    if (returnCode != SVGigE_SUCCESS)
    {
        const char *str = prependStr;
        if (prependStr == NULL)
        {
            str = "";
        }

        char *msg = Error_getMessage(returnCode);
        if (msg)
        {
            retval += ito::RetVal::format(ito::retError,returnCode, "%s: Vistek DLL error %i '%s' occurred", str, returnCode, msg);
        }
        else
        {
            retval += ito::RetVal::format(ito::retError,returnCode, "%s: unknown Vistek DLL error %i occurred", str, returnCode);
        }
    }
    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! With startDevice the camera aquisition is started.
/*!
    This method sets the acquisition mode of the camera to software triggering.

    \note This method is similar to VideoCapture::open() of openCV

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return ito::RetVal retOk if starting was successfull, retWarning if startDevice has been calling at least twice.
    \sa stopDevice
*/
ito::RetVal Vistek::startDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue = ito::retOk;

    if (m_streamingChannel == SVGigE_NO_STREAMING_CHANNEL)
    {
        retValue += ito::RetVal(ito::retError, 0, "streaming server not started");
    }
    else
    {
        //streaming server started, events connected, timeout set, m_data is correct, intermediate buffer is ok

        incGrabberStarted();

        if(grabberStartedCount() == 1)
        {
            //*********************************************
            if( m_cam != SVGigE_NO_CAMERA)
            {
                retValue += checkError("set software trigger and start 1", Camera_setAcquisitionMode(m_cam, ACQUISITION_MODE_SOFTWARE_TRIGGER));
                retValue += checkError("set software trigger and start 2", Camera_setAcquisitionControl(m_cam, ACQUISITION_CONTROL_START));
            }
            //*********************************************
        }

        // Dont count a grabber that is not really started ...
        if ( retValue != ito::retOk )
        {
            decGrabberStarted();
        }
    }

    if(waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}
         
//----------------------------------------------------------------------------------------------------------------------------------
//! With stopDevice the camera device is stopped (opposite to startDevice)
/*!
    This method sets the acquisition mode of the camera to off.

    \note This method is similar to VideoCapture::release() of openCV

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return ito::RetVal retOk if everything is ok, retError if camera wasn't started before
    \sa startDevice
*/
ito::RetVal Vistek::stopDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue = ito::retOk;

    decGrabberStarted();
    if(grabberStartedCount() == 0)
    {
        //*********************************************
        if( m_cam != SVGigE_NO_CAMERA)
        {
            retValue += Camera_setAcquisitionControl(m_cam, ACQUISITION_CONTROL_STOP);
        }
        //*********************************************
    }
    else if(grabberStartedCount() < 0)
    {
        retValue += ito::RetVal(ito::retError, 1001, tr("StopDevice of Vistek can not be executed, since camera has not been started.").toAscii().data());
        setGrabberStarted(0);
    }


    if(waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}
         
//----------------------------------------------------------------------------------------------------------------------------------
//! This method triggers a new data acquisition.
/*!
    With this method a new data acquisition is triggered by the camera, that means the acquisition of the data starts at the moment, this method is called.
    The new data is then stored either in internal camera memory or in internal memory of this class.

    \note This method is similar to VideoCapture::grab() of openCV

    \param [in] trigger may describe the trigger parameter (unused here)
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return ito::RetVal retOk if everything is ok, retError if camera has not been started or an older data lies in memory which has not be fetched by getVal, yet.
    \sa getVal, retrieveData
*/
ito::RetVal Vistek::acquire(const int trigger, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue = ito::retOk;
    
    int timeout = 0;

    if(grabberStartedCount() <= 0)
    {
        retValue += ito::RetVal(ito::retError, 1002, tr("acquisition not possible, since Vistek camera has not been started.").toAscii().data());
    }
    else
    {
        bool b;
        SVGigE_RETURN ret2 = StreamingChannel_getReadoutTransfer(m_streamingChannel, &b);
        m_acquiredImage.status = asWaitingForTransfer; //start acquisition
        SVGigE_RETURN ret = Camera_softwareTrigger(m_cam);
        if (ret != SVGigE_SUCCESS)
        {
            m_acquiredImage.status = asNoImageAcquired;
            retValue += checkError("Camera trigger failed.",ret);
        }
    }

    if(waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    /*if (retValue == ito::retOk)
    {
        while(!FrameCompletedFlag)
        {
            if (timeout > 2000)
            {
                std::cout<<"timeout\n";
                break;
            }
            Sleep(1);
            timeout++;
        }
        FrameCompletedFlag = false;
    }*/

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! This method copies the acquired image data to externalDataObject or m_data.
/*!
    If *externalDataObject is NULL then it will be reassigned to &m_data so the image data is copied to m_data instead.

    \param [in] *externalDataObject is the data object where the image is to be stored. (Defaults to NULL)
    \return ito::RetVal retOk if everything is ok, retError if camera has not been started or no image has been acquired.
    \sa DataObject, getVal, acquire
*/
ito::RetVal Vistek::retrieveData(ito::DataObject *externalDataObject)
{
    ito::RetVal retValue(ito::retOk);

    bool hasListeners = false;
    bool copyExternal = false;

    SVGigE_RETURN ret;

    if(m_autoGrabbingListeners.size() > 0)
    {
        hasListeners = true;
    }

    if(externalDataObject != NULL)
    {
        copyExternal = true;
    }

    if(grabberStartedCount() <= 0)
    {
        retValue += ito::RetVal(ito::retError, 1002, tr("getVal of Vistek can not be executed, since camera has not been started.").toAscii().data());
    }
    if(m_acquiredImage.status == asNoImageAcquired)
    {
        retValue += ito::RetVal(ito::retError, 1002, tr("getVal of Vistek can not be executed, since no image has been acquired.").toAscii().data());
    }
    else
    {   
        int timeout = 0;
        while(m_acquiredImage.status == asWaitingForTransfer)
        {
            if (timeout > 2000)
            {
                std::cout<<"timeout\n";
                m_acquiredImage.status = asTimeout;
                break;
            }
            Sleep(1);
            timeout++;
        }

        if (m_acquiredImage.status == asImageReady && m_acquiredImage.sizex >= 0)
        {
            if(m_data.getType() == ito::tUInt8)
            {
                if(copyExternal) retValue += externalDataObject->copyFromData2D<ito::uint8>((ito::uint8*) m_acquiredImage.buffer.data(), m_acquiredImage.sizex, m_acquiredImage.sizey);
                if(!copyExternal || hasListeners) retValue += m_data.copyFromData2D<ito::uint8>((ito::uint8*) m_acquiredImage.buffer.data(), m_acquiredImage.sizex, m_acquiredImage.sizey);
            }
            else if(m_data.getType() == ito::tUInt16)
            {
                if(copyExternal) retValue += externalDataObject->copyFromData2D<ito::uint16>((ito::uint16*) m_acquiredImage.buffer.data(), m_acquiredImage.sizex, m_acquiredImage.sizey);
                if(!copyExternal || hasListeners) retValue += m_data.copyFromData2D<ito::uint16>((ito::uint16*) m_acquiredImage.buffer.data(), m_acquiredImage.sizex, m_acquiredImage.sizey);            
            }
            else
            {
                retValue += ito::RetVal(ito::retError, 1002, tr("copy buffer during getVal of Vistek can not be executed, since no DataType unknown.").toAscii().data());
            }

            m_acquiredImage.status = asNoImageAcquired; //release frame
        }
        else if (m_acquiredImage.sizex == asWaitingForTransfer)
        {
            retValue += ito::RetVal(ito::retError,0,"invalid image data");
        }
        else if (m_acquiredImage.status == asTimeout)
        {
            retValue += ito::RetVal(ito::retError,0,"timeout while retrieving image");
        }
        else
        {
            retValue += ito::RetVal::format(ito::retError,0,"error while retrieving image: %i", (int)(m_acquiredImage.status)); //camera data could not be received", m_acquiredImage.status); //ito::RetVal(ito::retError, 1002, tr("getVal of Vistek can not be executed, since no Data has been acquired.").toAscii().data());
        }
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Returns the grabbed camera frame.
/*!
    This method copies the recently grabbed camera frame to the given DataObject. Therefore this camera size must fit to the data structure of the 
    DataObject.

    \note This method is similar to VideoCapture::retrieve() of openCV

    \param [in,out] vpdObj is the pointer to a given dataObject (this pointer should be cast to ito::DataObject*) where the acquired data is copied to.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError is camera has not been started or no data has been acquired by the method acquire.
    \sa DataObject, acquire, retrieveData
*/
ito::RetVal Vistek::getVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::DataObject *dObj = reinterpret_cast<ito::DataObject *>(vpdObj);

    ito::RetVal retValue(ito::retOk);

    retValue += retrieveData();

    if(!retValue.containsError())
    {

        if(dObj == NULL)
        {
            retValue += ito::RetVal(ito::retError, 1004, tr("data object of getVal is NULL or cast failed").toAscii().data());
        }
        else
        {
            retValue += sendDataToListeners(0);

            (*dObj) = this->m_data;
        }

    }

    if (waitCond) 
    {
        waitCond->returnValue=retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Returns the grabbed camera frame as a deep copy.
/*!
    This method copies the recently grabbed camera frame to the given DataObject. Therefore this camera size must fit to the data structure of the 
    DataObject.

    \note This method is similar to VideoCapture::retrieve() of openCV

    \param [in,out] vpdObj is the pointer to a given dataObject (this pointer should be cast to ito::DataObject*) where the acquired image is deep copied to.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError is camera has not been started or no image has been acquired by the method acquire.
    \sa DataObject, acquire
*/
ito::RetVal Vistek::copyVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    ito::DataObject *dObj = reinterpret_cast<ito::DataObject *>(vpdObj);
    
    if(!dObj)
    {
        retValue += ito::RetVal(ito::retError, 0, tr("Empty object handle retrieved from caller").toAscii().data());
    }
    else
    {
        retValue += checkData(dObj);  //intermediate buffer is already ok
    }

    if(!retValue.containsError())
    {
        retValue += retrieveData(dObj);  
    }

    if(!retValue.containsError())
    {
        sendDataToListeners(0); //don't wait for live image, since user should get the image as fast as possible.
    }

    if (waitCond) 
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! This method is only called during init and initializes the camera with the number CameraNumber.
/*!
    If the camera has no valid IP address, it is forced to a new one. The DataCallback for the camera is registered here.
    \sa DataCallback, init
*/
ito::RetVal Vistek::initCamera(int CameraNumber)
{
    // Some Variables that might be needed
    SVGigE_RETURN SVGigERet;
    unsigned int IPAddress = 0;
    unsigned int SubnetMask = 0;
    char Msg[256];
    ito::RetVal retval;

    // Check if there already is an open handle for a camera
    if( m_cam != SVGigE_NO_CAMERA )
    {
        // TODO: Shutdown the cam first.
        std::cout << "Camera handle exists, trying to close an existing connection." << std::endl;
        StreamingChannel_delete(m_streamingChannel);
        Camera_closeConnection(m_cam);
        m_cam = SVGigE_NO_CAMERA;
    }

    // Get a Camera Handle
    std::cout << "Getting camera handle ... " << std::endl;
    m_cam = CameraContainer_getCamera(m_pVistekContainer->getCameraContainerHandle(), CameraNumber);
    if( SVGigE_NO_CAMERA == m_cam )
    {
        return ito::RetVal(ito::retError, 1004, tr("Requested camera could not be selected.").toAscii().data());
    }
    std::cout << "done!\n" << std::endl;

    // Open the camera connection
    std::cout << "Opening camera connection ... " << std::endl;
    if( SVGigE_SUCCESS != Camera_openConnection(m_cam, 30) )
    {
        std::cout << "\nOpening camera failed. Trying to enforce valid network settings.\n" << std::endl;
        // Check for valid network settings & doing necessary steps
        SVGigERet = Camera_forceValidNetworkSettings(m_cam, &IPAddress, &SubnetMask);
        if( SVGigERet == SVGigE_SVCAM_STATUS_CAMERA_OCCUPIED )
        {
            retval += checkError("Requested camera is occupied by another application", SVGigERet);
        }
        else
        {
            retval += checkError("Enforcing valid network settings failed", SVGigERet);
        }

        if (!retval.containsError())
        {

            // Try to open the connection again
            SVGigERet = Camera_openConnection(m_cam, 30);
            retval += checkError("Selected camera could not be opened.", SVGigERet);

            // print out the new valid network settings
            memset(Msg,0,sizeof(Msg));
            sprintf_s(Msg,255,"Camera has been forced to a valid IP '%d.%d.%d.%d'\n", 
                (IPAddress >> 24)%256, (IPAddress >> 16)%256, (IPAddress >> 8)%256, IPAddress%256);
            std::cout << Msg << std::endl;
        }
    }

    if (!retval.containsError())
    {
        //check some necessary features
        if (!Camera_isCameraFeature(m_cam, CAMERA_FEATURE_SOFTWARE_TRIGGER))
        {
            retval += ito::RetVal(ito::retError,0,"Camera does not support software triggers. This camera cannot be used by this plugin.");
        }

    }

    if (!retval.containsError())
    {
        retval += checkError("",Camera_setAcquisitionMode(m_cam, ACQUISITION_MODE_NO_ACQUISITION));
        retval += checkError("",Camera_setAcquisitionControl(m_cam, ACQUISITION_CONTROL_STOP));

        m_features.adjustExposureTime = Camera_isCameraFeature(m_cam, CAMERA_FEATURE_EXPOSURE_TIME);
        m_features.adjustGain = Camera_isCameraFeature(m_cam, CAMERA_FEATURE_GAIN);
        m_features.adjustBinning = Camera_isCameraFeature(m_cam, CAMERA_FEATURE_BINNING);
        m_features.has8bit = Camera_isCameraFeature(m_cam, CAMERA_FEATURE_COLORDEPTH_8BPP);
        m_features.has10bit = Camera_isCameraFeature(m_cam, CAMERA_FEATURE_COLORDEPTH_10BPP);
        m_features.has12bit = Camera_isCameraFeature(m_cam, CAMERA_FEATURE_COLORDEPTH_12BPP);
        m_features.has16bit = Camera_isCameraFeature(m_cam, CAMERA_FEATURE_COLORDEPTH_16BPP);

        bool autoVal;
        float valMin, valMax, val;

        //check auto gain/exposure
        //set auto gain to false
        Camera_getAutoGainEnabled(m_cam, &autoVal);
        if (autoVal)
        {
            std::cout << "auto gain has been enabled, will be disabled" << std::endl;
            Camera_setAutoGainEnabled(m_cam, false);
        }

        //obtain gain value
        Camera_getGainMax(m_cam, &valMax);
        Camera_getGain(m_cam, &val);
        ito::DoubleMeta *dm = (ito::DoubleMeta*)(m_params["gain"].getMeta());
        dm->setMin(0.0);
        dm->setMax(valMax);
        m_params["gain"].setVal<double>(val);
        if (m_features.adjustGain == false)
        {
            m_params["gain"].setFlags(ito::ParamBase::Readonly);
        }

        //obtain exposure value
        Camera_getExposureTimeRange(m_cam, &valMin, &valMax);
        Camera_getExposureTime(m_cam, &val);
        dm = (ito::DoubleMeta*)(m_params["exposure"].getMeta());
        dm->setMin(valMin / 1.e6);
        dm->setMax(valMax / 1.e6);
        m_params["exposure"].setVal<double>(val / 1.e6);
        if (m_features.adjustExposureTime == false)
        {
            m_params["exposure"].setFlags(ito::ParamBase::Readonly);
        }

        //binning and bpp is read in registerCallbacks later
        if (m_features.adjustBinning == false)
        {
            m_params["binning"].setFlags(ito::ParamBase::Readonly);
        }

        std::cout << "done!\n" << std::endl;

        retval += startStreamAndRegisterCallbacks();
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
void Vistek::updateTimestamp()
{
    double timestamp = (MessageTimestampStartOfTransfer - MessageTimestampLastStartOfTransfer) * 1000;
    m_params["timestamp"].setVal<double>(timestamp);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal Vistek::stopStreamAndDeleteCallbacks()
{
    ito::RetVal retValue;
    //1. first check if there is already a stream initialized and if so, delete it
    if (m_streamingChannel != SVGigE_NO_STREAMING_CHANNEL)
    {
        //stop camera
        //Camera_setAcquisitionMode(m_cam, ACQUISITION_MODE_NO_ACQUISITION);
        //Camera_setAcquisitionControl(m_cam, ACQUISITION_CONTROL_STOP);

        //invalidate image buffer
        m_acquiredImage.status = asNoImageAcquired;
        // Unregister callbacks
        Stream_unregisterEventCallback(m_streamingChannel, m_eventID, &MessageCallback);
        Stream_closeEvent(m_streamingChannel, m_eventID);
        m_eventID = SVGigE_NO_EVENT;

        // delete stream
        StreamingChannel_delete(m_streamingChannel);
        m_streamingChannel = SVGigE_NO_STREAMING_CHANNEL;
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal Vistek::startStreamAndRegisterCallbacks()
{
    ito::RetVal retval;
    SVGigE_RETURN SVGigERet;
    int sizex, sizey;    

    //1. first check if there is already a stream initialized and if so, delete it
    if (m_streamingChannel != SVGigE_NO_STREAMING_CHANNEL)
    {
        retval += stopStreamAndDeleteCallbacks();
    }

    //2. sychronize current settings of camera with m_params
    {
        // Obtain geometry data
        SVGigERet = Camera_getSizeX(m_cam, &sizex);
        retval += checkError("Error getting image size", SVGigERet);
        SVGigERet = Camera_getSizeY(m_cam, &sizey);
        retval += checkError("Error getting image size", SVGigERet);

        m_params["sizex"].setVal<int>(sizex);
        m_params["sizey"].setVal<int>(sizey);

        //obtain binning mode
        BINNING_MODE currentBinning;
        Camera_getBinningMode(m_cam, &currentBinning);
        m_params["binning"].setVal<int>( (int)currentBinning);

        GVSP_PIXEL_TYPE pixeltype;
        Camera_getPixelType(m_cam, &pixeltype);

        switch (pixeltype)
        {
        case GVSP_PIX_MONO8:
            m_params["bpp"].setVal<int>(8);
            break;
        /*case GVSP_PIX_MONO10: //not supported by current Vistek driver
        case GVSP_PIX_MONO10_PACKED:
            m_params["bpp"].setVal<int>(10);
            break;*/
        case GVSP_PIX_MONO12:
        case GVSP_PIX_MONO12_PACKED:
            m_params["bpp"].setVal<int>(12);
            break;
        case GVSP_PIX_MONO16:
            m_params["bpp"].setVal<int>(16);
            break;
        default:
            retval += ito::RetVal(ito::retError,0,"given pixeltype not supported. Supported is only MONO8, MONO12, MONO12_PACKED and MONO16");
            break;
        }

        // Get Tick Frequency
        SVGigERet = Camera_getTimestampTickFrequency(m_cam, &TimestampTickFrequency);
        if (SVGigERet != SVGigE_SUCCESS)
        {
            std::cout<<"TimestampTickFrequency: "<<TimestampTickFrequency<<"\n";
        }
    }

    //3. determine (and set) maximal possible network packet size (if you have jumbo-frames enabled)
    Camera_setMulticastMode(m_cam, MULTICAST_MODE_NONE); //image is only delivered to this computer, not to other computers... therefore we can also (usually) use the maximum available package size

    int maximalPacketSize;
    SVGigERet = Camera_evaluateMaximalPacketSize(m_cam, &maximalPacketSize);
    retval += checkError("maximal packet size determination", SVGigERet);

    if (!retval.containsError())
    {
        ((ito::IntMeta*)(m_params["streamingPacketSize"].getMeta()))->setMax(maximalPacketSize);
        if (m_params["streamingPacketSize"].getVal<int>() > maximalPacketSize)
        {
            m_params["streamingPacketSize"].setVal<int>(maximalPacketSize); //maximum is already set as default
        }
        else if (m_params["streamingPacketSize"].getVal<int>() == -1)
        {
            m_params["streamingPacketSize"].setVal<int>(maximalPacketSize); //maximum is already set as default
        }
        //else
        //{
            SVGigERet = Camera_setStreamingPacketSize(m_cam, m_params["streamingPacketSize"].getVal<int>());
            retval += checkError("set streaming packet size", SVGigERet);
        //}
    }

    //4. Register data callback
    if (!retval.containsError())
    {
        // Register data callback
        std::cout << "Registering data callback ..." << std::endl;
        SVGigERet = StreamingChannel_create (&m_streamingChannel,                                // a streaming channel handle will be returned
                                    m_pVistekContainer->getCameraContainerHandle(),                 // a valid camera container client handle
                                    m_cam,                                                          // a valid camera handle
                                    300,                                                              // buffer count 0 => 3 buffers (big buffer is necessary in order to avoid huge timeouts)
                                    &DataCallback,                                                  // callback function pointer where datas are delivered to
                                    this);                                                          // current class pointer will be passed through as context

        // Check if data callback registration was successful
        retval += checkError("Streaming channel creation failed", SVGigERet);

        if (SVGigERet != SVGigE_SUCCESS)
        {
            m_streamingChannel = SVGigE_NO_STREAMING_CHANNEL;
        }
    }

    //5. register events
    if (!retval.containsError())
    {
        std::cout << "done!\n" << std::endl;

        // Create event
        SVGigERet = Stream_createEvent(m_streamingChannel,&m_eventID,100);
        retval += checkError("Event creation failed", SVGigERet);

        if (!retval.containsError())
        {
            // Register messages
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_FRAME_COMPLETED);
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_START_OF_TRANSFER);
            /*Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_CAMERA_TRIGGER_VIOLATION);
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_FRAME_ABANDONED);
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_END_OF_EXPOSURE);*/

            /*Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_FRAME_COMPLETED);
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_FRAME_ABANDONED);
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_END_OF_EXPOSURE);
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_START_OF_TRANSFER);
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_BANDWIDTH_EXCEEDED);
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_OLD_STYLE_DATA_PACKETS );
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_TEST_PACKET);*/
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_CAMERA_IMAGE_TRANSFER_DONE);
            /*Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_CAMERA_CONNECTION_LOST);
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_MULTICAST_MESSAGE );
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_FRAME_INCOMPLETE );
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_MESSAGE_FIFO_OVERRUN );
            Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_CAMERA_SEQ_DONE);
            */Stream_addMessageType(m_streamingChannel,m_eventID,SVGigE_SIGNAL_CAMERA_TRIGGER_VIOLATION);

            // Register message callback
            std::cout << "Registering message callback ..." << std::endl;
            SVGigERet = Stream_registerEventCallback(m_streamingChannel, m_eventID, &MessageCallback, this);
            // Check if message callback registration was successful
            retval += checkError("Message callback registration failed", SVGigERet);
        }
    }

    //6. prepare m_data and intermediate buffer structure
    if (!retval.containsError())
    {
        retval += checkData();

        if (m_params["bpp"].getVal<int>() == 8)
        {
            m_acquiredImage.buffer.resize( sizex * sizey ); //8bit content
        }
        else
        {
            m_acquiredImage.buffer.resize( 2 * sizex * sizey ); //2*8bit = 16bit content
        }

        m_acquiredImage.status = asNoImageAcquired;
    }

    if (!retval.containsError())
    {
        std::cout << "done!\n" << std::endl;
    }
    
    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
void Vistek::dockWidgetVisibilityChanged(bool visible)
{
    if (getDockWidget())
    {
        DockWidgetVistek *dw = qobject_cast<DockWidgetVistek*>(getDockWidget()->widget());
        if (visible)
        {
            connect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), dw, SLOT(valuesChanged(QMap<QString, ito::Param>)));
            connect(dw, SIGNAL(GainOffsetExposurePropertiesChanged(double,double,double)), this, SLOT(GainOffsetExposurePropertiesChanged(double,double,double)));

            emit parametersChanged(m_params);
        }
        else
        {
            disconnect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), dw, SLOT(valuesChanged(QMap<QString, ito::Param>)));
            disconnect(dw, SIGNAL(GainOffsetExposurePropertiesChanged(double,double,double)), this, SLOT(GainOffsetExposurePropertiesChanged(double,double,double)));
        }
    }
}

void Vistek::GainOffsetExposurePropertiesChanged(double gain, double /*offset*/, double exposure)
{
    ito::RetVal retValue;
        
    // Camera_setExposureTime expects �s, so multiply by 10^6
    retValue += checkError("set exposure time",Camera_setExposureTime(m_cam, exposure*1.e6));
        
    // Camera_setGain expects a value between 0 and 18 in dB
    retValue += checkError("set gain", Camera_setGain(m_cam, gain));

    emit parametersChanged(m_params);
}












//----------------------------------------------------------------------------------------------------------------------------------
SVGigE_RETURN __stdcall DataCallback(Image_handle data, void* context)
{
    // Check for valid context
    Vistek *v = reinterpret_cast<Vistek*>(context);
    if(v == NULL || data == NULL)
    {
        v->m_acquiredImage.sizex = -1;
        v->m_acquiredImage.sizey = -1;
        return SVGigE_ERROR;
    }

    if (v->m_acquiredImage.status == Vistek::asNoImageAcquired || v->m_acquiredImage.status == Vistek::asTimeout)
    {
        //nobody is waiting for an image -> kill it
        Image_release(data);
        return SVGigE_ERROR;
    }
    else
    {
        //the status is set by the messaging system
        v->m_acquiredImage.pixelType = Image_getPixelType(data);
        v->m_acquiredImage.sizex = Image_getSizeX(data);
        v->m_acquiredImage.sizey = Image_getSizeY(data);
        v->m_acquiredImage.dataID = Image_getImageID(data);
        v->m_acquiredImage.timestamp = Image_getTimestamp(data);
        v->m_acquiredImage.transferTime = Image_getTransferTime(data);
        v->m_acquiredImage.packetCount = Image_getPacketCount(data);

        size_t elems = v->m_acquiredImage.sizex * v->m_acquiredImage.sizey;
        
        if (v->m_acquiredImage.pixelType == GVSP_PIX_MONO8)
        {
            if (v->m_acquiredImage.buffer.size() != elems)
            {
                v->m_acquiredImage.buffer.resize(elems);
            }
            memcpy( v->m_acquiredImage.buffer.data(), Image_getDataPointer(data), sizeof(ito::uint8) * elems );
        }
        else if (v->m_acquiredImage.pixelType == GVSP_PIX_MONO12 || v->m_acquiredImage.pixelType == GVSP_PIX_MONO12_PACKED)
        {
            if (v->m_acquiredImage.buffer.size() != 2*elems)
            {
                v->m_acquiredImage.buffer.resize(2*elems);
            }
            Image_getImage12bitAs16bit(data, v->m_acquiredImage.buffer.data(), sizeof(ito::uint16) * elems);
        }
        else if (v->m_acquiredImage.pixelType == GVSP_PIX_MONO16)
        {
            if (v->m_acquiredImage.buffer.size() != 2 * elems)
            {
                v->m_acquiredImage.buffer.resize(2*elems);
            }
            memcpy( v->m_acquiredImage.buffer.data(), Image_getDataPointer(data), sizeof(ito::uint16) * elems );
        }
        else
        {
            Image_release(data);
            return SVGigE_ERROR;
        }

        v->m_acquiredImage.status = Vistek::asImageReady;

        Image_release(data);
    }

    return SVGigE_SUCCESS;
};

//----------------------------------------------------------------------------------------------------------------------------------
SVGigE_RETURN __stdcall MessageCallback(Event_handle eventID, void* context)
{
    // Check for valid context
    Vistek *v = reinterpret_cast<Vistek*>(context);
    if(v == NULL)
        return SVGigE_ERROR;

    Message_handle MessageID;
    SVGigE_SIGNAL_TYPE MessageType;

    // Get the signal
    if( SVGigE_SUCCESS != Stream_getMessage(v->m_streamingChannel,eventID,&MessageID,&MessageType) )
        return SVGigE_ERROR;

    qDebug() << "message:" << MessageType;

    // Get Message timestamp
    if(MessageType == SVGigE_SIGNAL_START_OF_TRANSFER)
    {
        v->MessageTimestampLastStartOfTransfer = v->MessageTimestampStartOfTransfer;
        Stream_getMessageTimestamp(v->m_streamingChannel,eventID,MessageID,&v->MessageTimestampStartOfTransfer);
        v->updateTimestamp();
    }
    //if(MessageType == SVGigE_SIGNAL_FRAME_COMPLETED)
    //{
    //    Stream_getMessageTimestamp(v->m_streamingChannel,eventID,MessageID,&v->MessageTimestampFrameCompleted);
    //}
    if(MessageType == SVGigE_SIGNAL_CAMERA_TRIGGER_VIOLATION)
    {
        // Count trigger violations in current streaming channel
        v->TriggerViolationCount++;
    }

    
    //if(MessageType == SVGigE_SIGNAL_FRAME_ABANDONED)
    //{
    //    // Not implemented yet
    //}
    if(MessageType == SVGigE_SIGNAL_END_OF_EXPOSURE)
    {
        //Stream_getMessageTimestamp(v->m_streamingChannel,eventID,MessageID, &v->MessageTimestampEndOfExposure);
    }

    // Release message
    Stream_releaseMessage(v->m_streamingChannel,eventID,MessageID);

    return SVGigE_SUCCESS;
}


