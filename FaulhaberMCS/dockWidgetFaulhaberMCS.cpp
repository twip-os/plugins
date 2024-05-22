/* ********************************************************************
    Plugin "FaulhaberMCS" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2024, Institut für Technische Optik (ITO),
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

#include "dockWidgetFaulhaberMCS.h"
#include "motorAxisController.h"

//----------------------------------------------------------------------------------------------------------------------------------
DockWidgetFaulhaberMCS::DockWidgetFaulhaberMCS(ito::AddInActuator* actuator) :
    AbstractAddInDockWidget(actuator), m_pActuator(actuator), m_inEditing(false), m_firstRun(true)
{
    ui.setupUi(this);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
void DockWidgetFaulhaberMCS::parametersChanged(QMap<QString, ito::Param> params)
{
    ui.checkBoxReadyToSwitchOn->setChecked(params["readyToSwitchOn"].getVal<int>());
    ui.checkBoxSwitchOn->setChecked(params["switchedOn"].getVal<int>());
    ui.checkBoxOperationEnabled->setChecked(params["operationEnabled"].getVal<int>());
    ui.checkBoxFault->setChecked(params["fault"].getVal<int>());
    ui.checkBoxVoltageEnabled->setChecked(params["voltageEnabled"].getVal<int>());
    ui.checkBoxQuickStop->setChecked(params["quickStop"].getVal<int>());
    ui.checkBoxSwitchOnDisabled->setChecked(params["switchOnDisabled"].getVal<int>());
    ui.checkBoxWarning->setChecked(params["warning"].getVal<int>());
    ui.checkBoxTargetReached->setChecked(params["targetReached"].getVal<int>());
    ui.checkBoxInternalLimitActive->setChecked(params["internalLimitActive"].getVal<int>());
    ui.checkBoxSetPointAcknowledged->setChecked(params["setPointAcknowledged"].getVal<int>());
    ui.checkBoxFollowingError->setCheckable(params["followingError"].getVal<int>());
}

//----------------------------------------------------------------------------------------------------------------------------------
void DockWidgetFaulhaberMCS::identifierChanged(const QString& identifier)
{
    ui.lblIdentifier->setText(identifier);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
void DockWidgetFaulhaberMCS::enableWidget(bool enabled)
{
    ui.axisController->setEnabled(enabled);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
void DockWidgetFaulhaberMCS::dockWidgetVisibilityChanged(bool visible)
{
    if (visible)
    {
        // to connect the signals
        QPointer<ito::AddInActuator> actuator(m_pActuator);
        ui.axisController->setActuator(actuator);
        ui.axisController->setNumAxis(1);
        ui.axisController->setAxisType(0, MotorAxisController::TypeRotational);
    }
    else
    {
        ui.axisController->setActuator(QPointer<ito::AddInActuator>());
    }
}
