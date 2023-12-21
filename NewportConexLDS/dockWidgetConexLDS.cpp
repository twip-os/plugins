/* ********************************************************************
    Plugin "NewportConexLDS" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2024, Institut f�r Technische Optik (ITO),
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

#include "dockWidgetConexLDS.h"

//----------------------------------------------------------------------------------------------------------------------------------
DockWidgetConexLDS::DockWidgetConexLDS(ito::AddInDataIO* grabber) :
    AbstractAddInDockWidget(grabber), m_inEditing(false), m_firstRun(true)
{
    ui.setupUi(this);
}

//----------------------------------------------------------------------------------------------------------------------------------
void DockWidgetConexLDS::parametersChanged(QMap<QString, ito::Param> params)
{
    if (m_firstRun)
    {
        // first time call
        // get all given parameters and adjust all widgets according to them (min, max, stepSize,
        // values...)

        m_firstRun = false;
    }

    if (!m_inEditing)
    {
        m_inEditing = true;
        // check the value of all given parameters and adjust your widgets according to them (value
        // only should be enough)

        m_inEditing = false;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
// void DockWidgetConexLDS::on_contrast_valueChanged(int i)
// {
// if (!m_inEditing)
// {
// m_inEditing = true;
// QSharedPointer<ito::ParamBase> p(new ito::ParamBase("contrast",ito::ParamBase::Int,d));
// setPluginParameter(p, msgLevelWarningAndError);
// m_inEditing = false;
// }
// }

//----------------------------------------------------------------------------------------------------------------------------------
void DockWidgetConexLDS::identifierChanged(const QString& identifier)
{
    ui.lblIdentifier->setText(identifier);
}
