/* ********************************************************************
    Plugin "PcoPixelFly" for itom software
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

#ifndef DIALOGPCOPixelFly_H
#define DIALOGPCOPixelFly_H

#include "common/addInGrabber.h"
//#include "common/sharedStructures.h"

#include <QtGui>
#include <qdialog.h>

#include "ui_dialogPCOPixelFly.h"

class dialogPCOPixelFly : public QDialog
{
    Q_OBJECT

    public:
        dialogPCOPixelFly(ito::AddInGrabber *grabber):m_Grabber(grabber){m_paramsVals.clear(); ui.setupUi(this);};
        ~dialogPCOPixelFly() {m_paramsVals.clear();};
        int getVals(QMap<QString, ito::Param> *paramVals);
        int sendVals(void);

    private:
        ito::AddInGrabber *m_Grabber;

        Ui::dialogPCOPixelFly ui;
        QMap<QString, ito::Param> m_paramsVals;

    signals:

    public slots:

        void valuesChanged(QMap<QString, ito::Param> params);

    private slots:
        void on_pushButton_setSizeXMax_clicked();    //!< Set x-size to maximum valid value
        void on_pushButton_setSizeYMax_clicked();    //!< Set y-sizes to maximum valid value
        void on_applyButton_clicked();    //!< Write the current settings to the internal paramsVals and sent them to the grabber

        void on_spinBox_x0_valueChanged(int value);
        void on_spinBox_x1_valueChanged(int value);
        void on_spinBox_y0_valueChanged(int value);
        void on_spinBox_y1_valueChanged(int value);
        void on_spinBox_binX_valueChanged(int value);
        void on_spinBox_binY_valueChanged(int value);

};

#endif
