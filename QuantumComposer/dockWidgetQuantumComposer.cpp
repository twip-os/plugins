/* ********************************************************************
    Plugin "QuantumComposer" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2022, Institut fuer Technische Optik (ITO),
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

#include "dockWidgetQuantumComposer.h"

//----------------------------------------------------------------------------------------------------------------------------------
DockWidgetQuantumComposer::DockWidgetQuantumComposer(ito::AddInDataIO* dataIO) :
    AbstractAddInDockWidget(dataIO),
    m_inEditing(false),
    m_firstRun(true)
{
    ui.setupUi(this);
}

//----------------------------------------------------------------------------------------------------------------------------------
void DockWidgetQuantumComposer::parametersChanged(QMap<QString, ito::Param> params)
 {
    

    if (m_firstRun)
    {

        m_firstRun = false;
    }
    
    if (!m_inEditing)
    {
        m_inEditing = true;


        m_inEditing = false;
    }
 }

//----------------------------------------------------------------------------------------------------------------------------------
 void DockWidgetQuantumComposer::identifierChanged(const QString& identifier)
 {
    
}

//----------------------------------------------------------------------------------------------------------------------------------
void DockWidgetQuantumComposer::on_spinBox_gain_valueChanged(int d)
{
    if (!m_inEditing)
    {
        m_inEditing = true;
        QSharedPointer<ito::ParamBase> p(new ito::ParamBase("gain",ito::ParamBase::Double,d/100.0));
        setPluginParameter(p, msgLevelWarningAndError);
        m_inEditing = false;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void DockWidgetQuantumComposer::on_spinBox_offset_valueChanged(int d)
{
    if (!m_inEditing)
    {
        m_inEditing = true;
        QSharedPointer<ito::ParamBase> p(new ito::ParamBase("offset",ito::ParamBase::Double,d/100.0));
        setPluginParameter(p, msgLevelWarningAndError);
        m_inEditing = false;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void DockWidgetQuantumComposer::on_doubleSpinBox_integration_time_valueChanged(double d)
{
    if (!m_inEditing)
    {
        m_inEditing = true;
        QSharedPointer<ito::ParamBase> p(new ito::ParamBase("integration_time",ito::ParamBase::Double,d/1000.0));
        setPluginParameter(p, msgLevelWarningAndError);
        m_inEditing = false;
    }
}



