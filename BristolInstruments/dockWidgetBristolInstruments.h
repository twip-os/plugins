/* ********************************************************************
    Template for a camera / grabber plugin for the software itom
    
    You can use this template, use it in your plugins, modify it,
    copy it and distribute it without any license restrictions.
*********************************************************************** */

#ifndef DOCKWIDGETBRISTOLINSTRUMENTS_H
#define DOCKWIDGETBRISTOLINSTRUMENTS_H

#include "common/addInInterface.h"
#include "common/abstractAddInDockWidget.h"

#include <qwidget.h>
#include <qmap.h>
#include <qstring.h>

#include "ui_dockWidgetBristolInstruments.h"

class DockWidgetBristolInstruments : public ito::AbstractAddInDockWidget
{
    Q_OBJECT

    public:
        DockWidgetBristolInstruments(ito::AddInDataIO *grabber);
        ~DockWidgetBristolInstruments() {};

    private:
        Ui::DockWidgetBristolInstruments ui;
        bool m_inEditing;
        bool m_firstRun;

    public slots:
        void parametersChanged(QMap<QString, ito::Param> params);
        void identifierChanged(const QString &identifier);

    private slots:
        //add here slots connected to changes of any widget
        //example:
        //void on_contrast_valueChanged(int i);
};

#endif
