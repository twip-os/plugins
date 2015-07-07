/* ********************************************************************
    Plugin "Standa ST8SMC4USB" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2015, Institut f�r Technische Optik (ITO),
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

#ifndef DIALOGST8SMC4USB_H
#define DIALOGST8SMC4USB_H

#include "common/sharedStructures.h"
#include "common/sharedStructuresQt.h"
#include "common/abstractAddInConfigDialog.h"

#include "ui_dialogST8SMC4USB.h"

#include <qdialog.h>
#include <qstring.h>
#include <qmap.h>
#include <qabstractbutton.h>
#include <qvector.h>
#include <qsharedpointer.h>
#include <qcombobox.h>
#include <qsignalmapper.h>
#include <qcommandlinkbutton.h>
#include <qspinbox.h>

namespace ito
{
    class AddInBase; //forward declaration
}

class DialogST8SMC4USB : public ito::AbstractAddInConfigDialog 
{
    Q_OBJECT

    public:
        DialogST8SMC4USB(ito::AddInBase *actuator);
        ~DialogST8SMC4USB();

        ito::RetVal applyParameters();

    private:
        void enableDialog(bool enabled);

        Ui::DialogST8SMC4USB ui;
        bool m_firstRun;

    public slots:
        void parametersChanged(QMap<QString, ito::Param> params);

    private slots:
        void on_buttonBox_clicked(QAbstractButton* btn);

};

#endif
