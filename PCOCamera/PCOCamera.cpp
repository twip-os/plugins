/* ********************************************************************
    Plugin "PCOCamera" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2013, Institut f�r Technische Optik (ITO),
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

#define ITOM_IMPORT_API
#define ITOM_IMPORT_PLOTAPI

#include "PCOCamera.h"
#include "pluginVersion.h"
#define _USE_MATH_DEFINES  // needs to be defined to enable standard declartions of PI constant
#include "math.h"

#include <qstring.h>
#include <qstringlist.h>
#include <QtCore/QtPlugin>
#include <qmetaobject.h>

#include <qdockwidget.h>
#include <qpushbutton.h>
#include <qmetaobject.h>
#include "dockWidgetPCOCamera.h"

#include "common/helperCommon.h"

#include "PCO_errt.h"

#include <QElapsedTimer>

//#pragma comment(linker, "/delayload:SC2_Cam.dll")

//#include <qdebug.h>
//#include <qmessagebox.h>


Q_DECLARE_METATYPE(ito::DataObject)

static QLibrary mySC2Lib;

//----------------------------------------------------------------------------------------------------------------------------------

/*!
    \class PCOCameraInterface
    \brief Small interface class for class PCOCamera. This class contains basic information about PCOCamera as is able to
        create one or more new instances of PCOCamera.
*/

//----------------------------------------------------------------------------------------------------------------------------------
//! creates new instance of PCOCamera and returns the instance-pointer.
/*!
    \param [in,out] addInInst is a double pointer of type ito::AddInBase. The newly created PCOCamera-instance is stored in *addInInst
    \return retOk
    \sa PCOCamera
*/
ito::RetVal PCOCameraInterface::getAddInInst(ito::AddInBase **addInInst)
{
    NEW_PLUGININSTANCE(PCOCamera)
    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! deletes instance of PCOCamera. This instance is given by parameter addInInst.
/*!
    \param [in] double pointer to the instance which should be deleted.
    \return retOk
    \sa PCOCamera
*/
ito::RetVal PCOCameraInterface::closeThisInst(ito::AddInBase **addInInst)
{
    REMOVE_PLUGININSTANCE(PCOCamera)
    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! constructor for interface
/*!
    defines the plugin type (dataIO and grabber) and sets the plugins object name. If the real plugin (here: PCOCamera) should or must
    be initialized (e.g. by a Python call) with mandatory or optional parameters, please initialize both vectors m_initParamsMand
    and m_initParamsOpt within this constructor.
*/
PCOCameraInterface::PCOCameraInterface()
{
    m_autoLoadPolicy = ito::autoLoadKeywordDefined;
    m_autoSavePolicy = ito::autoSaveAlways;

    m_type = ito::typeDataIO | ito::typeGrabber;
    setObjectName("PCOCamera");

    m_description = QObject::tr("DLL for PCO-Cameras");
    
    char docstring[] = \
"The PCOCamera is a Plugin to access PCO.XXXX, e.g. PCO.1300 or PCO.2000, with ito itom. \n\
This plugin has for instance be tested with the camera PCO.1300. \n\
\n\
For compiling this plugin, set the CMake variable **PCO_SDK_DIR** to the base directory of the pco.sdk. \n\
The SDK from PCO can be downloaded from http://www.pco.de (pco Software-Development-Toolkit (SDK)). \n\
Download the SDK and install it at any location. Additionally you need to install the drivers for operating your framegrabber board. \n\
\n\
For GigE cameras, make sure that the PCO GigE driver is installed and that the camera connection is properly configured.";

    m_detaildescription = QObject::tr(docstring);
    
    m_author = "W. Lyda, C. Lingel, M. Gronle, ITO, University Stuttgart";
    m_version = (PLUGIN_VERSION_MAJOR << 16) + (PLUGIN_VERSION_MINOR << 8) + PLUGIN_VERSION_PATCH;
    m_minItomVer = MINVERSION;
    m_maxItomVer = MAXVERSION;
    m_license = QObject::tr("LGPL / copyright of the external DLLs belongs to PCO");
    m_aboutThis = QObject::tr("N.A.");      
    
    m_initParamsMand.clear();
    m_initParamsOpt.clear();
    
    return;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! destructor
/*!
    clears both vectors m_initParamsMand and m_initParamsOpt.
*/
PCOCameraInterface::~PCOCameraInterface()
{
    m_initParamsMand.clear();
    m_initParamsOpt.clear();
}

//----------------------------------------------------------------------------------------------------------------------------------
// this makro registers the class PCOCameraInterface with the name PCOCamerainterface as plugin for the Qt-System (see Qt-DOC)
Q_EXPORT_PLUGIN2(PCOCamerainterface, PCOCameraInterface)

//----------------------------------------------------------------------------------------------------------------------------------

/*!
    \class PCOCamera
    \brief Class for the PCOCamera. The PCOCamera is able to create noisy images or simulate a typical WLI or confocal image signal.

    Usually every method in this class can be executed in an own thread. Only the constructor, destructor, showConfDialog will be executed by the 
    main (GUI) thread.
*/

//----------------------------------------------------------------------------------------------------------------------------------
//! shows the configuration dialog. This method must be executed in the main (GUI) thread and is usually called by the addIn-Manager.
/*!
    creates new instance of dialogPCOCamera, calls the method setVals of dialogPCOCamera, starts the execution loop and if the dialog
    is closed, reads the new parameter set and deletes the dialog.

    \return retOk
    \sa dialogPCOCamera
*/
const ito::RetVal PCOCamera::showConfDialog(void)
{
    dialogPCOCamera *confDialog = new dialogPCOCamera(this);

    connect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), confDialog, SLOT(valuesChanged(QMap<QString, ito::Param>)));  
    QMetaObject::invokeMethod(this, "sendParameterRequest");

    if (confDialog->exec())
    {
        disconnect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), confDialog, SLOT(valuesChanged(QMap<QString, ito::Param>))); 
        confDialog->sendVals();
    }
    else
    {
        disconnect(this, SIGNAL(parametersChanged(QMap<QString, ito::Param>)), confDialog, SLOT(valuesChanged(QMap<QString, ito::Param>)));     
    }
    delete confDialog;

    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
//! constructor for PCOCamera
/*!
    In this constructor the m_params-vector with all parameters, which are accessible by getParam or setParam, is built.
    Additionally the optional docking widget for the PCOCamera's toolbar is instantiated and created by createDockWidget.

    \param [in] uniqueID is an unique identifier for this PCOCamera-instance
    \sa ito::tParam, createDockWidget, setParam, getParam
*/
PCOCamera::PCOCamera() : 
    AddInGrabber(),
    m_isgrabbing(false),
    m_hCamera(NULL),
    m_hEvent(NULL)
{
    //qRegisterMetaType<QMap<QString, ito::Param> >("QMap<QString, ito::Param>");
    //qRegisterMetaType<ito::DataObject>("ito::DataObject");

    ito::Param paramVal("name", ito::ParamBase::String, "PCOCamera", "GrabberName");
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("integration_time", ito::ParamBase::Double, 0.000001, 65.0, 0.01, tr("Integrationtime of CCD programmed in s").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    // is not used anywhere...
    //paramVal = ito::Param("frame_time", ito::ParamBase::Double | ito::ParamBase::Readonly, 0.05, 10.0, 0.1, tr("Time between two frames").toAscii().data());
    //m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("gain", ito::ParamBase::Double | ito::ParamBase::Readonly, 0.0, 1.0, 1.0, tr("Virtual gain").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("offset", ito::ParamBase::Double | ito::ParamBase::Readonly, 0.0, 1.0, 0.5, tr("Virtual offset").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("binning", ito::ParamBase::Int, 101, 404, 101, tr("Binning of different pixel").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("sizex", ito::ParamBase::Int | ito::ParamBase::Readonly, 1, 2048, 2048, tr("Pixelsize in x (cols)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("sizey", ito::ParamBase::Int | ito::ParamBase::Readonly, 1, 2048, 2048, tr("Pixelsize in y (rows)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("x0", ito::ParamBase::Int, 0, 2047, 0, tr("First column within the region of interest (zero-based, x0 < x1)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("y0", ito::ParamBase::Int, 0, 2047, 0, tr("First row within the region of interest (zero-based, y0 < y1)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("x1", ito::ParamBase::Int, 0, 2047, 2047, tr("Last column within the region of interest (zero-based, x0 < x1)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);    
    paramVal = ito::Param("y1", ito::ParamBase::Int, 0, 2047, 2047, tr("Last row within the region of interest (zero-based, y0 < y1)").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("bpp", ito::ParamBase::Int, 16, 16, 16, tr("bits per pixel").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    //paramVal = ito::Param("time_out", ito::ParamBase::Double , 0.1, 60.0, 2.0, tr("Timeout for acquiring images").toAscii().data());
    //m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("temperatures", ito::ParamBase::DoubleArray | ito::ParamBase::Readonly, NULL, tr("CCD, camera and power supply temperatures in degree celcius").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("coolingSetPointTemperature", ito::ParamBase::Int, 0, 1000, 0, tr("Desired set point temperature for cooling").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("IRSensitivity", ito::ParamBase::Int, 0, 1, 1, tr("Switch the IR Sensitivity of the image sensor, Parameter is not available for all cameras").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("Pixelrate", ito::ParamBase::Int, 10, 20, 10, tr("Pixelrate of the image sensor in MHz").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);
    paramVal = ito::Param("ConversionFactor", ito::ParamBase::Double, 3.5, 3.8, 0.3, tr("Conversion factor in electrons/count").toAscii().data());
    m_params.insert(paramVal.getName(), paramVal);

    //now create dock widget for this plugin
    DockWidgetPCOCamera *dw = new DockWidgetPCOCamera(this);

    Qt::DockWidgetAreas areas = Qt::AllDockWidgetAreas;
    QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
    createDockWidget(QString(m_params["name"].getVal<char *>()), features, areas, dw);

    checkData();
}

//----------------------------------------------------------------------------------------------------------------------------------
//! destructor
/*!
    \sa ~AddInBase
*/
PCOCamera::~PCOCamera()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
//! adds the PCO error to ito::RetVal and translates the hex error to an error text.
ito::RetVal PCOCamera::checkError(int error)
{
    ito::RetVal retVal;

    if (error != PCO_NOERROR)
    {
        char buffer[512];
        buffer[511] = '\0';
        PCO_GetErrorText(error, buffer, 512);
        if (error & PCO_ERROR_IS_WARNING)
        {
            retVal += ito::RetVal(ito::retWarning,0,buffer);
        }
        else
        {
            retVal += ito::RetVal(ito::retError,0,buffer);
        }
    }
    return retVal;
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
ito::RetVal PCOCamera::getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retVal;
    QString key;
    bool hasIndex = false;
    int index;
    QString suffix;
    QMap<QString,ito::Param>::iterator it;

    //parse the given parameter-name (if you support indexed or suffix-based parameters)
    retVal += apiParseParamName(val->getName(), key, hasIndex, index, suffix);

    if(retVal == ito::retOk)
    {
        //gets the parameter key from m_params map (read-only is allowed, since we only want to get the value).
        retVal += apiGetParamFromMapByKey(m_params, key, it, false);
    }

    if(!retVal.containsError())
    {
        if (key == "temperatures")
        {
            short ccdtemp, camtemp, powtemp;
            retVal += checkError(PCO_GetTemperature(m_hCamera, &ccdtemp, &camtemp, &powtemp));
            if (!retVal.containsError())
            {
                double temps[] = {(double)ccdtemp/10.0, (double)camtemp, (double)powtemp};
                it->setVal<double*>(temps,3);                
            }            
        }
        //finally, save the desired value in the argument val (this is a shared pointer!)
        *val = it.value();
    }

    if (waitCond)
    {
        waitCond->returnValue = retVal;
        waitCond->release();
    }

    return retVal;
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
ito::RetVal PCOCamera::setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retVal(ito::retOk);
    QString key;
    bool hasIndex;
    int index;
    QString suffix;
    QMap<QString, ito::Param>::iterator it;

    int ret = 0, sizeX = 0, sizeY = 0;
    int running = 0; // Used to check if grabber was running bevor

    //parse the given parameter-name (if you support indexed or suffix-based parameters)
    retVal += apiParseParamName( val->getName(), key, hasIndex, index, suffix );

    if(!retVal.containsError())
    {
        //gets the parameter key from m_params map (read-only is not allowed and leads to ito::retError).
        retVal += apiGetParamFromMapByKey(m_params, key, it, true);
    }

    if(!retVal.containsError())
    {
        //here the new parameter is checked whether its type corresponds or can be cast into the
        // value in m_params and whether the new type fits to the requirements of any possible
        // meta structure.
        retVal += apiValidateParam(*it, *val, false, true);
    }

    if(!retVal.containsError())
    {
        m_caminfo.wSize =sizeof(PCO_Description);
        retVal += checkError(PCO_GetCameraDescription(m_hCamera, &m_caminfo));
    }
    

    char errbuffer[400] = {0};

    if (!retVal.containsError())
    {
        if (grabberStartedCount())
        {
            running = grabberStartedCount();
            setGrabberStarted(1);
            retVal += this->stopDevice(0);
        }
        else
        {
            setGrabberStarted(1);
            this->stopDevice(0);
        }

        if (key == "binning")
        {
            WORD newbinX = val->getVal<int>()/100;
            WORD newbinY = val->getVal<int>()-newbinX *100;

            WORD maxbinX = (WORD)it->getMax()/100;
            WORD maxbinY = (WORD)it->getMax()- maxbinX * 100;

            WORD minbinX = (WORD)it->getMin()/100;
            WORD minbinY = (WORD)it->getMin()- minbinX * 100;

            if( newbinX > maxbinX)
            {
                retVal += ito::RetVal(ito::retError, 0, tr("New value in X is larger than maximal value, input ignored").toAscii().data());
            }
            else if( newbinY > maxbinY)
            {
                retVal += ito::RetVal(ito::retError, 0, tr("New value in Y is larger than maximal value, input ignored").toAscii().data());
            }
            else if(newbinX < minbinX)
            {
                retVal += ito::RetVal(ito::retError, 0, tr("New value in X is smaller than parameter range, input ignored").toAscii().data());
            }
            else if(newbinY < minbinY)
            {
                retVal += ito::RetVal(ito::retError, 0, tr("New value in Y is smaller than parameter range, input ignored").toAscii().data());
            }
            else
            {
                if (grabberStartedCount() > 0)
                {
                    retVal += stopCamera();
                }

                retVal += checkError(PCO_SetBinning(m_hCamera,newbinX, newbinY));
                if(!retVal.containsError())
                {
                    it->setVal<int>(newbinX*100+newbinY);

                    WORD wRoiX0, wRoiY0, wRoiX1, wRoiY1;
                    retVal += checkError(PCO_GetROI(m_hCamera, &wRoiX0, &wRoiY0, &wRoiX1, &wRoiY1));

                    if(!retVal.containsError())
                    {
                        m_params["x0"].setVal<int>(wRoiX0-1);
                        m_params["y0"].setVal<int>(wRoiY0-1);
                        m_params["x1"].setVal<int>(wRoiX1  - 1);
                        m_params["y1"].setVal<int>(wRoiY1 - 1);
                        static_cast<ito::IntMeta*>( m_params["x1"].getMeta() )->setMax(m_caminfo.wMaxHorzResStdDESC / newbinX - 1);
                        static_cast<ito::IntMeta*>( m_params["y1"].getMeta() )->setMax(m_caminfo.wMaxVertResStdDESC / newbinY - 1);
                        static_cast<ito::IntMeta*>( m_params["x0"].getMeta() )->setMax(m_params["x1"].getVal<double>());
                        static_cast<ito::IntMeta*>( m_params["y0"].getMeta() )->setMax(m_params["y1"].getVal<double>());

                        static_cast<ito::IntMeta*>( m_params["sizex"].getMeta() )->setMax(m_params["x1"].getMax()-m_params["x0"].getMin()+1); 
                        static_cast<ito::IntMeta*>( m_params["sizey"].getMeta() )->setMax(m_params["y1"].getMax()-m_params["y0"].getMin()+1); 

                        m_params["sizex"].setVal<int>(m_params["x1"].getVal<int>()-m_params["x0"].getVal<int>()+1); 
                        m_params["sizey"].setVal<int>(m_params["y1"].getVal<int>()-m_params["y0"].getVal<int>()+1); 
                    }                    
                }

                if (grabberStartedCount() > 0)
                {
                    retVal += startCamera();
                }
            }
                    
        }

        else if (key == "x0" || key == "x1" || key == "y0" || key == "y1")
        {
            if (grabberStartedCount() > 0)
            {
                retVal += stopCamera();
            }

            // Adapted parameters and send out depending parameter
            WORD wRoiX0 = m_params["x0"].getVal<int>() + 1;
            WORD wRoiY0 = m_params["y0"].getVal<int>() + 1; 
            WORD wRoiX1 = m_params["x1"].getVal<int>() + 1; 
            WORD wRoiY1 = m_params["y1"].getVal<int>() + 1;

            retVal += checkError(PCO_SetROI(m_hCamera, wRoiX0, wRoiY0, wRoiX1, wRoiY1));
                        
            if(!retVal.containsError())
            {
                retVal += checkError(PCO_ArmCamera(m_hCamera));
                m_params["x0"].setVal<int>(wRoiX0 - 1);
                m_params["y0"].setVal<int>(wRoiY0 - 1);
                m_params["x1"].setVal<int>(wRoiX1 - 1);
                m_params["y1"].setVal<int>(wRoiY1 - 1);

                static_cast<ito::IntMeta*>( m_params["x0"].getMeta() )->setMax(m_params["x1"].getVal<double>());
                static_cast<ito::IntMeta*>( m_params["y0"].getMeta() )->setMax(m_params["y1"].getVal<double>());                    
                static_cast<ito::IntMeta*>( m_params["x1"].getMeta() )->setMin(m_params["x0"].getVal<double>());
                static_cast<ito::IntMeta*>( m_params["y1"].getMeta() )->setMin(m_params["y0"].getVal<double>());

                m_params["sizex"].setVal<int>(m_params["x1"].getVal<int>()-m_params["x0"].getVal<int>()+1); 
                m_params["sizey"].setVal<int>(m_params["y1"].getVal<int>()-m_params["y0"].getVal<int>()+1);
            }

            if (grabberStartedCount() > 0)
            {
                retVal += startCamera();
            }
        }

        else if (key == "integration_time")
        {
            double exposure = val->getVal<double>()*1000;

            WORD timebase = 2;

            if(exposure < 1)
            {
                exposure *= 1000;  
                timebase--;
            }
            if(exposure < 1)
            {
                exposure *= 1000;  
                timebase--;        
            }
            retVal += checkError(PCO_SetDelayExposureTime(m_hCamera,  
                                            0,        // DWORD dwDelay
                                            (DWORD)exposure,//(DWORD)dwExposure,
                                            0,        // WORD wTimeBaseDelay, Timebase: 0-ns; 1-us; 2-ms 
                                            (WORD)timebase));    // WORD wTimeBaseExposure

            if (!retVal.containsError())
            {
                it->copyValueFrom( &(*val));
            }                   
        }
        else if (key == "coolingSetPointTemperature")
        {
            short coolset = val->getVal<int>();
            retVal += checkError(PCO_SetCoolingSetpointTemperature(m_hCamera, coolset));
            if(!retVal.containsError())
            {
                retVal += checkError(PCO_GetCoolingSetpointTemperature(m_hCamera, &coolset));
                it->setVal<int>(coolset);
            }

        }
        else if (key == "IRSensitivity")
        {
            WORD IRSens = val->getVal<int>();
            retVal += checkError(PCO_SetIRSensitivity(m_hCamera, IRSens));
            if(!retVal.containsError())
            {
                retVal += checkError(PCO_GetIRSensitivity(m_hCamera, &IRSens));
                it->setVal<int>(IRSens);
            }
        }
        else if (key == "Pixelrate")
        {
            DWORD Pixelrate = val->getVal<int>() * 1000000;
            retVal += checkError(PCO_SetPixelRate(m_hCamera, Pixelrate));
            if(!retVal.containsError())
            {
                retVal += checkError(PCO_GetPixelRate(m_hCamera, &Pixelrate));
                it->setVal<int>(Pixelrate / 1000000);
            }
        }
        else if (key == "ConversionFactor")
        {
            WORD ConvFact = val->getVal<double>() * 100.0;
            retVal += checkError(PCO_SetConversionFactor(m_hCamera, ConvFact));
            if(!retVal.containsError())
            {
                retVal += checkError(PCO_GetConversionFactor(m_hCamera, &ConvFact));
                it->setVal<double>(ConvFact / 100.0);
            }
        }
        else
        {
            retVal += ito::RetVal::format(ito::retError,0, "parameter '%s' cannot be set", it->getName());
        }
    }

    if (!retVal.containsError())
    {
        emit parametersChanged(m_params);
    }

    retVal += checkData(); //check if image must be reallocated

    if (running)
    {
        retVal += this->startDevice(0);
        setGrabberStarted(running);
    }

    if (waitCond) 
    {
        waitCond->returnValue = retVal;
        waitCond->release();
    }

    return retVal;
}


//----------------------------------------------------------------------------------------------------------------------------------
//! init method which is called by the addInManager after the initiation of a new instance of PCOCamera.
/*!
    This init method gets the mandatory and optional parameter vectors of type tParam and must copy these given parameters to the
    internal m_params-vector. Notice that this method is called after that this instance has been moved to its own (non-gui) thread.

    \param [in] paramsMand is a pointer to the vector of mandatory tParams.
    \param [in] paramsOpt is a pointer to the vector of optional tParams.
    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk
*/
ito::RetVal PCOCamera::init(QVector<ito::ParamBase> *paramsMand, QVector<ito::ParamBase> *paramsOpt, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);

    //PCO_General strGeneral;
    PCO_CameraType strCamType;
    //PCO_Sensor strSensor;
    //PCO_Timing strTiming;
    //PCO_Storage strStorage;
    //PCO_Recording strRecording;

    //DWORD dwValidImageCnt;
    //DWORD dwMaxImageCnt;

    char errbuffer[400]={0};
    ito::RetVal retVal(ito::retOk,0,"");
    int ret = 0;

    retVal += checkError(PCO_OpenCamera(&m_hCamera, 0));

    if(!retVal.containsError())
    {
        strCamType.wSize = sizeof(PCO_CameraType);
        retVal += checkError(PCO_GetCameraType(m_hCamera, &strCamType));

        if (!retVal.containsError())
        {
            char name[] = {0,0,0,0,0,0,0,0,0,0};
            switch (strCamType.wCamType)
            {
                case CAMERATYPE_PCO1200HS:
                  _snprintf(name, 9, "PCO.1200");
                  break;
                case CAMERATYPE_KODAK1300OEM:
                case CAMERATYPE_284XS:
                case CAMERATYPE_PCO1300:
                  _snprintf(name, 9, "PCO.1300");
                  break;
                case CAMERATYPE_PCO1600:
                  _snprintf(name, 9, "PCO.1600");
                  break;
                case CAMERATYPE_PCO2000:
                  _snprintf(name, 9, "PCO.2000");
                  break;
                case CAMERATYPE_PCO4000:
                  _snprintf(name, 9, "PCO.4000");
                  break;
                default:
                  _snprintf(name, 9, "PCO.????");
            }
            m_params["name"].setVal<char*>(name, (int)strlen(name));
            setIdentifier(QString("%1 (%2)").arg(name).arg( getID() ));
        }
    }
    
    if(!retVal.containsError())
    {
        m_caminfo.wSize =sizeof(PCO_Description);
        retVal += checkError(PCO_GetCameraDescription(m_hCamera, &m_caminfo));
    }

    if(!retVal.containsError())
    {
        //set min, max, current value of coolingSetPointTemperature
        ito::IntMeta *intMeta = dynamic_cast<ito::IntMeta*>(m_params["coolingSetPointTemperature"].getMeta());
        intMeta->setMin(m_caminfo.sMinCoolSetDESC);
        intMeta->setMax(m_caminfo.sMaxCoolSetDESC);
        short coolset;
        retVal += checkError(PCO_GetCoolingSetpointTemperature(m_hCamera, &coolset));
        if(!retVal.containsError())
        {
            m_params["coolingSetPointTemperature"].setVal<int>(coolset);
        }
    }

    if(!retVal.containsError())
    {
        //check if camera supports IRSensitivity and sets the value if so
        WORD IRSens;
        int sensError = PCO_GetIRSensitivity(m_hCamera, &IRSens);

        if ((sensError & PCO_ERROR_SDKDLL_NOTAVAILABLE) == PCO_ERROR_SDKDLL_NOTAVAILABLE)
        {
            m_params.remove("IRSensitivity");
        }
        else
        {
            retVal += checkError(sensError);
            //set IRSensitivity status
            ito::IntMeta *IntMeta = dynamic_cast<ito::IntMeta*>(m_params["IRSensitivity"].getMeta());
            IntMeta->setMin(0);
            IntMeta->setMax(1);
            IntMeta->setStepSize(1);
        
            if(!retVal.containsError())
            {
                m_params["IRSensitivity"].setVal<int>(IRSens);
            }
        }
    }
    if(!retVal.containsError())
    {
        //set actual Pixelrate
        ito::IntMeta *intMeta = dynamic_cast<ito::IntMeta*>(m_params["Pixelrate"].getMeta());
        intMeta->setMin(m_caminfo.dwPixelRateDESC[0] / 1000000);
        intMeta->setMax(m_caminfo.dwPixelRateDESC[1] / 1000000);

        if(m_caminfo.dwPixelRateDESC[2]==0)
        {
            intMeta->setStepSize(intMeta->getMax() - intMeta->getMin());
        }
        else
        {
            intMeta->setStepSize(1); // if 3 Pixelrates are possible the StepSize might be not constant between them, so it is set to 1.
        }

        DWORD Pixelrate;
        retVal += checkError(PCO_GetPixelRate(m_hCamera, &Pixelrate));
        if(!retVal.containsError())
        {
            m_params["Pixelrate"].setVal<int>(Pixelrate / 1000000);
        }
    }
    if(!retVal.containsError())
    {
        //set actual Conversion factor
        ito::DoubleMeta *DoubleMeta = dynamic_cast<ito::DoubleMeta*>(m_params["ConversionFactor"].getMeta());
        DoubleMeta->setMin(m_caminfo.wConvFactDESC[1] / 100.0);
        DoubleMeta->setMax(m_caminfo.wConvFactDESC[0] / 100.0);
        if(m_caminfo.wConvFactDESC[2]==0)
        {
            DoubleMeta->setStepSize(DoubleMeta->getMax() - DoubleMeta->getMin());
        }
        else
        {
            DoubleMeta->setStepSize(0.1); // if 3 ConversionFactors are possible the StepSize might be not constant between them, so it is set to 0.1.
        }
        WORD ConvFact;
        retVal += checkError(PCO_GetConversionFactor(m_hCamera, &ConvFact));
        if(!retVal.containsError())
        {
            m_params["ConversionFactor"].setVal<double>(ConvFact / 100.0);
        }
    }
    if(!retVal.containsError())
    {
        // set the actual temperatures
        short ccdtemp, camtemp, powtemp;
            retVal += checkError(PCO_GetTemperature(m_hCamera, &ccdtemp, &camtemp, &powtemp));

            if (!retVal.containsError())
            {
                int temps[] = {ccdtemp, camtemp, powtemp};
                m_params["temperatures"].setVal<int*>(temps,3);                
            }
    }

    if(!retVal.containsError())
    {
        ito::IntMeta *IntMeta = dynamic_cast<ito::IntMeta*>(m_params["binning"].getMeta());
        IntMeta->setMin(101);
        IntMeta->setMax(m_caminfo.wMaxBinHorzDESC*100+ m_caminfo.wMaxBinVertDESC);
        WORD hbin, vbin;
        retVal += checkError(PCO_GetBinning(m_hCamera, &hbin, &vbin));
        if(!retVal.containsError())
        {
            m_params["binning"].setVal<int>(hbin*100 + vbin);
        }
    }

  /***********************************************************
    The following step is a must. 
    In SC2, bin proceeds ROI in control.  
    ROI "field of definition" is subject to bin settings.
    Please be advised that this is opposite to SensiCam SDK
  *************************************************************/

    if(!retVal.containsError())
    {
        int bpp = m_caminfo.wDynResDESC;
        m_params["bpp"].setVal<int>(bpp);
        m_params["bpp"].setMeta( new ito::IntMeta(bpp,bpp), true);

        int iVal = (int)m_caminfo.wMaxHorzResStdDESC;

        if(m_caminfo.wRoiHorStepsDESC == 0)
        {
            m_params["x0"].setFlags(ito::ParamBase::Readonly);
            m_params["x1"].setFlags(ito::ParamBase::Readonly);
        }
        if(m_caminfo.wRoiVertStepsDESC == 0)
        {
            m_params["y0"].setFlags(ito::ParamBase::Readonly);
            m_params["y1"].setFlags(ito::ParamBase::Readonly);
        }

        m_params["sizex"].setVal<int>(iVal);
        static_cast<ito::IntMeta*>( m_params["sizex"].getMeta() )->setMax((double)iVal);
        static_cast<ito::IntMeta*>( m_params["x0"].getMeta() )->setMax((double)iVal-1.0);
        m_params["x0"].setVal<int>(0);
        static_cast<ito::IntMeta*>( m_params["x1"].getMeta() )->setMax((double)iVal-1.0);
        m_params["x1"].setVal<int>(iVal-1);

        iVal = (int)m_caminfo.wMaxVertResStdDESC;
        m_params["sizey"].setVal<int>(iVal);
        static_cast<ito::IntMeta*>( m_params["sizey"].getMeta() )->setMax((double)iVal);
        static_cast<ito::IntMeta*>( m_params["y0"].getMeta() )->setMax((double)iVal-1.0);
        m_params["y0"].setVal<int>(0);
        static_cast<ito::IntMeta*>( m_params["y1"].getMeta() )->setMax((double)iVal-1.0);
        m_params["y1"].setVal<int>(iVal-1);
    }

    if(!retVal.containsError())
    {
        unsigned short x0 = m_params["x0"].getVal<int>() + 1;
        unsigned short y0 = m_params["y0"].getVal<int>() + 1;
        unsigned short xsize = m_params["x1"].getVal<int>() + 1;
        unsigned short ysize = m_params["y1"].getVal<int>() + 1;
        ret = PCO_SetROI(m_hCamera,x0,y0,xsize,ysize);
        if (ret != 0)
        {
            _snprintf(errbuffer, 399, "PCO_SetROI error (hex): %lx", (unsigned long)ret);
            retVal += ito::RetVal(ito::retError, 0, errbuffer);
        }
    }

    if(!retVal.containsError())
    {
        // Set trigger mode to auto trigger
        retVal += checkError(PCO_SetTriggerMode(m_hCamera, 0x0000));
    }

    if(!retVal.containsError())
    {
        // Set Storage mode to recorder mode
        retVal += checkError(PCO_SetStorageMode(m_hCamera, 0x0000));
    }

    if(!retVal.containsError())
    {
        // Set recorder submode to ring buffer
        retVal += checkError(PCO_SetRecorderSubmode(m_hCamera, 1));
    }

    if(!retVal.containsError())
    {
        // Set acquire mode to auto = all images taken are stored
        retVal += checkError(PCO_SetAcquireMode(m_hCamera, 0x0000));
    }

    // prepare delay exposure time
    if(!retVal.containsError())
    {
        m_params["integration_time"].setMeta( new ito::DoubleMeta(m_caminfo.dwMinExposureDESC / 1000000000.0, m_caminfo.dwMaxExposureDESC / 1000.0, m_caminfo.dwMinExposureStepDESC / 1000000000.0), true);

        double exposure = m_params["integration_time"].getVal<double>()*1000;
        WORD timebase = 2;

        if(exposure < 1)
        {
            exposure *= 1000;  
            timebase--;
        }
        if(exposure < 1)
        {
            exposure *= 1000;  
            timebase--;        
        }


        retVal += checkError(PCO_SetDelayExposureTime(m_hCamera,  
                                        0,        // DWORD dwDelay
                                        (DWORD)exposure,//(DWORD)dwExposure,
                                        0,        // WORD wTimeBaseDelay, Timebase: 0-ns; 1-us; 2-ms 
                                        (WORD)timebase));    // WORD wTimeBaseExposure
    }
  
    // set gain
    //pcofw->grab_gain=this->m_caminfo.wConvFactDESC[1];

    //if(!retVal.containsError())
    //{
    //    if (m_caminfo.wConvFactDESC[1] > 0)
    //    {
    //        ret = PCO_SetConversionFactor(m_hCamera, m_caminfo.wConvFactDESC[1]);
    //        if (ret != 0)
    //        {
    //            _snprintf(errbuffer, 399,  "PCO_SetDelayExposureTime (hex): %lx", (unsigned long)ret);
    //            retVal += ito::RetVal(ito::retError, 0, errbuffer);
    //        }
    //    }
    //}
  
        /***********************************************************
        Cam Ram can be partitioned and set active. 
        by deafult, it is a single piece. An ID is returned
        *************************************************************/
  
    if(!retVal.containsError())
    {
        retVal += checkError(PCO_GetActiveRamSegment(m_hCamera, &m_wActSeg));
    }
  
    /***********************************************************
    ArmCamera validates settings.  
    recorder must be turned off to ArmCamera
    *************************************************************/
  
    if(!retVal.containsError())
    {
        WORD recstate;
        retVal += checkError(PCO_GetRecordingState(m_hCamera, &recstate));
        if (recstate > 0)
        {
            retVal += checkError(PCO_SetRecordingState(m_hCamera, 0x0000));
            if(!retVal.containsError())
            {
                retVal += checkError(PCO_CancelImages(m_hCamera));
            }
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
//! close method which is called before that this instance is deleted by the PCOCameraInterface
/*!
    notice that this method is called in the actual thread of this instance.

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk
    \sa ItomSharedSemaphore
*/
ito::RetVal PCOCamera::close(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    char errbuffer[400]={0};
    int ret = 0;
    ito::RetVal retVal = stopCamera();

    if (ret != 0)
    {
        _snprintf(errbuffer, 399, "PCO_FreeBuffer error(hex): %lx", (unsigned long)ret);
        retVal += ito::RetVal(ito::retError, 0, errbuffer);
    }

    ret = PCO_CloseCamera(m_hCamera);// Correct code...

    if(m_timerID > 0)
    {
        killTimer(m_timerID);
        m_timerID = 0;
    }

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
// function checkdata moved into addInGrabber.cpp -> standard for all cameras / ADDA


//----------------------------------------------------------------------------------------------------------------------------------
//! With startDevice this camera is initialized.
/*!
    In the PCOCamera, this method does nothing. In general, the hardware camera should be intialized in this method and necessary memory should be allocated.

    \note This method is similar to VideoCapture::open() of openCV

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if starting was successfull, retWarning if startDevice has been calling at least twice.
*/
ito::RetVal PCOCamera::startDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retVal = ito::retOk;

    checkData(); //this will be reallocated in this method.

    incGrabberStarted();

    if(grabberStartedCount() == 1)
    {
        retVal += startCamera();
    }

    if(waitCond)
    {
        waitCond->returnValue = retVal;
        waitCond->release();
    }

    return retVal;
}
         
//----------------------------------------------------------------------------------------------------------------------------------
//! With stopDevice the camera device is stopped (opposite to startDevice)
/*!
    In this PCOCamera, this method does nothing. In general, the hardware camera should be closed in this method.

    \note This method is similar to VideoCapture::release() of openCV

    \param [in] waitCond is the semaphore (default: NULL), which is released if this method has been terminated
    \return retOk if everything is ok, retError if camera wasn't started before
    \sa startDevice
*/
ito::RetVal PCOCamera::stopDevice(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retVal = ito::retOk;

    decGrabberStarted();
    if(grabberStartedCount() == 0)
    {
        retVal += stopCamera();        
    }
    else if(grabberStartedCount() < 0)
    {
        retVal += ito::RetVal(ito::retError, 1001, tr("StopDevice of PCOCamera can not be executed, since camera has not been started.").toAscii().data());
        setGrabberStarted(0);
    }


    if(waitCond)
    {
        waitCond->returnValue = retVal;
        waitCond->release();
    }

    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PCOCamera::stopCamera()
{
    ito::RetVal retVal;
    WORD wRecState;
    PCO_GetRecordingState(m_hCamera, &wRecState);

    if (wRecState > 0)
    {
        retVal += checkError(PCO_SetRecordingState(m_hCamera, 0x0000)); //stops recording
        retVal += checkError(PCO_CancelImages(m_hCamera)); //removes pending buffers from the drivers queue

        if (!retVal.containsError())
        {
            for (short sBufNr = 0; sBufNr < PCO_NUMBER_BUFFERS; ++sBufNr)
            {
                PCOBuffer *buffer = &(m_buffers[sBufNr]);
                retVal += checkError(PCO_FreeBuffer(m_hCamera, buffer->bufNr));
                buffer->bufNr = -1; //request new buffer
                buffer->bufData = NULL;
            }
        }
    }

    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PCOCamera::startCamera()
{
    ito::RetVal retVal;
    WORD wRecState;
    PCO_GetRecordingState(m_hCamera, &wRecState);

    if (wRecState == 0)
    {
        m_hEvent = NULL;

        //recommended order: setBinning, setROI... (done in setParam), then ARM, GetSizes, AllocateBuffer, SetTriggerMode, SetRecordingState
        retVal += checkError(PCO_ArmCamera(m_hCamera));

        WORD roiX0, roiY0, roiX1, roiY1;
        WORD sizeX, sizeY, sizeXMax, sizeYMax;
        retVal += checkError(PCO_GetROI(m_hCamera, &roiX0, &roiY0, &roiX1, &roiY1));
        retVal += checkError(PCO_GetSizes(m_hCamera, &sizeX, &sizeY, &sizeXMax, &sizeYMax));
        
        if(!retVal.containsError())
        {
            m_params["sizex"].setVal<int>(sizeX);
            m_params["sizey"].setVal<int>(sizeY);

            retVal += checkError(PCO_CamLinkSetImageParameters(m_hCamera,sizeX,sizeY));

            if(!retVal.containsError())
            {
                DWORD imgsize = sizeX*sizeY*sizeof(WORD);
                if (imgsize % 0x1000)
                {
                    imgsize = imgsize / 0x1000;
                    imgsize += 2;
                    imgsize *= 0x1000;
                }
                else
                {
                    imgsize += 0x1000;
                }

                for (short sBufNr = 0; sBufNr < PCO_NUMBER_BUFFERS; ++sBufNr)
                {
                    PCOBuffer *buffer = &(m_buffers[sBufNr]);
                    buffer->bufNr = -1; //request new buffer
                    buffer->bufData = NULL;
                    buffer->bufQueued = false;
                    buffer->bufEvent = NULL;
                    buffer->bufError = false;
                    retVal += checkError(PCO_AllocateBuffer(m_hCamera, &(buffer->bufNr), imgsize, &(buffer->bufData), &(buffer->bufEvent)));
                }
            }
        }

        retVal += checkError(PCO_SetTriggerMode(m_hCamera, 0x0001)); //software trigger

        if (!retVal.containsError())
        {
            //queue all images
            for (short sBufNr = 0; sBufNr < PCO_NUMBER_BUFFERS; ++sBufNr)
            {
                PCOBuffer *buffer = &(m_buffers[sBufNr]);
                if (buffer->bufNr >= 0 && buffer->bufQueued == false)
                {
                    retVal += checkError(PCO_AddBufferEx(m_hCamera, 0, 0, buffer->bufNr, sizeX, sizeY, m_caminfo.wDynResDESC));
                    buffer->bufQueued = true;
                }
                
            }
        }

        if(!retVal.containsError())
        {
            retVal += checkError(PCO_ArmCamera(m_hCamera));
            retVal += checkError(PCO_SetRecordingState(m_hCamera, 0x0001));
        }
    }

    return retVal;
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
ito::RetVal PCOCamera::acquire(const int trigger, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retVal = ito::retOk;
    int ret = 0;
    WORD intrigger = 0;

    unsigned short xsize = m_params["sizex"].getVal<int>();
    unsigned short ysize = m_params["sizey"].getVal<int>();

    if(grabberStartedCount() <= 0)
    {
        retVal += ito::RetVal(ito::retError, 1002, tr("Acquire of PCOCamera can not be executed, since camera has not been started.").toAscii().data());
    }
    else
    {
       
        if(!retVal.containsError())
        {
            retVal += checkError(PCO_ForceTrigger(m_hCamera,&intrigger));
        }

        if(retVal.containsError()) // || !intrigger)
        {
            retVal += ito::retError;
            this->m_isgrabbing = false;
        }
        else
        {
            this->m_isgrabbing = true;
        }
    }

    if(waitCond)
    {
        waitCond->returnValue = retVal;
        waitCond->release();
    }

    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PCOCamera::retrieveData(ito::DataObject *externalDataObject)
{
    ito::RetVal retVal(ito::retOk);
    int ret = 0;
    int timeOutMS = m_params["integration_time"].getVal<double>() * 1300 + 500; // *1300 because of ms and factor 1,3 and +500 minimum timeout for short integration time    
    unsigned long imglength = 0;
    long lcopysize = 0;
    long lsrcstrpos = 0;
    int y  = 0;
    //int maxxsize = (int)m_params["sizex"].getMax();
    //int maxysize = (int)m_params["sizey"].getMax();
    int curxsize = m_params["sizex"].getVal<int>();
    int curysize = m_params["sizey"].getVal<int>();
    //int x0 = m_params["x0"].getVal<int>();
    //int y0 = m_params["y0"].getVal<int>();

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

    QElapsedTimer timer;
    timer.start();
    bool waitingSuccessful = false;
    WORD *wBuf = NULL;
    HANDLE Event = NULL;
    short bufNr = 255; //invalid

    HANDLE handles[PCO_NUMBER_BUFFERS];
    short bufNumbers[PCO_NUMBER_BUFFERS];
    DWORD ncount = 0;

    for (short i = 0; i < PCO_NUMBER_BUFFERS; ++i)
    {
        //list all buffers that could potentially fire an event with a really new image now
        if (m_buffers[i].bufQueued && !m_buffers[i].bufError)
        {
            bufNumbers[ncount] = i;
            handles[ncount++] = m_buffers[i].bufEvent;
        }
    }

    while (timer.elapsed() < timeOutMS)
    {
        ret = WaitForMultipleObjects(ncount, handles, false, 500);

        if (ret >= WAIT_OBJECT_0 && ret < (WAIT_OBJECT_0 + PCO_NUMBER_BUFFERS)) //waitForSingleObject is done
        {
            bufNr = bufNumbers[ret - WAIT_OBJECT_0];
            retVal += checkError(PCO_GetBuffer(m_hCamera, bufNr, &wBuf, &Event));

            for (short i = 0; i < PCO_NUMBER_BUFFERS; ++i)
            {
                if (m_buffers[i].bufNr == bufNr)
                {
                    m_buffers[i].bufQueued = false;
                    m_buffers[i].bufError = false;
                }
            }
            waitingSuccessful = true;
            break;
        }
        else if (ret != WAIT_TIMEOUT)
        {
            retVal += ito::RetVal(ito::retError, 1002, tr("getVal of PCOCamera failed.").toAscii().data());
            break;
        }
        else //timeout -> is ok
        {
            setAlive();
        }
    }

    if (!waitingSuccessful)
    {
        retVal += ito::RetVal(ito::retError, 1001, tr("timeout while waiting for image from PCO camera device").toAscii().data());
    }
    else if (!retVal.containsError())
    {      
        int bpp = m_params["bpp"].getVal<int>();
        if(bpp <= 8)
        { 
            ito::uint8 *cbuf=(ito::uint8*)wBuf;
            if(cbuf == NULL)
            {
                retVal += ito::RetVal(ito::retError, 1002, tr("getVal of PCOCamera failed, since retrieved NULL-Pointer.").toAscii().data());
            }
            else
            {
                if(copyExternal) retVal += externalDataObject->copyFromData2D<ito::uint8>((ito::uint8*)cbuf, curxsize, curysize);
                if(!copyExternal || hasListeners) retVal += m_data.copyFromData2D<ito::uint8>((ito::uint8*)cbuf, curxsize, curysize);
            }
        }
        else if(bpp <= 16)
        {
            ito::uint16 *cbuf=(ito::uint16*)wBuf;
            if(cbuf == NULL)
            {
                retVal += ito::RetVal(ito::retError, 1002, tr("getVal of PCOCamera failed, since retrieved NULL-Pointer.").toAscii().data());
            }
            else
            {
                if(copyExternal) retVal += externalDataObject->copyFromData2D<ito::uint16>((ito::uint16*)cbuf, curxsize, curysize);
                if(!copyExternal || hasListeners) retVal += m_data.copyFromData2D<ito::uint16>((ito::uint16*)cbuf, curxsize, curysize);
            }

        }
        else if(bpp <= 32)
        {
            ito::int32 *cbuf=(ito::int32*)wBuf;
            if(cbuf == NULL)
            {
                retVal += ito::RetVal(ito::retError, 1002, tr("getVal of PCOCamera failed, since retrieved NULL-Pointer.").toAscii().data());
            }
            {
                if(copyExternal) retVal += externalDataObject->copyFromData2D<ito::int32>((ito::int32*)cbuf, curxsize, curysize);
                if(!copyExternal || hasListeners) retVal += m_data.copyFromData2D<ito::int32>((ito::int32*)cbuf, curxsize, curysize);
            }

        }
        else
        {
            retVal += ito::RetVal(ito::retError, 1002, tr("getVal of PCOCamera failed, since undefined bitdepth.").toAscii().data());            
        }
        this->m_isgrabbing = false;
    }

    if (bufNr < 255) //try to requeue buffer
    {
        for (short i = 0; i < PCO_NUMBER_BUFFERS; ++i)
        {
            if (!m_buffers[i].bufQueued)
            {
                retVal += checkError(PCO_AddBufferEx(m_hCamera, 0, 0, m_buffers[i].bufNr, curxsize, curysize, m_caminfo.wDynResDESC));
                m_buffers[i].bufQueued = true;
            }
        }
    }

    if (retVal.containsError()) //maybe all buffers are pending, no free buffers, try to requeue them though
    {
        int icount = 0;
        PCO_GetPendingBuffer(m_hCamera, &icount);

        if (icount == PCO_NUMBER_BUFFERS)
        {
            //queue all images
            for (short sBufNr = 0; sBufNr < PCO_NUMBER_BUFFERS; ++sBufNr)
            {
                PCOBuffer *buffer = &(m_buffers[sBufNr]);
                if (buffer->bufNr >= 0)
                {
                    retVal += checkError(PCO_AddBufferEx(m_hCamera, 0, 0, buffer->bufNr, curxsize, curysize, m_caminfo.wDynResDESC));
                    buffer->bufQueued = true;
                    buffer->bufError = false;
                }
                
            }
        }
    }

    return retVal;
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
ito::RetVal PCOCamera::getVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::DataObject *dObj = reinterpret_cast<ito::DataObject *>(vpdObj);

    ito::RetVal retVal(ito::retOk);

    retVal += retrieveData();

    if(!retVal.containsError())
    {
        if(dObj == NULL)
        {
            retVal += ito::RetVal(ito::retError, 1004, tr("data object of getVal is NULL or cast failed").toAscii().data());
        }
        else
        {
            retVal += sendDataToListeners(0); //don't wait for live image, since user should get the image as fast as possible.

            (*dObj) = this->m_data;
        }
    }

    if (waitCond) 
    {
        waitCond->returnValue=retVal;
        waitCond->release();
    }

    return retVal;
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
ito::RetVal PCOCamera::copyVal(void *vpdObj, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retVal(ito::retOk);
    ito::DataObject *dObj = reinterpret_cast<ito::DataObject *>(vpdObj);

    if(!dObj)
    {
        retVal += ito::RetVal(ito::retError, 0, tr("Empty object handle retrieved from caller").toAscii().data());
    }
    else
    {
        retVal += checkData(dObj);  
    }

    if(!retVal.containsError())
    {
        retVal += retrieveData(dObj);  
    }

    if(!retVal.containsError())
    {
        sendDataToListeners(0); //don't wait for live image, since user should get the image as fast as possible.
    }

    if (waitCond) 
    {
        waitCond->returnValue = retVal;
        waitCond->release();
    }

    return retVal;
}

//----------------------------------------------------------------------------------------------------------------------------------
void PCOCamera::dockWidgetVisibilityChanged(bool visible)
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