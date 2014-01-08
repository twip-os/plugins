//#ifndef DIALOGDUMMYMOTOR_H
//#define DIALOGDUMMYMOTOR_H
//
///**\file dialogUSBMotion3XIII.h
//* \brief In this file the class of the modal dialog for the USB Motor Driver Board is described.
//* 
//*\sa dialogDummyMotor, DummyMotor
//*\author Wolfram Lyda
//*\date	Oct2011
//*/
//
//#include "common/sharedStructures.h"
//#include "common/sharedStructuresQt.h"
//#include "common/addInInterface.h"
//
////#include "ui_dialogUSBMotion3XIII.h"	//! Header-file generated by Qt-Gui-Editor which has to be called
//
//#include <QtGui>
//#include <qdialog.h>
//
////----------------------------------------------------------------------------------------------------------------------------------
///** @class dialogDummyMotor
//*   @brief Config dialog functionality of DummyMotor
//*   
//*   This class is used for the modal configuration dialog. It is used for parameter setup and calibration.
//*	ui_dialogDummyMotor.h is generated by the Gui-Editor. 
//*
//*\sa DummyMotor
//*/
//class DialogAerotechEnsemble : public QDialog 
//{
//    Q_OBJECT
//
//    private:
//        Ui::DialogAerotechEnsemble ui;	//! Handle to the dialog
//
//        inline int coilCurrentIndex( double percentage )
//        {
//            if(percentage >= 100.0) return 7;
//            if(percentage >= 87.5) return 6;
//            if(percentage >= 75.0) return 5;
//            if(percentage >= 62.5) return 4;
//            if(percentage >= 50.0) return 3;
//            if(percentage >= 37.5) return 2;
//            if(percentage >= 25.0) return 1;
//            return 0;
//        }
//
//    public:
//        DialogAerotechEnsemble(int uniqueID);
//		~DialogAerotechEnsemble() {};
//        int setVals(QMap<QString, ito::Param> *paramVals); //!< Function called by AerotechEnsemble::showConfDialog to set parameters values at dialog startup
//        int getVals(QMap<QString, ito::Param> *paramVals);	//!< Function called by DummyMotor::showConfDialog to get back the changed parameters
//
//        int getRunMode();
//        void getAxisValues( int axis, int &enabled, int &microSteps, double &vMin, double &vMax, double &aMax, double &coilThreshold, double &coilHigh, double &coilLow, double &coilRest);
//
//};
//
//#endif