#pragma once

#include <QObject>
#include <QtCore/QCoreApplication>
#include <QtNetwork>
#include <QtTest/qtest.h>
#include <QTextStream>
#include <QSaveFile>
#include <iostream>

class BristolSCPI
{

public:

	QTcpSocket * socket; //!< This is the telnet communication object.
	
	double readWL(); //!< Example of a common function
	QByteArray getSimpleMsg(QString msg); //!< general function for simple one output parameter
	void skipOpeningMessage(int wait_msec); //!< used to skip the MityDSP telnet opening message
	void getStartWL(); //!< reads the starting wavelength range
	void getEndWL(); //!< reads the ending wavelength range

	void startBuffer(); //!< Initializes and starts the buffer using SCPI commands OPEN and INIT
	void readBuffer(char *outfile, int acq_time); //!< Stops and reads the buffer using SCPI commands CLOSE and DATA?
	void readBuffer(char *outfile); //!< Same readBuffer function except doesn't compute the sample rate

	int getSpectrum(char *outfile); //!< Calls SCPI command CALC2:DATA? to read a spectrum  
	int getWLSpectrum(char *outfile, double wvl_start, double wvl_stop); //!< Calls CALC3:DATA? to read a porting of the wavelength spectrum
	BristolSCPI(); //!< Opens a telnet communication channel.
	~BristolSCPI(); //<! Closes the telent communication channel.
};
