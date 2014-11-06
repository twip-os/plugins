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

#define ITOM_IMPORT_API
#define ITOM_IMPORT_PLOTAPI

// main-include
#include "AndorSDK3Interface.h"

// standard-includes

// qt-core stuff
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtPlugin>
#include <QtCore/QMetaObject>


// project-includes
#include "pluginVersion.h"
#include "AndorSDK3.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AndorSDK3Interface::AndorSDK3Interface(QObject *parent)
{
    m_autoLoadPolicy = ito::autoLoadNever;
    m_autoSavePolicy = ito::autoSaveNever;

    m_type = ito::typeDataIO | ito::typeGrabber;
    setObjectName("AndorSDK3");

    m_description = QObject::tr("Andor cameras via its SDK3 (Neo, Zyla).");
    

    char docstring[] = \
 ""; 
    m_detaildescription = tr(docstring);

    m_author = PLUGIN_AUTHOR;
    m_version = PLUGIN_VERSION;
    m_minItomVer = MINVERSION;
    m_maxItomVer = MAXVERSION;
    m_license = tr("Licensed under LGPL");
    m_aboutThis = tr( "N.A." );  

    ito::Param param( "camera_idx", ito::ParamBase::Int | ito::ParamBase::In, 0, 31, 0, tr("camera index that should be opened. The first camera is 0, the second 1...").toLatin1().data());
    m_initParamsMand.append(param);
}

//----------------------------------------------------------------------------------------------------------------------------------
AndorSDK3Interface::~AndorSDK3Interface()
{
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal AndorSDK3Interface::getAddInInst( ito::AddInBase **addInInst )
{
    if ( !addInInst )
    {
        return ito::retError;
    }

    NEW_PLUGININSTANCE(AndorSDK3)
    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal AndorSDK3Interface::closeThisInst( ito::AddInBase **addInInst )
{
   REMOVE_PLUGININSTANCE(AndorSDK3)
   return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
// this macro registers the class AndorSDK3Interface with the name AndorSDK3 as plugin for the Qt-System (see Qt-DOC)
#if QT_VERSION < 0x050000
    Q_EXPORT_PLUGIN2(AndorSDK3Interface, AndorSDK3Interface)
#endif