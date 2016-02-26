/* ********************************************************************
    Plugin "PIPiezoControl" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2016, Institut fuer Technische Optik (ITO),
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

#ifndef DOCKWIDGETNANOTECSTEPMOTOR_H
#define DOCKWIDGETNANOTECSTEPMOTOR_H

#include "common/abstractAddInDockWidget.h"
#include "common/addInInterface.h"
#include <qsignalmapper.h>
#include <qspinbox.h>

#include <qmap.h>
#include <qstring.h>

#include "ui_dockWidgetNanotecStepMotor.h"

class DockWidgetNanotecStepMotor : public ito::AbstractAddInDockWidget
{
    Q_OBJECT

    public:
        DockWidgetNanotecStepMotor(ito::AddInActuator * myPlugin);
        ~DockWidgetNanotecStepMotor();

    private:
        bool firstRun;
        QVector<double> m_absPosTarget;

        void enableWidget(bool enabled);
        void createUiListEntry(const int i);
        
        QSignalMapper *m_pIncSignalMapper;
        QSignalMapper *m_pDecSignalMapper;
        QSignalMapper *m_pGoSignalMapper;
        QSignalMapper *m_pAbsPosSignalMapper;

        QVector<QDoubleSpinBox*> m_pDestSpinBoxes;
        QVector<QDoubleSpinBox*> m_pCurrSpinBoxes;
        QVector<QPushButton*> m_pIncButtons;
        QVector<QPushButton*> m_pDecButtons;
        QVector<QPushButton*> m_pGoButtons;

        QList<QFrame*> m_pListElements;
        Ui::DockWidgetNanotecStepMotor ui;

    public slots:
        void parametersChanged(QMap<QString, ito::Param> params);
        void identifierChanged(const QString &identifier) {};
        
        void actuatorStatusChanged(QVector<int> status, QVector<double> actPosition);
        void targetChanged(QVector<double> targetPositions);

    private slots:
        // Slots from the signalmapper
        void incBtnClicked(const int & i);    
        void decBtnClicked(const int & i);
        void goBtnClicked(const int & i);
        void absDestPosChanged(const int & i);

        void on_btnStart_clicked();
        void on_btnRefresh_clicked();
        void on_btnCancel_clicked();
};

#endif
