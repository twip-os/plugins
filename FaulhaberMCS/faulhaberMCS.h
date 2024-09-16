/* ********************************************************************
    Plugin "FaulhaberMCS" for itom software
    URL: http://www.uni-stuttgart.de/ito
    Copyright (C) 2024, Institut für Technische Optik (ITO),
    Universität Stuttgart, Germany

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

#ifndef FAULHABERMCS_H
#define FAULHABERMCS_H

#include "common/addInInterface.h"
#include "dialogFaulhaberMCS.h"
#include "dockWidgetFaulhaberMCS.h"
#include <qsharedpointer.h>

#include <bitset>

//----------------------------------------------------------------------------------------------------------------------------------
class FaulhaberMCSInterface : public ito::AddInInterfaceBase
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ito.AddInInterfaceBase")
    Q_INTERFACES(ito::AddInInterfaceBase)
    PLUGIN_ITOM_API

public:
    FaulhaberMCSInterface();
    ~FaulhaberMCSInterface();
    ito::RetVal getAddInInst(ito::AddInBase** addInInst);
    bool hasDockWidget()
    {
        return true;
    }

private:
    ito::RetVal closeThisInst(ito::AddInBase** addInInst);
};

//----------------------------------------------------------------------------------------------------------------------------------
class FaulhaberMCS : public ito::AddInActuator
{
    Q_OBJECT;

protected:
    //! Destructor
    ~FaulhaberMCS();
    //! Constructor
    FaulhaberMCS();

public:
    friend class FaulhaberMCSInterface;
    const ito::RetVal showConfDialog(void);
    int hasConfDialog(void)
    {
        return 1;
    }; //!< indicates that this plugin has got a configuration dialog

private:
    ito::AddInDataIO* m_pSerialIO;
    static QList<ito::uint8> openedNodes;
    int m_delayAfterSendCommandMS;
    int m_requestTimeOutMS;
    int m_async; //!< variable to set up async and sync positioning --> Synchrone means program do
                 //!< not return until positioning was done.
    int m_numOfAxes;
    int m_waitForDoneTimeout;
    int m_waitForMCSTimeout;
    int m_port;
    ito::uint16 m_statusWord;

    ito::uint8 m_node;
    bool m_nodeAppended;


    const int m_serialBufferSize;
    QSharedPointer<int> m_serialBufferLength;
    QSharedPointer<char> m_serialBuffer;

    ito::uint8 m_S = 0x53;
    ito::uint8 m_E = 0x45;
    ito::uint8 m_GET = 0x01;
    ito::uint8 m_SET = 0x02;

    enum CommunicationSettings : uint32_t
    {
        CAN_NMT_MANDATORY = 0x00000001,
        TRANSMIT_ASYNC_PDO_EMCY_VIA_CAN = 0x00000002,
        TRANSMIT_EMCY_VIA_USB = 0x00000100,
        TRANSMIT_ASYNC_MESSAGES_VIA_USB = 0x00000200,
        SUPPRESS_BOOT_MESSAGE_VIA_USB = 0x00008000,
        TRANSMIT_EMCY_VIA_RS232 = 0x00010000,
        TRANSMIT_ASYNC_MESSAGES_VIA_RS232 = 0x00020000,
        IGNORE_CRC = 0x00800000
    };

    struct Register
    {
        ito::uint16 index;
        ito::uint8 subindex;
    };

    const Register serialNumber_register = {0x1018, 0x04};
    const Register deviceName_register = {0x1008, 0x00};
    const Register vendorID_register = {0x1018, 0x01};
    const Register productCode_register = {0x1018, 0x02};
    const Register revisionNumber_register = {0x1018, 0x03};
    const Register firmwareVersion_register = {0x100A, 0x00};
    const Register operationMode_register = {0x6060, 0x00};
    const Register netMode_register = {0x2400, 0x05};
    const Register nodeID_register = {0x2400, 0x03};
    const Register deviceID_register = {0x2400, 0x08};
    const Register CPUTemperature_register = {0x2326, 0x01};
    const Register powerStageTemperature_register = {0x2326, 0x02};
    const Register windingTemperature_register = {0x2326, 0x03};
    const Register positionActualValue_register = {0x6064, 0x00};
    const Register positionTargetValue_register = {0x6062, 0x00};
    const Register torqueActualValue_register = {0x6077, 0x00};
    const Register currentActualValue_register = {0x6078, 0x00};
    const Register targetTorque_register = {0x6071, 0x00};
    const Register statusWord_register = {0x6041, 0x00};


    QString convertHexToString(const Register& reg);

    struct ErrorInfo
    {
        QString shortDescription;
        QString longDescription;
        uint16_t cia402ErrorCode;
    };

    ito::RetVal waitForDone(
        const int timeoutMS = -1,
        const QVector<int> axis = QVector<int>() /*if empty -> all axis*/,
        const int flags = 0 /*for your use*/);

    ito::RetVal updateStatus(); // optional method to obtain the status and position of all
                                // connected axes

    // SeralIO functions
    ito::RetVal sendCommand(const QByteArray& command);
    ito::RetVal sendCommandAndGetResponse(const QByteArray& command, QByteArray& responseList);

    ito::RetVal readResponse(QByteArray& result, const ito::uint8& command);

    ito::uint8 calculateChecksum(const QByteArray& message);
    bool verifyChecksum(QByteArray& message, ito::uint8& receivedCRC);

    // READ REGISTER
    ito::RetVal readRegister(
        const ito::uint16& address, const ito::uint8& subindex, QByteArray& response);

    template <typename T>
    ito::RetVal readRegisterWithParsedResponse(
        const ito::uint16& address, const ito::uint8& subindex, T& answer);
    template <typename T> ito::RetVal parseResponse(QByteArray& response, T& parsedResponse);

    // SET REGISTER
    template <typename T>
    ito::RetVal setRegister(
        const ito::uint16& address,
        const ito::uint8& subindex,
        const ito::uint32& value,
        const ito::uint8& length);

    template <typename T>
    ito::RetVal setRegisterWithParsedResponse(
        const ito::uint16& address,
        const ito::uint8& subindex,
        const ito::uint32& value,
        const ito::uint8& length,
        T& answer);

    // CONTROL WORD
    void setControlWord(const ito::uint16 word);

    void shutDown();
    void disable();
    void quickStop();
    void enableOperation();
    void disableVoltage();

    // STARTUP and STATUS
    ito::RetVal startupSequence();
    ito::RetVal shutDownSequence();
    ito::RetVal updateStatusMCS();
    void updateStatusBits(const ito::uint16& statusWord);

    ito::RetVal setCommunicationSettings(const ito::uint32& settings);
    ito::RetVal getError();
    ito::RetVal interpretEMCYError(const ito::uint16& errorCode);

    // PARAMETER FUNCTIONS
    ito::RetVal getSerialNumber(QString& serialNum);
    ito::RetVal getDeviceName(QString& name);
    ito::RetVal getVendorID(QString& id);
    ito::RetVal getProductCode(QString& code);
    ito::RetVal getRevisionNumber(QString& num);
    ito::RetVal getFirmware(QString& version);

    ito::RetVal getNodeID(ito::uint8& id);
    ito::RetVal setNodeID(const ito::uint8& id);

    ito::RetVal getExplicitDeviceID(ito::uint16& id);
    ito::RetVal setExplicitDeviceID(const ito::uint16& id);

    ito::RetVal getMaxMotorSpeed(ito::uint32& speed);
    ito::RetVal setMaxMotorSpeed(const ito::uint32& speed);

    ito::RetVal getAcceleration(ito::uint32& acceleration);
    ito::RetVal setAcceleration(const ito::uint32& acceleration);

    ito::RetVal getDeceleration(ito::uint32& deceleration);
    ito::RetVal setDeceleration(const ito::uint32& deceleration);

    ito::RetVal getQuickStopDeceleration(ito::uint32& deceleration);
    ito::RetVal setQuickStopDeceleration(const ito::uint32& deceleration);

    ito::RetVal getProfileVelocity(ito::uint32& speed);
    ito::RetVal setProfileVelocity(const ito::uint32& speed);

    ito::RetVal getOperationMode(ito::int8& mode);
    ito::RetVal setOperationMode(const ito::int8& mode);

    ito::RetVal getMaxTorqueLimit(ito::uint16& limit);
    ito::RetVal setMaxTorqueLimit(const ito::uint16 limit);

    ito::RetVal getTargetTorque(ito::int16& torque);
    ito::RetVal setTargetTorque(const ito::int16 torque);

    ito::RetVal getNetMode(ito::uint8& mode);
    ito::RetVal setNetMode(const ito::uint8& mode);

    ito::RetVal getTorque(ito::int16& torque);
    ito::RetVal getCurrent(ito::int16& current);

    // TEMPERATURES
    ito::RetVal getCPUTemperature(ito::int16& temp);
    ito::RetVal getPowerStageTemperature(ito::int16& temp);
    ito::RetVal getWindingTemperature(ito::int16& temp);

    // POSITION
    ito::RetVal getPosMCS(ito::int32& pos);
    ito::RetVal getTargetPosMCS(ito::int32& pos);
    ito::RetVal setPosAbsMCS(const ito::int32& pos);
    ito::RetVal setPosRelMCS(const ito::int32& pos);

    // HOMING
    ito::RetVal setHomingMode(const ito::int8& mode);
    ito::RetVal setHomingOffset(const ito::int32& offset);
    ito::RetVal setHomingSpeed(const ito::uint32& speed);
    ito::RetVal setHomingSeekVelocity(const ito::uint32& seek);
    ito::RetVal setHomingAcceleration(const ito::uint32& acceleration);
    ito::RetVal setHomingLimitCheckDelayTime(const ito::uint16& time);
    ito::RetVal setHomingTorqueLimits(const ito::uint16 limits[]);

    ito::RetVal homingCurrentPosToZero(const int& axis);
    ito::RetVal performHoming(
        const ito::int8& method,
        const ito::int32& offset,
        const ito::uint32& switchSeekVelocity,
        const ito::uint32& homingSpeed,
        const ito::uint32& acceleration,
        ito::uint16& limitCheckDelayTime,
        ito::uint16& negativeLimit,
        ito::uint16& positiveLimit);

public slots:
    ito::RetVal getParam(QSharedPointer<ito::Param> val, ItomSharedSemaphore* waitCond);

    ito::RetVal setParam(QSharedPointer<ito::ParamBase> val, ItomSharedSemaphore* waitCond);

    ito::RetVal init(
        QVector<ito::ParamBase>* paramsMand,
        QVector<ito::ParamBase>* paramsOpt,
        ItomSharedSemaphore* waitCond = nullptr);

    ito::RetVal close(ItomSharedSemaphore* waitCond);

    ito::RetVal calib(const int axis, ItomSharedSemaphore* waitCond = nullptr);
    ito::RetVal calib(const QVector<int> axis, ItomSharedSemaphore* waitCond = nullptr);

    ito::RetVal setOrigin(const int axis, ItomSharedSemaphore* waitCond = nullptr);
    ito::RetVal setOrigin(const QVector<int> axis, ItomSharedSemaphore* waitCond = nullptr);

    ito::RetVal getStatus(QSharedPointer<QVector<int>> status, ItomSharedSemaphore* waitCond);

    ito::RetVal getPos(const int axis, QSharedPointer<double> pos, ItomSharedSemaphore* waitCond);
    ito::RetVal getPos(
        const QVector<int> axis,
        QSharedPointer<QVector<double>> pos,
        ItomSharedSemaphore* waitCond);

    ito::RetVal setPosAbs(
        const int axis, const double pos, ItomSharedSemaphore* waitCond = nullptr);
    ito::RetVal setPosAbs(
        const QVector<int> axis, QVector<double> pos, ItomSharedSemaphore* waitCond = nullptr);

    ito::RetVal setPosRel(
        const int axis, const double pos, ItomSharedSemaphore* waitCond = nullptr);
    ito::RetVal setPosRel(
        const QVector<int> axis, QVector<double> pos, ItomSharedSemaphore* waitCond = nullptr);

    ito::RetVal execFunc(
        const QString funcName,
        QSharedPointer<QVector<ito::ParamBase>> paramsMand,
        QSharedPointer<QVector<ito::ParamBase>> paramsOpt,
        QSharedPointer<QVector<ito::ParamBase>> paramsOut,
        ItomSharedSemaphore* waitCond = nullptr);

private slots:
    void dockWidgetVisibilityChanged(bool visible);
};

#endif // FAULHABERMCS_H
