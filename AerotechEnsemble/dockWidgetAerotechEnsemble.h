#ifndef DOCKWIDGETAEROTECHENSEMBLE_H
#define DOCKWIDGETAEROTECHENSEMBLE_H

/**\file aerotechEnsemble.h
* \brief In this file the class of the non modal dialog for the AerotechEnsemble are specified
* 
*/
#include "common/sharedStructures.h"
#include "common/sharedStructuresQt.h"

#include "ui_dockWidgetAerotechEnsemble.h"    //! Header-file generated by Qt-Gui-Editor which has to be called

#include <QtGui>
#include <qwidget.h>
#include <qmap.h>
#include <qstring.h>
#include <qvector.h>
#include <qsignalmapper.h>

namespace ito //Forward-Declaration
{
    class AddInActuator;
}

//----------------------------------------------------------------------------------------------------------------------------------
class DockWidgetAerotechEnsemble : public QWidget
{
    Q_OBJECT
    public:
        DockWidgetAerotechEnsemble(QMap<QString, ito::Param> params, int uniqueID, ito::AddInActuator *actuator); //, ito::AddInActuator * myPlugin);    //!< Constructor called by AerotechEnsemble::Constructor
        ~DockWidgetAerotechEnsemble() {};

    private:
        Ui::DockWidgetAerotechEnsemble ui;    //! Handle to the dialog
        ito::AddInActuator *m_actuator;

        int m_numaxis;                    //! Number of axis
        void CheckAxisNums(QMap<QString, ito::Param> params);    //! This functions checks all axis after parameters has changed and blocks unspecified axis
        //ito::AddInActuator *m_pMyPlugin;    //! Handle to the attached motor to enable / disable connections
//        bool m_isVisible;

        void enableWidget(bool enabled);
//        void visibleWidget();

        bool m_initialized;

        QVector<QCheckBox*> m_pWidgetEnabled;
        QVector<QDoubleSpinBox*> m_pWidgetActual;
        QVector<QDoubleSpinBox*> m_pWidgetTarget;
        QVector<QPushButton*> m_pWidgetPosInc;
        QVector<QPushButton*> m_pWidgetPosDec;

        QSignalMapper *m_pSignalMapperEnabled;
        QSignalMapper *m_pSignalPosInc;
        QSignalMapper *m_pSignalPosDec;

    signals:
        void MoveRelative(const int axis, const double pos, ItomSharedSemaphore *waitCond = NULL);    //!< This signal is connected to SetPosRel
        void MoveAbsolute(QVector<int> axis,  QVector<double> pos, ItomSharedSemaphore *waitCond = NULL); //!< This signal is connected to SetPosAbs
        void MotorTriggerStatusRequest(bool sendActPosition, bool sendTargetPos);    //!< This signal is connected to RequestStatusAndPosition

    public slots:
        void init(QMap<QString, ito::Param> params, QStringList axisNames);
        void valuesChanged(QMap<QString, ito::Param> params);    //!< Slot to recive the valuesChanged signal
        void actuatorStatusChanged(QVector<int> status, QVector<double> actPosition); //!< slot to receive information about status and position changes.
        void targetChanged(QVector<double> targetPositions);

    private slots:
        void on_pb_Start_clicked();
        void on_pb_Stop_clicked();
        void on_pb_Refresh_clicked();

        void checkEnabledClicked(const int &index);  //!< This button disables the current GUI-Elements for the specified axis
        void posIncrementClicked(const int &index);  //!< If the Botton is clicked a MoveRelative()-Signal is emitted
        void posDecrementClicked(const int &index);  //!< If the Botton is clicked a MoveRelative()-Signal is emitted
};

#endif
