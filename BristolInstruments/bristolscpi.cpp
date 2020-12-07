#include "bristolscpi.h"

/*!
* This class implements a Qt version of socket communication for telnet SCPI
* commands to work with the instruments.
*/

/*! \class BristolSCPI
* \brief This is a C++/Qt class used for SCPI communication.
*/

BristolSCPI::BristolSCPI()
{
	/*!
	* Constructor opens a socket connection, waits for a response and skips the opening message.
	* If no connection is available, prints failed to connect message.
	*/
	socket = new QTcpSocket;
	qDebug() << "connecting to host";
	socket->connectToHost("10.199.199.1", 23);
	if (socket->waitForConnected(5000)) {
		qDebug() << "connected!";
	}
	if (socket->state() == QTcpSocket::ConnectedState) {
		qDebug("skipping opening message");
		skipOpeningMessage(500);
	}
	else {
		qDebug("failed to connect");
	}
	
}

void BristolSCPI::skipOpeningMessage(int wait_msec //!< Time to wait for message to appear.
)
{
	QByteArray data;
	int skip_count = 0;
	size_t pos;
	while (true) {
		if (socket->waitForReadyRead(wait_msec)) {
			data = socket->read(200);
		}
		pos = data.toStdString().find("\n\n");
		if (pos != std::string::npos) {
			skip_count += 1;
			break;
		}
	}
}

int BristolSCPI::getSpectrum(char *outfile //!< Filename for the spectrum to be written.
)
{
	QByteArray data;
	QByteArray spectrum;
	QByteArrayList spectrumList;
	qDebug("getting spectrum...");
	socket->write(QByteArray("CALC2:DATA?\r\n"));
	QSaveFile *fs = new QSaveFile(outfile);
	fs->open(QIODevice::WriteOnly);
	while (true) {
		if (socket->waitForReadyRead(5000)) {
			data = socket->read(70001);
			spectrum = spectrum + data;
		}
		else {
			qDebug("done reading spectrum");
			//qDebug() << "spectrum data: " << spectrum;
			qDebug() << "writing spectrum to " << outfile;
			fs->write(spectrum);
			fs->commit();
			delete fs; //closes the file
			return 1;
		}
	}
}

int BristolSCPI::getWLSpectrum(char *outfile,  //!< Filename for wavelength and power to be written.
double wvl_start, //!< Starting wavelength to read.
double wvl_stop //!< Ending wavelength to read.
)
{
	socket->write(QByteArray(":CALC3:WLIM:START " + QByteArray::number(wvl_start, 'f', 6) + "\r\n"));
	socket->write(QByteArray(":CALC3:WLIM:STOP " + QByteArray::number(wvl_stop, 'f', 6) + "\r\n"));
	socket->write(QByteArray(":CALC3:DATA?\r\n"));
	QSaveFile *fs = new QSaveFile(outfile);
	fs->open(QIODevice::WriteOnly);
	const int num_samples_to_read = 100; //optimal size for the buffer
	const int bytes_per_sample = 12;
	const int buffer_size = num_samples_to_read * bytes_per_sample;
	char raw[1];
	double wvl = 0;
	float pwr = 0;
	char raw_spec[buffer_size];
	int num_samples_read = 0;
	int num_bytes_read;
	int total_bytes_read = 0;

	if (socket->waitForReadyRead(10000)) { //takes a while to start returning wavelengths
		socket->getChar(raw);
		qDebug() << raw[0]; //#
		socket->getChar(raw);
		QString num_bytes = raw[0]; //
		qDebug() << "Number of bytes: " << num_bytes;
		QString total_bytes_str;
		int total_bytes;
		for (int i = 0; i < num_bytes.toInt(); i++) {
			socket->getChar(raw);
			total_bytes_str += raw[0];
		}
		total_bytes = total_bytes_str.toInt();
		qDebug() << "total bytes: " << total_bytes;
		int num_samples = total_bytes / bytes_per_sample;
		qDebug() << "number of samples: " << num_samples;
		//spectrum_struct *data = new spectrum_struct[num_samples];
		bool readReady = false;
		QByteArray sample;
		for (int i = 0; i < num_samples/num_samples_to_read + 1; i++) {
			if (socket->waitForReadyRead(500) || readReady) {
				num_bytes_read = socket->read(raw_spec, buffer_size);
				total_bytes_read += num_bytes_read;
				qDebug() << "Number of bytes read: " << num_bytes_read;
				qDebug() << "Total bytes read: " << total_bytes_read;
				for (int k = 0; k < num_bytes_read/bytes_per_sample; k++) {
					memcpy(&wvl, raw_spec + k*bytes_per_sample, sizeof(wvl));
					memcpy(&pwr, raw_spec + k*bytes_per_sample + sizeof(wvl), sizeof(pwr));
					//data[i].pwr = pwr;
					//data[i].wvl = wvl;
					readReady = true;
					num_samples_read++;
					sample = QByteArray::number(pwr, 'f', 4) + "," + QByteArray::number(wvl, 'f', 6) + "\r\n";
					fs->write(sample);
				}
				
			}
			else {
				qDebug("operation timed out");
				break;
			}
		}
		fs->commit();
		delete fs;
	}
	else {
		qDebug("operation timed out");
	}
	//qDebug() << "Total bytes read: " << total_bytes_read;
	qDebug() << "number of samples read " << num_samples_read;
	return 1;
}

QByteArray BristolSCPI::getSimpleMsg(QString msg //<! ASCII encoded SCPI command.
)
/*!
* This function will handle a general SCPI command that returns a single scalar response from the instrument.
*/
{
	qint64 result = socket->write(msg.toUtf8()); //convert to 
	qDebug() << "result " << result << "message " << msg.toUtf8();
	QByteArray data;
	size_t pos;
	while (true) {
		if (socket->waitForReadyRead(5000)) {
			data = socket->read(100);
			pos = data.toStdString().find("\r\n");
			if (pos != std::string::npos) {
				return data.left(pos);
			}
		}
		else {
			qDebug("read timed out");
			break;
		}
	}
	return QByteArray();
}

void BristolSCPI::getStartWL()
{

}

void BristolSCPI::getEndWL()
{

}

void BristolSCPI::startBuffer()
{
	socket->write(QByteArray(":MMEM:INIT\n\r"));
	socket->write(QByteArray(":MMEM:OPEN\n\r"));
	socket->flush();
}

void BristolSCPI::readBuffer(char* outfile, //!< Filename for the buffer data to be written.
	int acq_time //!< Acquisition time used for sample rate calculation.
)
{
	socket->write(QByteArray(":MMEM:CLOSE\n\r"));
	socket->write(QByteArray(":MMEM:DATA?\n\r"));
	QSaveFile *fs = new QSaveFile(outfile);
	fs->open(QIODevice::WriteOnly);
	char raw[1];
	bool readReady = false;
	const int num_samples_to_read = 100; //optimal size for the buffer
	const int bytes_per_sample = 20;
	const int buffer_size = num_samples_to_read * bytes_per_sample;
	double wvl = 0;
	float pwr = 0;
	int status;
	int scan_idx;
	char raw_spec[buffer_size];
	int num_samples_read = 0;
	int num_bytes_read;
	int total_bytes_read = 0;
	if (socket->waitForReadyRead(5000)) {
		socket->getChar(raw);
		qDebug() << raw[0];
		socket->getChar(raw);
		QString num_bytes = raw[0];
		qDebug() << "Number of bytes: " << num_bytes;
		QString total_bytes_str;
		int total_bytes;
		for (int i = 0; i < num_bytes.toInt(); i++) {
			socket->getChar(raw);
			total_bytes_str += raw[0];
		}
		total_bytes = total_bytes_str.toInt();
		qDebug() << "total bytes: " << total_bytes;
		int num_samples = total_bytes / bytes_per_sample;
		qDebug() << "number of samples: " << num_samples;
		qDebug() << "sample rate: " << (float)num_samples / (float)acq_time;
		QByteArray sample;
		for (int i = 0; i < num_samples / num_samples_to_read + 1; i++) {
			if (socket->waitForReadyRead(50) || readReady) {
				num_bytes_read = socket->read(raw_spec, buffer_size);
				for (int k = 0; k < num_bytes_read / bytes_per_sample; k++) {
					memcpy(&wvl, raw_spec + k * bytes_per_sample, sizeof(wvl));
					memcpy(&pwr, raw_spec + k * bytes_per_sample + 8, sizeof(pwr));
					memcpy(&status, raw_spec + k * bytes_per_sample + 12, sizeof(status));
					memcpy(&scan_idx, raw_spec + k * bytes_per_sample + 16, sizeof(scan_idx));
					readReady = true;
					num_samples_read++;
					sample = QByteArray::number(pwr, 'f', 4) + ","
						+ QByteArray::number(wvl, 'f', 6) + ","
						+ QByteArray::number(status) + ","
						+ QByteArray::number(scan_idx) + "\r\n";
					fs->write(sample);
					//qDebug() << "sample: " << sample;
					readReady = true;
				}
			}
		}
	}
	fs->commit();
	delete fs;
}

void BristolSCPI::readBuffer(char* outfile //!< Filename for the buffer data to be written.
)
{
	socket->write(QByteArray(":MMEM:DATA?\n\r"));
	QSaveFile *fs = new QSaveFile(outfile);
	fs->open(QIODevice::WriteOnly);
	char raw[1];
	bool readReady = false;
	const int num_samples_to_read = 100; //optimal size for the buffer
	const int bytes_per_sample = 20;
	const int buffer_size = num_samples_to_read * bytes_per_sample;
	double wvl = 0;
	float pwr = 0;
	int status;
	int scan_idx;
	char raw_spec[buffer_size];
	int num_samples_read = 0;
	int num_bytes_read;
	int total_bytes_read = 0;
	if (socket->waitForReadyRead(5000)) {
		socket->getChar(raw);
		qDebug() << raw[0];
		socket->getChar(raw);
		QString num_bytes = raw[0];
		qDebug() << "Number of bytes: " << num_bytes;
		QString total_bytes_str;
		int total_bytes;
		for (int i = 0; i < num_bytes.toInt(); i++) {
			socket->getChar(raw);
			total_bytes_str += raw[0];
		}
		total_bytes = total_bytes_str.toInt();
		qDebug() << "total bytes: " << total_bytes;
		int num_samples = total_bytes / bytes_per_sample;
		qDebug() << "number of samples: " << num_samples;
		QByteArray sample;
		for (int i = 0; i < num_samples / num_samples_to_read + 1; i++) {
			if (socket->waitForReadyRead(50) || readReady) {
				num_bytes_read = socket->read(raw_spec, buffer_size);
				for (int k = 0; k < num_bytes_read / bytes_per_sample; k++) {
					memcpy(&wvl, raw_spec + k * bytes_per_sample, sizeof(wvl));
					memcpy(&pwr, raw_spec + k * bytes_per_sample + 8, sizeof(pwr));
					memcpy(&status, raw_spec + k * bytes_per_sample + 12, sizeof(status));
					memcpy(&scan_idx, raw_spec + k * bytes_per_sample + 16, sizeof(scan_idx));
					readReady = true;
					num_samples_read++;
					sample = QByteArray::number(pwr, 'f', 4) + ","
						+ QByteArray::number(wvl, 'f', 6) + ","
						+ QByteArray::number(status) + ","
						+ QByteArray::number(scan_idx) + "\r\n";
					fs->write(sample);
					//qDebug() << "sample: " << sample;
				}
			}
		}
	}
	fs->commit();
	delete fs;
}

/*!
* Command to read peak wavelength. Works on all instruments.
*/
double BristolSCPI::readWL()
{
	QString msg = ":READ:WAV?\n\r";
	qint64 result = socket->write(msg.toUtf8()); //convert to 
	qDebug() << "result " << result << "message " << msg.toUtf8();
	QByteArray data;
	double wvl;
	size_t pos;
	while (true) {
		if (socket->waitForReadyRead(5000)) {
			data = socket->read(100);
			pos = data.toStdString().find("\r\n");
			if (pos != std::string::npos) {
				wvl = data.left(pos).toDouble();
				return wvl;
			}
		}
		else {
			qDebug("read timed out");
			return 0;
		}
	}
	return 0;
}

/*!
* Destructor disconnects and closes the Telnet connection.
*
*/
BristolSCPI::~BristolSCPI()
{
	socket->disconnectFromHost();
	if (socket->state() == QAbstractSocket::UnconnectedState ||
		socket->waitForDisconnected(1000)) {
		qDebug("Disconnected!");
	}
}
