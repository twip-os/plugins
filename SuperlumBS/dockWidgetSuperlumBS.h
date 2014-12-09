/* ********************************************************************
    Plugin "SuperlumBS" for itom software
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

#ifndef DOCKWIDGETSUPERLUMBS_H
#define DOCKWIDGETSUPERLUMBS_H

#include "common/abstractAddInDockWidget.h"
#include "common/addInInterface.h"
#include "common/sharedStructures.h"
#include "common/sharedStructuresQt.h"
#include "common/param.h"
#include "common/retVal.h"

#include <qmap.h>
#include <qstring.h>
#include <qabstractbutton.h>

#include "ui_dockWidgetSuperlumBS.h"

#include <QtGui>
#include <qwidget.h>
#include <qsharedpointer.h>
#include <qmetaobject.h>

class DockWidgetSuperlumBS : public ito::AbstractAddInDockWidget
{
    Q_OBJECT

    public:
        DockWidgetSuperlumBS(int uniqueID, ito::AddInBase *actuator);
        ~DockWidgetSuperlumBS() {};

    private:
        void enableWidget(bool enabled);
        void visibleWidget();

		Ui::DockWidgetSuperlumBS ui;

	    bool m_inEditing;
        bool m_firstRun;

    public slots:
		void parametersChanged(QMap<QString, ito::Param> params);
        void identifierChanged(const QString &identifier);

    private slots:
		void on_btn_Optical_Output_clicked(bool checked);
		void on_comboBox_Remote_activated(int);
		void on_comboBox_Power_Mode_activated(int combo);
		void on_tabWidget_Operation_Mode_currentChanged(int combo);
		void on_doubleSpinBox_MAN_Wavelength_valueChanged(double value);
		void on_doubleSpinBox_AUTO_Speed_valueChanged(double value);
		void on_doubleSpinBox_AUTO_Stop_Wavelength_valueChanged(double value);
		void on_doubleSpinBox_AUTO_Start_Wavelength_valueChanged(double value);
		void on_doubleSpinBox_EXT_Speed_valueChanged(double value);
		void on_doubleSpinBox_EXT_Stop_Wavelength_valueChanged(double value);
		void on_doubleSpinBox_EXT_Start_Wavelength_valueChanged(double value);
		void on_doubleSpinBox_MOD_Wavelength1_valueChanged(double value);
		void on_doubleSpinBox_MOD_Wavelength2_valueChanged(double value);
		void on_comboBox_MOD_frequency_activated(int combo);
};

#endif
