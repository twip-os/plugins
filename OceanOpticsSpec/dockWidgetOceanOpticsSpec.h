/* ********************************************************************
    Plugin "OceanOpticsSpec" for itom software
    URL: http://www.bitbucket.org/itom/plugins
    Copyright (C) 2014, Institut fuer Technische Optik, Universitaet Stuttgart

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

#ifndef DOCKWIDGETAVANTESAVASPEC_H
#define DOCKWIDGETAVANTESAVASPEC_H

#include "common/abstractAddInDockWidget.h"
#include "common/addInGrabber.h"

#include <qmap.h>
#include <qstring.h>

#include "ui_dockWidgetOceanOpticsSpec.h"

class DockWidgetOceanOpticsSpec : public ito::AbstractAddInDockWidget
{
    Q_OBJECT

    public:
        DockWidgetOceanOpticsSpec(ito::AddInDataIO *grabber);
        ~DockWidgetOceanOpticsSpec() {};

    private:
        Ui::DockWidgetOceanOpticsSpec ui;
        QMap<QString, ito::Param> m_currentParams;
        bool m_inEditing;
        bool m_firstRun;

    public slots:
        void parametersChanged(QMap<QString, ito::Param> params);
        void identifierChanged(const QString &identifier);


    private slots:
        void on_spinBox_average_valueChanged(int d);
        void on_rangeWidget_ROI_minimumValueChanged(int d);
        void on_rangeWidget_ROI_maximumValueChanged(int d);
        void on_doubleSpinBox_integration_time_valueChanged(double d);
		//void on_comboDarkCorrection_currentIndexChanged(int d);
};

#endif
