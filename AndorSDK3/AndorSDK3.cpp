/* ********************************************************************
    Plugin "AndorSDK3" for itom software
    URL: http://www.bitbucket.org/itom/plugins
	Copyright (C) 2014, Institut f�r Technische Optik, Universit�t Stuttgart

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

#include "AndorSDK3.h"
#include "pluginVersion.h"

#include <qstring.h>
#include <qstringlist.h>
#include <QtCore/QtPlugin>
#include <qmetaobject.h>

#include "DockWidgetAndorSDK3.h"
#include "DialogAndorSDK3.h"

#include "common/helperCommon.h"


/*static*/ int AndorSDK3::andorOpenedIndices[32] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,};

//----------------------------------------------------------------------------------------------------------------------------------
//! constructor for AndorSDK3
/*!
    In this constructor the m_params-vector with all parameters, which are accessible by getParam or setParam, is built.
    Additionally the optional docking widget for the AndorSDK3's toolbar is instantiated and created by createDockWidget.

    \param [in] uniqueID is an unique identifier for this AndorSDK3-instance
    \sa ito::tParam, createDockWidget, setParam, getParam
*/
AndorSDK3::AndorSDK3() :
    AddInGrabber(),
    m_handle(AT_HANDLE_UNINITIALISED),
    m_hBin(1),
    m_vBin(1)
{

    ito::Param paramVal("name", ito::ParamBase::String | ito::ParamBase::Readonly, "AndorSDK3", "plugin name");
    m_params.insert(paramVal.getName(), paramVal);
    
    paramVal = ito::Param("integration_time", ito::ParamBase::Double, 0.0, 1.0, 0.005, tr("Exposure time of chip (in seconds).").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);
    
    paramVal = ito::Param("binning", ito::ParamBase::Int, 101, 808, 101, tr("Horizontal and vertical binning, depending on camera ability. 104 means a 1x binning in horizontal and 4x binning in vertical direction. (only quadratic binning is allowed; if read only binning is not supported)").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("gain", ito::ParamBase::Double, 0.0, 1.0, 0.5, tr("Gain (normalized value 0..1)").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("sizex", ito::ParamBase::Int | ito::ParamBase::Readonly, 1, 2048, 2048, tr("Pixelsize in x (cols)").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("sizey", ito::ParamBase::Int | ito::ParamBase::Readonly, 1, 2048, 2048, tr("Pixelsize in y (rows)").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("x0", ito::ParamBase::Int, 0, 2047, 0, tr("Index of left boundary pixel within ROI").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("y0", ito::ParamBase::Int, 0, 2047, 0, tr("Index of top boundary pixel within ROI").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("x1", ito::ParamBase::Int, 0, 2047, 2047, tr("Index of right boundary pixel within ROI").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("y1", ito::ParamBase::Int, 0, 2047, 2047, tr("Index of bottom boundary pixel within ROI").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("bpp", ito::ParamBase::Int, 8, 16, 16, tr("Bitdepth of each pixel").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("timeout", ito::ParamBase::Double, 0.0, std::numeric_limits<double>::max(), 1.0, tr("acquisition timeout in secs").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("interface", ito::ParamBase::String | ito::ParamBase::Readonly, "[unknown]", tr("camera interface type").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("firmware_version", ito::ParamBase::String | ito::ParamBase::Readonly, "[unknown]", tr("firmware version").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("camera_model", ito::ParamBase::String | ito::ParamBase::Readonly, "[unknown]", tr("model name of camera").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("camera_name", ito::ParamBase::String | ito::ParamBase::Readonly, "[unknown]", tr("name of camera").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("serial_number", ito::ParamBase::String | ito::ParamBase::Readonly, "[unknown]", tr("serial number of camera").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("sdk_version", ito::ParamBase::String | ito::ParamBase::Readonly, "[unknown]", tr("Andor SDK3 version").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("trigger_mode", ito::ParamBase::String, "Software", tr("camera trigger (Interal, Software, External, External Start, External Exposure)").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("electronic_shuttering_mode", ito::ParamBase::Int, 0, 1, 0, tr("0: rolling shutter (for highest frame rate, best noise performance, default), 1: global shutter (for pulsed, fast moving images)").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);

    paramVal = ito::Param("full_aoi_control", ito::ParamBase::Int | ito::ParamBase::Readonly, 0, 1, 1, tr("indicates if full AOI control is available (usually yes, for some Neo cameras it isn't and you can only apply certain ROI sizes (see camera manual))").toLatin1().data());
    m_params.insert(paramVal.getName(), paramVal);


    //now create dock widget for this plugin
    DockWidgetSDK3 *dw = new DockWidgetSDK3(this);

    Qt::DockWidgetAreas areas = Qt::AllDockWidgetAreas;
    QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
    createDockWidget(QString(m_params["name"].getVal<char *>()), features, areas, dw);
}

//----------------------------------------------------------------------------------------------------------------------------------
//! destructor
/*!
    \sa ~AddInBase
*/
AndorSDK3::~AndorSDK3()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
//! init method which is called by the addInManager after the initiation of a new instance of AndorSDK3.
/*!
    This init method gets the mandatory and optional parameter vectors of type tParam and must copy these given parameters to the
    internal m_params-vector. Notice that this method is called after that this instance has been moved to its own (non-gui) thread.

    \param [in] paramsMand is a pointer to the vector of mandatory tParams.
    \param [in] paramsOpt is a pointer to the vector of optional tParams.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk
*/
ito::RetVal AndorSDK3::init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retVal;

    int desiredIndex = paramsMand->at(0).getVal<int>();

    if (AndorSDK3::andorOpenedIndices[desiredIndex] >= 0)
    {
        retVal += ito::RetVal::format(ito::retError, 0, "the camera with index %i is already opened", desiredIndex);
    }
    else
    {
        retVal += checkError(AT_InitialiseLibrary());
    }

    if (!retVal.containsError())
    {
        AT_64 iNumberDevices = 0;
        retVal += checkError(AT_GetInt(AT_HANDLE_SYSTEM, L"DeviceCount", &iNumberDevices));
        if (iNumberDevices <= 0)
        {
            retVal += ito::RetVal(ito::retError, 0, "no Andor SDK3 compatible cameras have been detected at this computer. Check all redistributable binaries \
                                                    have been copied to the executable directory or are in the system path and check atdebug.log file");
        }
    }

    if (!retVal.containsError())
    {
        retVal += AT_Open(desiredIndex, &m_handle);
    }

    if (!retVal.containsError())
    {
        m_cameraIndex = desiredIndex;
        AndorSDK3::andorOpenedIndices[m_cameraIndex] = 1;
    }

    if (!retVal.containsError())
    {
        //get sensor info
        retVal += loadSensorInfo();
        retVal += loadEnumIndices();

        //pre-set some settings (no error check, if the setting is not available, this doesn't matter)
        qDebug() << AT_SetBool(m_handle, L"VerticallyCentreAOI", false);
        if (AT_SetEnumIndex(m_handle, L"TriggerMode", m_triggerModeIdx.tSoftware) != AT_SUCCESS)
        {
            qDebug() << AT_SetEnumIndex(m_handle, L"TriggerMode", m_triggerModeIdx.tInternal);
        }
        qDebug() << AT_SetBool(m_handle, L"MetadataEnable", false);
        //Set the camera to continuously acquires frames 
        qDebug() << AT_SetEnumString(m_handle, L"CycleMode", L"Continuous");

        if (!retVal.containsError())
        {
            //try to set different color modes in order to check if they are supported
            ito::RetVal bppRet = ito::retError;
            if (m_pixelEncodingIdx.mono8 >= 0)
            {
                bppRet = checkError(AT_SetEnumIndex(m_handle, L"PixelEncoding", m_pixelEncodingIdx.mono8));
            }
            if (bppRet.containsError() && m_pixelEncodingIdx.mono12 >= 0)
            {
                bppRet = checkError(AT_SetEnumIndex(m_handle, L"PixelEncoding", m_pixelEncodingIdx.mono12));
            }
            if (bppRet.containsError() && m_pixelEncodingIdx.mono16 >= 0)
            {
                bppRet = checkError(AT_SetEnumIndex(m_handle, L"PixelEncoding", m_pixelEncodingIdx.mono16));
            }

            if (bppRet.containsError())
            {
                retVal += ito::RetVal(ito::retError, 0, "no of the pixel formats mono8, mono12 or mono16 can be set. Camera not supported");
            }
        }

        if (!retVal.containsError())
        {
            retVal += synchronizeCameraSettings();
        }

    }

    if(!retVal.containsError())
    {
        checkData(); //check if image must be reallocated
    }

    if(waitCond)
    {
        waitCond->returnValue = retVal;
        waitCond->release();
    }

    setInitialized(true); //init method has been finished (independent on retval)
    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! close method which is called before that this instance is deleted by the AndorSDK3Interface
/*!
    notice that this method is called in the actual thread of this instance.

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk
    \sa ItomSharedSemaphore
*/
ito::RetVal AndorSDK3::close(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;

    if (m_handle != AT_HANDLE_UNINITIALISED)
    {
        retValue += checkError(AT_Close(m_handle));
        AndorSDK3::andorOpenedIndices[m_cameraIndex] = -1;
        m_handle = AT_HANDLE_UNINITIALISED;
    }   

    retValue += checkError(AT_FinaliseLibrary());

    if(waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();

        return retValue;
    }
    else
    {
        return retValue;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
//! returns parameter of m_params with key name.
/*!
    This method copies the string of the corresponding parameter to val with a maximum length of maxLen.

    \param [in] name is the key name of the parameter
    \param [in,out] val is a shared-pointer of type char*.
    \param [in] maxLen is the maximum length which is allowed for copying to val
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk in case that everything is ok, else retError
    \sa ito::tParam, ItomSharedSemaphore
*/
ito::RetVal AndorSDK3::getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue;
    QString key;
    bool hasIndex = false;
    int index;
    QString suffix;
    QMap<QString,ito::Param>::iterator it;

    retValue += apiParseParamName(val->getName(), key, hasIndex, index, suffix);

    if(retValue == ito::retOk)
    {
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
//! sets parameter of m_params with key name.
/*!
    This method copies the given value  to the m_params-parameter.

    \param [in] name is the key name of the parameter
    \param [in] val is the double value to set.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk in case that everything is ok, else retError
    \sa ito::tParam, ItomSharedSemaphore
*/
ito::RetVal AndorSDK3::setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    QString key;
    bool hasIndex;
    int index;
    QString suffix;
    QMap<QString, ito::Param>::iterator it;
    int running = 0;

    //parse the given parameter-name (if you support indexed or suffix-based parameters)
    retValue += apiParseParamName( val->getName(), key, hasIndex, index, suffix );

    if(!retValue.containsError())
    {
        retValue += apiGetParamFromMapByKey(m_params, key, it, true);
    }

    if(!retValue.containsError())
    {
#if defined(ITOM_ADDININTERFACE_VERSION) && ITOM_ADDININTERFACE_VERSION < 0x010300
        //old style api, round the incoming double value to the allowed step size.
        //in a new itom api, this is automatically done by a new api function.
        if (val->getType() == ito::ParamBase::Double || val->getType() == ito::ParamBase::Int)
        {
            double value = val->getVal<double>();
            if (it->getType() == ito::ParamBase::Double)
            {
                ito::DoubleMeta *meta = (ito::DoubleMeta*)it->getMeta();
                if (meta)
                {
                    double step = meta->getStepSize();
                    if (step != 0.0)
                    {
                        int multiple = qRound((value - meta->getMin()) / step);
                        value = meta->getMin() + multiple * step;
                        value = qBound(meta->getMin(), value, meta->getMax());
                        val->setVal<double>(value);
                    }
                }
            }
        }
        retValue += apiValidateParam(*it, *val, false, true);
#else
        retValue += apiValidateAndCastParam(*it, *val, false, true, true);
#endif
    }

    if(!retValue.containsError())
    {
        if (key == "x0")
        {
            int offset = val->getVal<int>() * m_hBin;
            retValue += checkError(AT_SetInt(m_handle, L"AOILeft", offset));
            if (!retValue.containsError()) retValue += synchronizeCameraSettings(sRoi);
        }
        else if (key == "y0")
        {
            int offset = val->getVal<int>() * m_vBin;
            retValue += checkError(AT_SetInt(m_handle, L"AOITop", offset));
            if (!retValue.containsError()) retValue += synchronizeCameraSettings(sRoi);
        }
        else if (key == "x1")
        {
            int size = 1 + val->getVal<int>()  - m_params["x0"].getVal<int>();
            retValue += checkError(AT_SetInt(m_handle, L"AOIWidth", size));
            if (!retValue.containsError()) retValue += synchronizeCameraSettings(sRoi);
        }
        else if (key == "y1")
        {
            int size = 1 + val->getVal<int>()  - m_params["y0"].getVal<int>();
            retValue += checkError(AT_SetInt(m_handle, L"AOIHeight", size));
            if (!retValue.containsError()) retValue += synchronizeCameraSettings(sRoi);
        }
        else if (key == "integration_time")
        {
            double timeSec = val->getVal<double>();
            retValue += checkError(AT_SetFloat(m_handle, L"ExposureTime", timeSec));

            if (!retValue.containsError())
            {
                it->setVal<double>(timeSec); //after a call, timeMs contains the really set exposure time
            }
        }
        else if (key == "electronic_shuttering_mode")
        {
            if (val->getVal<int>() == 0) //rolling
            {
                if (m_electronicShutteringMode.tShutterRolling >= 0)
                {
                    retValue += checkError(AT_SetEnumIndex(m_handle, L"ElectronicShutteringMode", m_electronicShutteringMode.tShutterRolling));
                }
                else
                {
                    retValue += ito::RetVal(ito::retError, 0, "rolling shutter not available");
                }
            }
            else
            {
                if (m_electronicShutteringMode.tShutterGlobal >= 0)
                {
                    retValue += checkError(AT_SetEnumIndex(m_handle, L"ElectronicShutteringMode", m_electronicShutteringMode.tShutterGlobal));
                }
                else
                {
                    retValue += ito::RetVal(ito::retError, 0, "rolling shutter not available");
                }
            }

            if (!retValue.containsError())
            {
                it->setVal<int>(val->getVal<int>());
            }
        }
        //else if (key == "pixel_clock")
        //{
        //    UINT clock = val->getVal<int>();
        //    retValue += checkError(is_PixelClock(m_camera, IS_PIXELCLOCK_CMD_SET, (void*)&clock, sizeof(clock)));

        //    if (!retValue.containsError())
        //    {
        //        retValue += setMinimumFrameRate();
        //        retValue += synchronizeCameraSettings(sPixelClock | sExposure);
        //    }
        //}
        else if (key == "binning")
        {
            int b = val->getVal<int>();
            int v = b % 100;
            int h = (b-v) / 100;
            INT mode = 0;

            retValue += checkError(AT_SetInt(m_handle, L"AOIHBin", h));
            retValue += checkError(AT_SetInt(m_handle, L"AOIVBin", h));

            if (!retValue.containsError())
            {
                retValue += synchronizeCameraSettings(sBinning | sRoi);
            }
        }
        //else if (key == "gain")
        //{
        //    retValue += checkError(is_SetHardwareGain(m_camera, qRound(val->getVal<double>()*100.0), IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER));

        //    if (!retValue.containsError())
        //    {
        //        retValue += synchronizeCameraSettings(sGain);
        //    }
        //}
        //else if (key == "offset")
        //{
        //    if (m_blacklevelRange.s32Max == 0 && m_blacklevelRange.s32Min == 0)
        //    {
        //        retValue += ito::RetVal(ito::retError, 0, "no valid blacklevel offset range available");
        //    }
        //    else
        //    {
        //        INT offset = (m_blacklevelRange.s32Max - m_blacklevelRange.s32Min) * val->getVal<double>();
        //        offset = m_blacklevelRange.s32Min + offset - (offset % m_blacklevelRange.s32Inc);
        //        retValue += checkError(is_Blacklevel(m_camera, IS_BLACKLEVEL_CMD_SET_OFFSET, (void*)&offset, sizeof(offset)));
        //    }

        //    if (!retValue.containsError())
        //    {
        //        retValue += synchronizeCameraSettings(sOffset);
        //    }
        //}
        else if (key == "trigger_mode")
        {
            QString mode = val->getVal<char*>();
            AT_WC mode_[100];
            AT_WC wstring[100];
            int count;
            int len = mode.toWCharArray(mode_);
            mode_[len] = '\0';

            if (AT_GetEnumCount(m_handle, L"TriggerMode", &count) == AT_SUCCESS)
            {
                for (int c = 0; c < count; ++c)
                {
                    AT_GetEnumStringByIndex(m_handle, L"TriggerMode", c, wstring, 100);
                    if (wcscmp(wstring, mode_) == 0)
                    {
                        retValue += checkError(AT_SetEnumIndex(m_handle, L"TriggerMode", c));

                        if (!retValue.containsError())
                        {
                            QString v = QString::fromWCharArray(wstring);
                            it->setVal<char*>(v.toLatin1().data());
                        }
                    }
                }
            }
        }
        else if (key == "bpp")
        {
            
            switch (val->getVal<int>())
            {
            case 8:
                retValue += checkError(AT_SetEnumIndex(m_handle, L"PixelEncoding", m_pixelEncodingIdx.mono8));
                break;
            case 12:
                retValue += checkError(AT_SetEnumIndex(m_handle, L"PixelEncoding", m_pixelEncodingIdx.mono12));
                break;
            case 16:
                retValue += checkError(AT_SetEnumIndex(m_handle, L"PixelEncoding", m_pixelEncodingIdx.mono16));
                break;
            default:
                retValue += ito::RetVal(ito::retError, 0, "unsupported bitdepth for gray value color mode");
            }

            if (!retValue.containsError())
            {
                retValue += synchronizeCameraSettings(sBppAndPreAmpGain);
            }
        }
        else
        { 
            //e.g. timeout
            it->copyValueFrom(val.data());
        }
    }

    if(!retValue.containsError())
    {
        retValue += checkData();
    }

    emit parametersChanged(m_params); //send changed parameters to any connected dialogs or dock-widgets

    if (waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}






//----------------------------------------------------------------------------------------------------------------------------------
// function moved into addInGrabber.cpp -> standard for all cameras / ADDA


//----------------------------------------------------------------------------------------------------------------------------------
//! With startDevice this camera is initialized.
/*!
    In the AndorSDK3, this method does nothing. In general, the hardware camera should be intialized in this method and necessary memory should be allocated.

    \note This method is similar to VideoCapture::open() of openCV

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if starting was successfull, retWarning if startDevice has been calling at least twice.
*/
ito::RetVal AndorSDK3::startDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue = ito::retOk;

    checkData(); //this will be reallocated in this method.

    incGrabberStarted();

    if (grabberStartedCount() == 1)
    {
        retValue += checkError(AT_Flush(m_handle));
        retValue += checkError(AT_Command(m_handle, L"AcquisitionStart")); //no image is acquired yet since input buffer is empty (due to flushing before)
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
    In this AndorSDK3, this method does nothing. In general, the hardware camera should be closed in this method.

    \note This method is similar to VideoCapture::release() of openCV

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError if camera wasn't started before
    \sa startDevice
*/
ito::RetVal AndorSDK3::stopDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue = ito::retOk;

    decGrabberStarted();
    if(grabberStartedCount() == 0)
    {
        retValue += checkError(AT_Command(m_handle, L"AcquisitionStop"));
        retValue += checkError(AT_Flush(m_handle));
    }
    else if(grabberStartedCount() < 0)
    {
        retValue += ito::RetVal(ito::retError, 1001, tr("StopDevice of AndorSDK3 can not be executed, since camera has not been started.").toLatin1().data());
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
//! Call this method to trigger a new image.
/*!
    By this method a new image is trigger by the camera, that means the acquisition of the image starts in the moment, this method is called.
    The new image is then stored either in internal camera memory or in internal memory of this class.

    \note This method is similar to VideoCapture::grab() of openCV

    \param [in] trigger may describe the trigger parameter (unused here)
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError if camera has not been started or an older image lies in memory which has not be fetched by getVal, yet.
    \sa getVal
*/
ito::RetVal AndorSDK3::acquire(const int trigger, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue = ito::retOk;

    if(grabberStartedCount() <= 0)
    {
        retValue += ito::RetVal(ito::retError, 1002, tr("Acquire of AndorSDK3 can not be executed, since camera has not been started.").toLatin1().data());
    }
    else if (m_buffer.alignedBuffer == NULL)
    {
        retValue += ito::RetVal(ito::retError, 1003, tr("no valid camera memory has been allocated").toLatin1().data());
    }
    else
    {
        //Re-queue the buffers 
        m_buffer.imageAvailable = false;
        retValue += checkError(AT_QueueBuffer(m_handle, m_buffer.alignedBuffer, m_buffer.bufferSize));

        if (!retValue.containsError() && m_softwareTriggerEnabled)
        {
            retValue += checkError(AT_Command(m_handle, L"SoftwareTrigger"));
        }

        m_acquisitionRetVal = ito::retOk;

        if(waitCond)
        {
            waitCond->returnValue = retValue;
            waitCond->release();
            waitCond = NULL;
        }

        AT_U8 *ptr;
        int ptrSize;
        double timeout = m_params["timeout"].getVal<double>();
        unsigned int timeout_;
        if (timeout < AT_INFINITE)
        {
            timeout_ = static_cast<unsigned int>(timeout) * 1000.0;
        }
        else
        {
            timeout_ = AT_INFINITE;
        }

        int result = AT_WaitBuffer(m_handle, &ptr, &ptrSize, timeout_);
        if (result == AT_ERR_TIMEDOUT)
        {
            m_acquisitionRetVal = ito::RetVal(ito::retError, 0, "timeout while waiting for image.");
        }
        else
        {
            m_acquisitionRetVal = checkError(result);
        }

        if (!m_acquisitionRetVal.containsError())
        {
            m_buffer.imageAvailable = true;
        }
    }

    //only release it if not yet done
    if(waitCond)
    {
        waitCond->returnValue = retValue;
        waitCond->release();
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! Returns the grabbed camera frame as a shallow copy.
/*!
    This method copies the recently grabbed camera frame to the given DataObject-handle

    \note This method is similar to VideoCapture::retrieve() of openCV

    \param [in,out] vpdObj is the pointer to a given dataObject (this pointer should be cast to ito::DataObject*) where the acquired image is shallow-copied to.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError is camera has not been started or no image has been acquired by the method acquire.
    \sa DataObject, acquire
*/
ito::RetVal AndorSDK3::getVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::DataObject *dObj = reinterpret_cast<ito::DataObject *>(vpdObj);

    ito::RetVal retValue(ito::retOk);

    retValue += retrieveData();

    if(!retValue.containsError())
    {
        if(dObj == NULL)
        {
            retValue += ito::RetVal(ito::retError, 1004, tr("data object of getVal is NULL or cast failed").toLatin1().data());
        }
        else
        {
            retValue += sendDataToListeners(0); //don't wait for live image, since user should get the image as fast as possible.

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
ito::RetVal AndorSDK3::copyVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retValue(ito::retOk);
    ito::DataObject *dObj = reinterpret_cast<ito::DataObject *>(vpdObj);

    if(!dObj)
    {
        retValue += ito::RetVal(ito::retError, 0, tr("Empty object handle retrieved from caller").toLatin1().data());
    }
    else
    {
        retValue += checkData(dObj);  
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
ito::RetVal AndorSDK3::retrieveData(ito::DataObject *externalDataObject)
{
    ito::RetVal retValue = m_acquisitionRetVal;

    if (m_buffer.imageAvailable == false)
    {
        retValue += ito::RetVal(ito::retError, 0, "no image has been acquired");
    }

    if (!retValue.containsError())
    {
        bool hasListeners = false;
        bool copyExternal = false;
        if(m_autoGrabbingListeners.size() > 0)
        {
            hasListeners = true;
        }
        if(externalDataObject != NULL)
        {
            copyExternal = true;
        }

        int bytesPerLine = m_buffer.aoiWidth * m_buffer.aoiBitsPerPixel / 8;
        AT_U8 *startPtr = m_buffer.alignedBuffer;
        // pnPitch is in bits

        if (copyExternal)
        {
            cv::Mat *cvMat = ((cv::Mat *)externalDataObject->get_mdata()[externalDataObject->seekMat(0)]);

            for (int y = 0; y < m_buffer.aoiHeight; y++)
            {
                memcpy(cvMat->ptr(y), startPtr , bytesPerLine);
                startPtr += m_buffer.aoiStride;
            }
        }

        if (!copyExternal || hasListeners)
        {
            cv::Mat *cvMat = ((cv::Mat *)m_data.get_mdata()[m_data.seekMat(0)]);

            for (int y = 0; y < m_buffer.aoiHeight; y++)
            {
                memcpy(cvMat->ptr(y), startPtr , bytesPerLine);
                startPtr += m_buffer.aoiStride;
            }
        }

        m_buffer.imageAvailable = false;
    }

    return retValue;
}

//----------------------------------------------------------------------------------------------------------------------------------
void AndorSDK3::dockWidgetVisibilityChanged(bool visible)
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
ito::RetVal AndorSDK3::checkError(const int &code)
{
    switch (code)
    {
    case AT_SUCCESS:
        return ito::retOk;
    case AT_ERR_NOTINITIALISED:
        return ito::RetVal(ito::retError, AT_ERR_NOTINITIALISED, "Function called with an uninitialized handle");
    case AT_ERR_NOTIMPLEMENTED:
        return ito::RetVal(ito::retError, AT_ERR_NOTIMPLEMENTED, "Feature has not been implemented for the chosen camera");
    case AT_ERR_READONLY:
        return ito::RetVal(ito::retError, AT_ERR_READONLY, "Feature is read only");
    case AT_ERR_NOTREADABLE:
        return ito::RetVal(ito::retError, AT_ERR_NOTREADABLE, "Feature is currently not readable");
    case AT_ERR_NOTWRITABLE:
        return ito::RetVal(ito::retError, AT_ERR_NOTWRITABLE, "Feature is currently not writable");
    case AT_ERR_OUTOFRANGE:
        return ito::RetVal(ito::retError, AT_ERR_OUTOFRANGE, "Value is outside the maximum and minimum limits");
    case AT_ERR_INDEXNOTAVAILABLE:
        return ito::RetVal(ito::retError, AT_ERR_INDEXNOTAVAILABLE, "Index is currently not available");
    case AT_ERR_INDEXNOTIMPLEMENTED:
        return ito::RetVal(ito::retError, AT_ERR_INDEXNOTIMPLEMENTED, "Index is not implemented for the chosen camera");
    case AT_ERR_EXCEEDEDMAXSTRINGLENGTH:
        return ito::RetVal(ito::retError, AT_ERR_EXCEEDEDMAXSTRINGLENGTH, "String value provided exceeds the maximum allowed length");
    case AT_ERR_CONNECTION:
        return ito::RetVal(ito::retError, AT_ERR_CONNECTION, "Error connecting to or disconnecting from hardware");
    case AT_ERR_NODATA:
        return ito::RetVal(ito::retError, AT_ERR_NODATA, "No Internal Event or Internal Error");
    case AT_ERR_INVALIDHANDLE:
        return ito::RetVal(ito::retError, AT_ERR_INVALIDHANDLE, "Invalid device handle passed to function");
    case AT_ERR_TIMEDOUT:
        return ito::RetVal(ito::retError, AT_ERR_TIMEDOUT, "The case AT_WaitBuffer function timed out while waiting for data arrive in output queue");
    case AT_ERR_BUFFERFULL:
        return ito::RetVal(ito::retError, AT_ERR_BUFFERFULL, "The input queue has reached its capacity");
    case AT_ERR_INVALIDSIZE:
        return ito::RetVal(ito::retError, AT_ERR_INVALIDSIZE, "The size of a queued buffer did not match the frame size");
    case AT_ERR_INVALIDALIGNMENT:
        return ito::RetVal(ito::retError, AT_ERR_INVALIDALIGNMENT, "A queued buffer was not aligned on an 8-byte boundary");
    case AT_ERR_COMM:
        return ito::RetVal(ito::retError, AT_ERR_COMM, "An error has occurred while communicating with hardware");
    case AT_ERR_STRINGNOTAVAILABLE:
        return ito::RetVal(ito::retError, AT_ERR_STRINGNOTAVAILABLE, "Index / String is not available");
    case AT_ERR_STRINGNOTIMPLEMENTED:
        return ito::RetVal(ito::retError, AT_ERR_STRINGNOTIMPLEMENTED, "Index / String is not implemented for the chosen camera");
    case AT_ERR_NULL_FEATURE:
        return ito::RetVal(ito::retError, AT_ERR_NULL_FEATURE, "NULL feature name passed to function");
    case AT_ERR_NULL_HANDLE:
        return ito::RetVal(ito::retError, AT_ERR_NULL_HANDLE, "Null device handle passed to function");
    case AT_ERR_NULL_IMPLEMENTED_VAR:
        return ito::RetVal(ito::retError, AT_ERR_NULL_IMPLEMENTED_VAR, "Feature not implemented");
    case AT_ERR_NULL_READABLE_VAR:
        return ito::RetVal(ito::retError, AT_ERR_NULL_READABLE_VAR, "Readable not set");
    case AT_ERR_NULL_READONLY_VAR:
        return ito::RetVal(ito::retError, AT_ERR_NULL_READONLY_VAR, "Readonly");
    case AT_ERR_NULL_WRITABLE_VAR:
        return ito::RetVal(ito::retError, AT_ERR_NULL_WRITABLE_VAR, "Writable not set");
    case AT_ERR_NULL_MINVALUE:
        return ito::RetVal(ito::retError, AT_ERR_NULL_MINVALUE, "NULL min value");
    case AT_ERR_NULL_MAXVALUE:
        return ito::RetVal(ito::retError, AT_ERR_NULL_MAXVALUE, "NULL max value");
    case AT_ERR_NULL_VALUE:
        return ito::RetVal(ito::retError, AT_ERR_NULL_VALUE, "NULL value returned from function");
    case AT_ERR_NULL_STRING:
        return ito::RetVal(ito::retError, AT_ERR_NULL_STRING, "NULL string returned from function");
    case AT_ERR_NULL_COUNT_VAR:
        return ito::RetVal(ito::retError, AT_ERR_NULL_COUNT_VAR, "NULL feature count");
    case AT_ERR_NULL_ISAVAILABLE_VAR:
        return ito::RetVal(ito::retError, AT_ERR_NULL_ISAVAILABLE_VAR, "Available not set");
    case AT_ERR_NULL_MAXSTRINGLENGTH:
        return ito::RetVal(ito::retError, AT_ERR_NULL_MAXSTRINGLENGTH, "Max string length is NULL");
    case AT_ERR_NULL_EVCALLBACK:
        return ito::RetVal(ito::retError, AT_ERR_NULL_EVCALLBACK, "EvCallBack parameter is NULL");
    case AT_ERR_NULL_QUEUE_PTR:
        return ito::RetVal(ito::retError, AT_ERR_NULL_QUEUE_PTR, "Pointer to queue is NULL");
    case AT_ERR_NULL_WAIT_PTR:
        return ito::RetVal(ito::retError, AT_ERR_NULL_WAIT_PTR, "Wait pointer is NULL");
    case AT_ERR_NULL_PTRSIZE:
        return ito::RetVal(ito::retError, AT_ERR_NULL_PTRSIZE, "Pointer size is NULL");
    case AT_ERR_NOMEMORY:
        return ito::RetVal(ito::retError, AT_ERR_NOMEMORY, "No memory has been allocated for the current action");
    case AT_ERR_DEVICEINUSE:
        return ito::RetVal(ito::retError, AT_ERR_DEVICEINUSE, "Function failed to connect to a device because it is already being used");
    case AT_ERR_HARDWARE_OVERFLOW:
        return ito::RetVal(ito::retError, AT_ERR_HARDWARE_OVERFLOW, "The software was not able to retrieve data from the card or camera fast enough to avoid the internal hardware buffer bursting.");
    }

    return ito::RetVal(ito::retError, 0, "unknown error");
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal AndorSDK3::synchronizeCameraSettings(int what /*= sAll*/)
{
    ito::RetVal retval, rettemp;
    double dmin, dmax, dval;
    QMap<QString, ito::Param>::iterator it;
    AT_BOOL implemented;

    /*if (what & sPixelClock)
    {
        //get pixelclock and ranges
        it = m_params.find("pixel_clock");
        rettemp = checkError(is_PixelClock(m_camera, IS_PIXELCLOCK_CMD_GET_RANGE, (void*)uintVal3, sizeof(uintVal3)));
        rettemp += checkError(is_PixelClock(m_camera, IS_PIXELCLOCK_CMD_GET, (void*)&uintVal, sizeof(uintVal)));
        if (!rettemp.containsError())
        {
            it->setVal<int>(uintVal);
            it->setMeta(new ito::IntMeta(uintVal3[0], uintVal3[1], uintVal3[2]),true);
        }
        else
        {
            it->setFlags(ito::ParamBase::Readonly);
        }
        retval += rettemp;
    }*/

    if (what & sExposure)
    {
        //get exposure time, exposure modes and ranges
        it = m_params.find("integration_time");
        rettemp = checkError(AT_IsImplemented(m_handle, L"ExposureTime", &implemented));
        if (!rettemp.containsError() && implemented)
        {
            it->setFlags(0);
            rettemp += checkError(AT_GetFloatMin(m_handle, L"ExposureTime", &dmin));
            rettemp += checkError(AT_GetFloatMax(m_handle, L"ExposureTime", &dmax));
            rettemp += checkError(AT_GetFloat(m_handle, L"ExposureTime", &dval));
            it->setMeta(new ito::DoubleMeta(dmin,dmax),true);
            it->setVal<double>(dval);
        }
        else
        {
            it->setFlags(ito::ParamBase::Readonly);
        }
        retval += rettemp;
    }

    if (what & sBinning)
    {
        //get binning
        it = m_params.find("binning");
        AT_64 binHMin, binHMax, binH;
        AT_64 binVMin, binVMax, binV;
        AT_BOOL available;
        AT_IsImplemented(m_handle, L"AOIHBin", &available);
        rettemp = checkError(AT_GetInt(m_handle, L"AOIHBin", &binH));
        rettemp += checkError(AT_GetIntMin(m_handle, L"AOIHBin", &binHMin));
        rettemp += checkError(AT_GetIntMax(m_handle, L"AOIHBin", &binHMax));
        rettemp += checkError(AT_GetIntMin(m_handle, L"AOIVBin", &binVMin));
        rettemp += checkError(AT_GetIntMax(m_handle, L"AOIVBin", &binVMax));
        rettemp += checkError(AT_GetInt(m_handle, L"AOIVBin", &binV));

        if (available == false || rettemp.containsError()) //no binning abilities
        {
            it->setFlags(ito::ParamBase::Readonly);
            m_hBin = 1;
            m_vBin = 1;
        }
        else
        {
            it->setFlags(0);
            m_hBin = binH;
            m_vBin = binV;
            it->setMeta(new ito::IntMeta(binHMin*100+binVMin, binHMax*100+binHMax), true);
        }

        it->setVal<int>(m_hBin*100+m_vBin);
        retval += rettemp;
    }

    if (what & sRoi)
    {
        //get current roi and adjust min/max values of roi
        AT_Size offset, offsetMin, offsetMax;
        AT_Size size, sizeMin, sizeMax;

        rettemp = checkError(AT_GetInt(m_handle, L"AOIWidth", &(size.x)));
        rettemp += checkError(AT_GetInt(m_handle, L"AOIHeight", &(size.y)));
        rettemp += checkError(AT_GetIntMin(m_handle, L"AOIWidth", &(sizeMin.x)));
        rettemp += checkError(AT_GetIntMin(m_handle, L"AOIHeight", &(sizeMin.y)));
        rettemp += checkError(AT_GetIntMax(m_handle, L"AOIWidth", &(sizeMax.x)));
        rettemp += checkError(AT_GetIntMax(m_handle, L"AOIHeight", &(sizeMax.y)));

        if (!rettemp.containsError())
        {
            it = m_params.find("sizex");
            it->setVal<int>(size.x);
            it->setMeta(new ito::IntMeta(sizeMin.x, sizeMax.x), true);
            
            it = m_params.find("sizey");
            it->setVal<int>(size.y);
            it->setMeta(new ito::IntMeta(sizeMin.y, sizeMax.y), true);
        }

        rettemp += checkError(AT_GetIntMin(m_handle, L"AOILeft", &(offsetMin.x)));
        rettemp += checkError(AT_GetIntMax(m_handle, L"AOILeft", &(offsetMax.x)));
        rettemp += checkError(AT_GetInt(m_handle, L"AOILeft", &(offset.x)));

        rettemp += checkError(AT_GetIntMin(m_handle, L"AOITop", &(offsetMin.y)));
        rettemp += checkError(AT_GetIntMax(m_handle, L"AOITop", &(offsetMax.y)));
        rettemp += checkError(AT_GetInt(m_handle, L"AOITop", &(offset.y)));

        //offset values are in pixel coordinates
        offsetMin.x /= m_hBin;
        offsetMin.y /= m_vBin;
        offsetMax.x /= m_hBin;
        offsetMax.y /= m_vBin;
        offset.x /= m_hBin;
        offset.y /= m_vBin;

        //size values are in super-pixels (considering binning)

        if (!rettemp.containsError())
        {
            int currentX1 = size.x + offset.x - 1;
            int currentY1 = size.y + offset.y - 1;

            it = m_params.find("x0");
            it->setVal<int>(offset.x);
            //x0 max is one smaller than the current x1 value, considering the increment value
            int x0max = currentX1 - (currentX1 - offsetMin.x) % 1; //offsetInc.x;
            it->setMeta(new ito::IntMeta(offsetMin.x, x0max, 1), true);
            it->setFlags(0);

            it = m_params.find("y0");
            it->setVal<int>(offset.y);
            //x0 max is one smaller than the current x1 value, considering the increment value
            int y0max = currentY1 - (currentY1 - offsetMin.y) % 1; //offsetInc.y;
            it->setMeta(new ito::IntMeta(offsetMin.y, y0max, 1), true);
            it->setFlags(0);

            it = m_params.find("x1");
            it->setVal<int>(currentX1);
            it->setMeta(new ito::IntMeta(offset.x + sizeMin.x - 1, offset.x + sizeMax.x - 1), true);
            it->setFlags(0);

            it = m_params.find("y1");
            it->setVal<int>(currentY1);
            it->setMeta(new ito::IntMeta(offset.y + sizeMin.y - 1, offset.y + sizeMax.y - 1), true);
            it->setFlags(0);
        }
        else
        {
            m_params["x0"].setVal<int>(0);
            m_params["y0"].setVal<int>(0);
            m_params["x1"].setVal<int>(size.x-1);
            m_params["y1"].setVal<int>(size.y-1);
            m_params["x0"].setFlags(ito::ParamBase::Readonly);
            m_params["x1"].setFlags(ito::ParamBase::Readonly);
            m_params["y0"].setFlags(ito::ParamBase::Readonly);
            m_params["y1"].setFlags(ito::ParamBase::Readonly);
        }

        AT_BOOL available;
        AT_GetBool(m_handle, L"FullAOIControl", &available);
        m_params["full_aoi_control"].setVal<int>( available ? 1 : 0 );

        retval += rettemp;
    }

    /*if (what & sGain)
    {
        //get pixelclock and ranges
        it = m_params.find("gain");
        it->setVal<double>((double)is_SetHardwareGain(m_camera, IS_GET_MASTER_GAIN, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) / 100.0);
        if (m_sensorInfo.nColorMode & IS_COLORMODE_MONOCHROME)
        {
            it->setFlags(0);
        }
        else
        {
            it->setFlags(ito::ParamBase::Readonly);
        }

        it = m_params.find("gain_rgb");
        double rgbGain[] = {0.0,0.0,0.0};
        rgbGain[0] = is_SetHardwareGain(m_camera, IS_GET_RED_GAIN, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) / 100.0;
        rgbGain[1] = is_SetHardwareGain(m_camera, IS_GET_GREEN_GAIN, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) / 100.0;
        rgbGain[2] = is_SetHardwareGain(m_camera, IS_GET_BLUE_GAIN, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) / 100.0;
        it->setVal<double*>(rgbGain, 3);
        if (m_sensorInfo.nColorMode & IS_COLORMODE_BAYER)
        {
            it->setFlags(0);
        }
        else
        {
            it->setFlags(ito::ParamBase::Readonly);
        }

        it = m_params.find("gain_boost_enabled");
        if (is_SetGainBoost(m_camera, IS_GET_SUPPORTED_GAINBOOST) == IS_SET_GAINBOOST_ON)
        {
            it->setFlags(0);
            int activated = is_SetGainBoost(m_camera, IS_GET_GAINBOOST);
            it->setVal<int>( activated == IS_SET_GAINBOOST_ON ? 1 : 0 );
        }
        else
        {
            it->setFlags(ito::ParamBase::Readonly);
            it->setVal<int>(0);
        }
    }

    if (what & sOffset)
    {
        //offset is represented by the manual blacklevel correction that is added to each value. If this feature cannot be changed the
        //parameter is set to readonly
        it = m_params.find("offset");
        UINT capabilities, val2;
        ito::RetVal retTemp = checkError(is_Blacklevel(m_camera, IS_BLACKLEVEL_CMD_GET_CAPS, (void*)&capabilities, sizeof(capabilities)));

        if (!retTemp.containsError())
        {
            it->setFlags((capabilities & IS_BLACKLEVEL_CAP_SET_OFFSET) ? 0 : ito::ParamBase::Readonly);
            
            retTemp += checkError(is_Blacklevel(m_camera, IS_BLACKLEVEL_CMD_GET_OFFSET_RANGE, (void*)&m_blacklevelRange, sizeof(m_blacklevelRange)));
            retTemp += checkError(is_Blacklevel(m_camera, IS_BLACKLEVEL_CMD_GET_OFFSET, (void*)&val2, sizeof(val2)));

            if (!retTemp.containsError())
            {
                it->setVal<double>((double)(val2 - m_blacklevelRange.s32Min) / ((double)(m_blacklevelRange.s32Max -m_blacklevelRange.s32Min)));
                it->setMeta(new ito::DoubleMeta(0.0,1.0,0.0), true);
            }
            else
            {
                it->setVal<int>(0.0);
                it->setMeta(new ito::DoubleMeta(0.0,1.0,0.0), true);
            }

            it = m_params.find("auto_blacklevel_enabled");
            if (capabilities & IS_BLACKLEVEL_CAP_SET_AUTO_BLACKLEVEL)
            {
                it->setMeta(new ito::IntMeta(0,1), true);
                it->setFlags(0);
                retTemp += checkError(is_Blacklevel(m_camera, IS_BLACKLEVEL_CMD_GET_MODE, (void*)&val2, sizeof(val2)));
                if (!retTemp.containsError())
                {
                    it->setVal<int>( val2 == IS_AUTO_BLACKLEVEL_ON ? 1 : 0 );
                }
            }
            else
            {
                it->setFlags(ito::ParamBase::Readonly);
                retTemp += checkError(is_Blacklevel(m_camera, IS_BLACKLEVEL_CMD_GET_MODE, (void*)&val2, sizeof(val2)));
                if (!retTemp.containsError())
                {
                    int v = val2 == IS_AUTO_BLACKLEVEL_ON ? 1 : 0;
                    it->setVal<int>( v );
                    it->setMeta(new ito::IntMeta(v,v), true);                    
                }
            }
        }
    }*/

    if (what & sTriggerMode)
    {
        it = m_params.find("trigger_mode");
        ito::StringMeta *sm = new ito::StringMeta(ito::StringMeta::String);

        int count;
        AT_WC wstring[100];
        QString value;
        int iVal;

        AT_GetEnumIndex(m_handle, L"TriggerMode", &iVal);
        if (AT_GetEnumCount(m_handle, L"TriggerMode", &count) == AT_SUCCESS)
        {
            for (int c = 0; c < count; ++c)
            {
                AT_GetEnumStringByIndex(m_handle, L"TriggerMode", c, wstring, 100);
                value = QString::fromWCharArray(wstring);
                sm->addItem(value.toLatin1().data());

                if (c == iVal)
                {
                    it->setVal<char*>(value.toLatin1().data());

                    m_softwareTriggerEnabled = (c == m_triggerModeIdx.tSoftware);
                }
            }
            it->setFlags(0);
        }
        else
        {
            it->setVal<char*>("[notSupported]");
            it->setFlags(ito::ParamBase::Readonly);
            
        }

        it->setMeta(sm,true);   
    }

    if (what & sElectronicShutteringMode)
    {
        it = m_params.find("electronic_shuttering_mode");
        
        int count;
        AT_WC wstring[100];
        QString value;
        int iVal;

        AT_GetEnumIndex(m_handle, L"ElectronicShutteringMode", &iVal);
        if (AT_GetEnumCount(m_handle, L"ElectronicShutteringMode", &count) == AT_SUCCESS)
        {
            for (int c = 0; c < count; ++c)
            {
                AT_GetEnumStringByIndex(m_handle, L"ElectronicShutteringMode", c, wstring, 100);
                if (wcscmp(wstring, L"Rolling") == 0)
                {
                    it->setVal<int>(0);
                }
                else if (wcscmp(wstring, L"Global") == 0)
                {
                    it->setVal<int>(1);
                }
            }
            it->setFlags(0);
        }
        else
        {
            it->setVal<int>(0);
            it->setFlags(ito::ParamBase::Readonly);
        }  
    }

    if (what & sBppAndPreAmpGain)
    {
        it = m_params.find("bpp");
        int minBpp = 16;
        int maxBpp = 8;
        int stepBpp = 8;
        AT_BOOL available;
        int current;
        AT_GetEnumIndex(m_handle, L"PixelEncoding", &current);

        if (m_pixelEncodingIdx.mono8 >= 0)
        {
            AT_IsEnumIndexAvailable(m_handle, L"PixelEncoding", m_pixelEncodingIdx.mono8, &available);
            if (available)
            {
                minBpp = 8;
                maxBpp = 8;
                stepBpp = 8;
            }
        }

        if (m_pixelEncodingIdx.mono12 >= 0)
        {
            AT_IsEnumIndexAvailable(m_handle, L"PixelEncoding", m_pixelEncodingIdx.mono12, &available);
            if (available)
            {
                minBpp = std::min(minBpp,12);
                maxBpp = std::max(maxBpp,12);
                stepBpp = 4;
            }
        }

        if (m_pixelEncodingIdx.mono16 >= 0)
        {
            AT_IsEnumIndexAvailable(m_handle, L"PixelEncoding", m_pixelEncodingIdx.mono16, &available);
            if (available)
            {
                minBpp = std::min(minBpp,16);
                maxBpp = std::max(maxBpp,16);
            }
        }

        if (current == m_pixelEncodingIdx.mono8)
        {
            it->setVal<int>(8);
        }
        else if (current == m_pixelEncodingIdx.mono12)
        {
            it->setVal<int>(12);
        }
        else if (current == m_pixelEncodingIdx.mono16)
        {
            it->setVal<int>(16);
        }
        
        it->setMeta(new ito::IntMeta(minBpp,maxBpp,stepBpp), true);

        retval += rettemp;

    }

    return retval;
}


//-------------------------------------------------------------------------------------------------
ito::RetVal AndorSDK3::checkData(ito::DataObject *externalDataObject)
{
    int futureHeight = m_params["sizey"].getVal<int>();
    int futureWidth = m_params["sizex"].getVal<int>();
    int futureType;

    int bpp = m_params["bpp"].getVal<int>();
    if (bpp <= 8)
    {
        futureType = ito::tUInt8;
    }
    else if (bpp <= 16)
    {
        futureType = ito::tUInt16;
    }
    else if (bpp <= 32)
    {
        futureType = ito::tInt32;
    }
    else 
    {
        futureType = ito::tFloat64;
    }

    if (externalDataObject == NULL)
    {
        if (m_data.getDims() < 2 || m_data.getSize(0) != (unsigned int)futureHeight || m_data.getSize(1) != (unsigned int)futureWidth || m_data.getType() != futureType)
        {
            m_data = ito::DataObject(futureHeight,futureWidth,futureType);
        }
    }
    else
    {
        int dims = externalDataObject->getDims();
        if (externalDataObject->getDims() == 0)
        {
            *externalDataObject = ito::DataObject(futureHeight,futureWidth,futureType);
        }
        else if (externalDataObject->calcNumMats () > 1)
        {
            return ito::RetVal(ito::retError, 0, tr("Error during check data, external dataObject invalid. Object has more than 1 plane. It must be of right size and type or a uninitilized image.").toLatin1().data());            
        }
        else if (externalDataObject->getSize(dims - 2) != (unsigned int)futureHeight || externalDataObject->getSize(dims - 1) != (unsigned int)futureWidth || externalDataObject->getType() != futureType)
        {
            return ito::RetVal(ito::retError, 0, tr("Error during check data, external dataObject invalid. Object must be of right size and type or a uninitilized image.").toLatin1().data());
        }
    }

    AT_64 bufferSize;
    AT_GetInt(m_handle, L"ImageSizeBytes", &bufferSize);

    if (m_buffer.bufferSize != bufferSize || m_buffer.aoiHeight != futureHeight)
    {
        AT_Flush(m_handle);

        //delete old buffer
        if (m_buffer.buffer)
        {
            delete[] m_buffer.buffer;
            m_buffer.buffer = NULL;
        }

        m_buffer.alignedBuffer = NULL;

        m_buffer.bufferSize = bufferSize;
        AT_GetInt(m_handle, L"AOIStride", &(m_buffer.aoiStride));
        AT_GetInt(m_handle, L"AOIWidth", &(m_buffer.aoiWidth));
        AT_GetInt(m_handle, L"AOIHeight", &(m_buffer.aoiHeight));
        m_buffer.aoiBitsPerPixel = bpp;
        m_buffer.imageAvailable = false;

        //Allocate a number of memory buffers to store frames 
        m_buffer.buffer = new unsigned char[bufferSize + 7]; 
        m_buffer.alignedBuffer = reinterpret_cast<unsigned char*>((reinterpret_cast<unsigned long>(m_buffer.buffer) + 7) & ~7); 
    }

    return ito::retOk;
}


//----------------------------------------------------------------------------------------
ito::RetVal AndorSDK3::loadEnumIndices()
{
    ito::RetVal retval;
    int count;
    AT_WC wstring[100];

    //PixelEncoding
    if (AT_GetEnumCount(m_handle, L"PixelEncoding", &count) == AT_SUCCESS)
    {
        for (int c = 0; c < count; ++c)
        {
            AT_GetEnumStringByIndex(m_handle, L"PixelEncoding", c, wstring, 100);

            if (wcscmp(wstring, L"Mono8") == 0)
            {
                m_pixelEncodingIdx.mono8 = c;
            }
            else if (wcscmp(wstring, L"Mono12") == 0)
            {
                m_pixelEncodingIdx.mono12 = c;
            }
            else if (wcscmp(wstring, L"Mono16") == 0)
            {
                m_pixelEncodingIdx.mono16 = c;
            }
        }
    }

    //TriggerMode
    if (AT_GetEnumCount(m_handle, L"TriggerMode", &count) == AT_SUCCESS)
    {
        for (int c = 0; c < count; ++c)
        {
            AT_GetEnumStringByIndex(m_handle, L"TriggerMode", c, wstring, 100);

            if (wcscmp(wstring, L"Internal") == 0)
            {
                m_triggerModeIdx.tInternal = c;
            }
            else if (wcscmp(wstring, L"Software") == 0)
            {
                m_triggerModeIdx.tSoftware = c;
            }
            else if (wcscmp(wstring, L"External") == 0)
            {
                m_triggerModeIdx.tExternal = c;
            }
            else if (wcscmp(wstring, L"External Start") == 0)
            {
                m_triggerModeIdx.tExternalStart = c;
            }
            else if (wcscmp(wstring, L"External Exposure") == 0)
            {
                m_triggerModeIdx.tExternalExposure = c;
            }
        }
    }
    
    //ElectronicShutteringMode
    if (AT_GetEnumCount(m_handle, L"ElectronicShutteringMode", &count) == AT_SUCCESS)
    {
        for (int c = 0; c < count; ++c)
        {
            AT_GetEnumStringByIndex(m_handle, L"ElectronicShutteringMode", c, wstring, 100);

            if (wcscmp(wstring, L"Global") == 0)
            {
                m_electronicShutteringMode.tShutterGlobal = c;
            }
            else if (wcscmp(wstring, L"Rolling") == 0)
            {
                m_electronicShutteringMode.tShutterRolling = c;
            }
        }
    }

    return retval;
}

//----------------------------------------------------------------------------------------
ito::RetVal AndorSDK3::loadSensorInfo()
{
    ito::RetVal retval;

    AT_WC szValue[256];
    QString value;
    AT_BOOL implemented;

    //firmware version
    if (AT_IsImplemented(m_handle, L"FirmwareVersion", &implemented) == AT_SUCCESS && implemented)
    {
        if (AT_GetString(m_handle, L"FirmwareVersion", szValue, 256) == AT_SUCCESS)
        {
            value = QString::fromWCharArray(szValue);
            m_params["firmware_version"].setVal<char*>(value.toLatin1().data());
        }
    }

    //interface type
    if (AT_IsImplemented(m_handle, L"InterfaceType", &implemented) == AT_SUCCESS && implemented)
    {
        if (AT_GetString(m_handle, L"InterfaceType", szValue, 256) == AT_SUCCESS)
        {
            value = QString::fromWCharArray(szValue);
            m_params["interface"].setVal<char*>(value.toLatin1().data());
        }
    }

    //camera_model
    if (AT_IsImplemented(m_handle, L"CameraModel", &implemented) == AT_SUCCESS && implemented)
    {
        if (AT_GetString(m_handle, L"CameraModel", szValue, 256) == AT_SUCCESS)
        {
            value = QString::fromWCharArray(szValue);
            m_params["camera_model"].setVal<char*>(value.toLatin1().data());
        }
    }

    //camera_name
    if (AT_IsImplemented(m_handle, L"CameraName", &implemented) == AT_SUCCESS && implemented)
    {
        if (AT_GetString(m_handle, L"CameraName", szValue, 256) == AT_SUCCESS)
        {
            value = QString::fromWCharArray(szValue);
            m_params["camera_name"].setVal<char*>(value.toLatin1().data());
        }
    }

    //serial_number
    if (AT_IsImplemented(m_handle, L"SerialNumber", &implemented) == AT_SUCCESS && implemented)
    {
        if (AT_GetString(m_handle, L"SerialNumber", szValue, 256) == AT_SUCCESS)
        {
            value = QString::fromWCharArray(szValue);
            m_params["serial_number"].setVal<char*>(value.toLatin1().data());
        }
    }

    //sdk_version
    if (AT_IsImplemented(m_handle, L"SoftwareVersion", &implemented) == AT_SUCCESS && implemented)
    {
        if (AT_GetString(m_handle, L"SoftwareVersion", szValue, 256) == AT_SUCCESS)
        {
            value = QString::fromWCharArray(szValue);
            m_params["sdk_version"].setVal<char*>(value.toLatin1().data());
        }
    }

    return retval;
}

//----------------------------------------------------------------------------------------
const ito::RetVal AndorSDK3::showConfDialog(void)
{
    return apiShowConfigurationDialog(this, new DialogSDK3(this));
}