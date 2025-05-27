/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  Author(s): Peter Kazanzides, Dimitri Lezcano, Anton Deguet

  (C) Copyright 2024-2025 Johns Hopkins University (JHU), All Rights Reserved.

  This component provides an interface to a Galil DMC controller, using the DR
  (DataRecord) approach, where the Galil controller periodically sends a data
  record. The format of the data record varies based on Galil DMC model type,
  which can be specified (as "galil_model") in the JSON configuration file.
  Valid values of "galil_model" (which is an unsigned integer) are:

      4000   for DMC 4000, 4200, 4103, and 50000 (default)
     52000   for DMC 52000
      1806   for DMC 1806
      2103   for DMC 2103, 2102
      1802   for DMC 1802
     30000   for DMC 30010

  Note that Galil also specifies these model numbers using "x" for the field that is
  typically used to specify the number of axes, i.e.:
     40x0, 42x0, 41x3, 500x0, 52xx0, 18x6, 21x3, 21x2, 18x2, 30x1x
  (52xx0 supports up to 32 axes, and 30x1x uses the second x to specify type of drive)

  It is recommended to not specify "galil_model" in the JSON file (in which case it
  defaults to 0) and instead allow the system to auto-detect.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#ifndef _mtsGalilController_h
#define _mtsGalilController_h

#include <string>
#include <cstdint>

#include <cisstCommon/cmnPath.h>
#include <cisstVector/vctDynamicVectorTypes.h>
#include <cisstMultiTask/mtsTaskContinuous.h>
#include <cisstMultiTask/mtsInterfaceProvided.h>
#include <cisstParameterTypes/prmConfigurationJoint.h>
#include <cisstParameterTypes/prmStateJoint.h>
#include <cisstParameterTypes/prmPositionJointSet.h>
#include <cisstParameterTypes/prmVelocityJointSet.h>
#include <cisstParameterTypes/prmPositionCartesianGet.h>
#include <cisstParameterTypes/prmOperatingState.h>
#include <cisstParameterTypes/prmActuatorState.h>

#include <sawGalilController/sawGalilControllerConfig.h>

// Always include last
#include <sawGalilController/sawGalilControllerExport.h>

class CISST_EXPORT mtsGalilController : public mtsTaskContinuous
{
    CMN_DECLARE_SERVICES(CMN_DYNAMIC_CREATION_ONEARG, CMN_LOG_LOD_RUN_ERROR)

 public:

    mtsGalilController(const std::string &name);
    mtsGalilController(const std::string &name, unsigned int sizeStateTable, bool newThread = true);
    mtsGalilController(const mtsTaskContinuousConstructorArg &arg);

    ~mtsGalilController();

    enum { GALIL_MAX_AXES = 8 };

    // cisstMultiTask functions
    void Configure(const std::string &fileName) override;
    void Startup(void) override;
    void Run(void) override;
    void Cleanup(void) override;

protected:

    void         *mGalil;                   // Gcon
    sawGalilControllerConfig::controller m_configuration;

    // Path to configuration files
    cmnPath mConfigPath;

    // Data for the Galil controller (not specific to robot or analog input)
    unsigned int  mModel;                   // Galil model
    uint32_t      mHeader;                  // Header bytes in DR packet
    uint16_t      mSampleNum;               // Sample number from controller
    uint8_t       mErrorCode;               // Error code from controller
    uint32_t      mAmpStatus;               // Amplifier status
    bool          mLimitSwitchActiveLow;    // Limit switches are active low (true) or active high (false)
    bool          mHomeSwitchInverted;      // Home switch reading is inverted

    // Structure for robot data
    struct RobotData {
        sawGalilControllerConfig::robots config;  // Axis config
        std::string   name;                     // Robot name (from config file)
        unsigned int  mNumAxes;                 // Number of axes
        unsigned int  mGalilIndexMax;           // Maximum galil index
        prmConfigurationJoint m_config_j;       // Joint configuration
        prmStateJoint m_measured_js;            // Measured joint state (CRTK)
        prmStateJoint m_setpoint_js;            // Setpoint joint state (CRTK)
        prmOperatingState m_op_state;           // Operating state (CRTK)
        prmOperatingState::StateType newState;
        prmActuatorState mActuatorState;        // Actuator state
        vctUIntVec    mAxisToGalilIndexMap;     // Map from axis number to Galil index
        vctUIntVec    mGalilIndexToAxisMap;     // Map from Galil index to axis number

        vctDoubleVec  mEncoderCountsPerUnit;    // Encoder conversion factors
        vctLongVec    mEncoderOffset;           // Encoder offset (counts or bits)
        vctBoolVec    mEncoderAbsolute;         // True if absolute encoder (false if incremental)
        vctDoubleVec  mHomePos;                 // Encoder home positions (offsets)

        vctIntVec     mHomeLimitDisable;        // Limit switch disable during homing
        vctIntVec     mLimitDisable;            // Current setting of limit disable (LD)
        vctBoolVec    mHomingMask;              // Mask for use in homing routines
        bool          mHomeCustom;              // True if custom home needed
        vctIntVec     mHomingSpeed;             // Homing speed for slower searches
        vctUShortVec  mAxisStatus;              // Axis status
        vctUCharVec   mStopCode;                // Axis stop code (see Galil SC command)
        vctBoolVec    mStopCodeChange;          // Whether axis stop code just changed (not currently used)
        vctUCharVec   mSwitches;                // Axis switches (see Galil TS command)
        vctUShortVec  mAnalogIn;                // Axis analog input (raw value)
        bool          mMotorPowerOn;            // Whether motor power is on (for all configured motors)
        bool          mMotionActive;            // Whether a motion is active
        vctDoubleVec  mSpeedDefault;            // Default speed
        vctDoubleVec  mSpeed;                   // Current speed
        vctDoubleVec  mAccelDefault;            // Default accel
        vctDoubleVec  mAccel;                   // Current accel
        vctDoubleVec  mDecelDefault;            // Default decel
        vctDoubleVec  mDecel;                   // Current decel
        vctUIntVec    mState;                   // Internal axis state machine
        unsigned int  mTimeout;                 // Timeout
        mtsInterfaceProvided *mInterface;       // Provided interface
        mtsFunctionWrite operating_state;       // Event generator
        bool          mHadError;                // Indicates that an error occurred last iteration

        // String of configured axes (e.g., "ABC")
        char mGalilAxes[GALIL_MAX_AXES+1];
        // String for querying (e.g., "?,?,?")
        char mGalilQuery[2*GALIL_MAX_AXES];
        // Boolean array indicating which Galil indexes are valid
        bool mGalilIndexValid[GALIL_MAX_AXES];

        bool PTmode;                            // Shadow variable for position tracking (PT)

        mtsGalilController *mParent;            // Pointer to parent object
        char *mBuffer;                          // Local buffer for building command strings

        RobotData();
        ~RobotData();

        void GetNumAxes(unsigned int &numAxes) const { numAxes = mNumAxes; }

        // Move joint to specified position
        //  servo_jp:  uses Position Tracking mode (PT)
        //  move_jp:  uses Independent Axis Positioning mode (PA, BG)
        void servo_jp(const prmPositionJointSet &jtpos);
        void move_jp(const prmPositionJointSet &jtpos);
        // Move joint to specified relative position
        void servo_jr(const prmPositionJointSet &jtpos);
        // Move joint at specified velocity
        void servo_jv(const prmVelocityJointSet &jtvel);
        // Hold joint at current position (Stop)
        void hold(void);

        // Get joint configuration
        void GetConfig_js(prmConfigurationJoint &cfg_j) const
        { cfg_j = m_config_j; }

        // Set operating state
        void state_command(const std::string &command);

        // TEMP: following is to be able to use prmStateRobotQtWidgetComponent
        void measured_cp(prmPositionCartesianGet &pos) const
        { pos = prmPositionCartesianGet(); }

        // Enable motor power
        void EnableMotorPower(void);
        // Disable motor power
        void DisableMotorPower(void);

        // Set speed, acceleration and deceleration
        void SetSpeed(const vctDoubleVec &spd);
        void SetAccel(const vctDoubleVec &accel);
        void SetDecel(const vctDoubleVec &decel);

        // Home: mask indicates which axes to home
        void Home(const vctBoolVec &mask);
        void UnHome(const vctBoolVec &mask);

        // FindEdge: move specified axes until transition on home input
        void FindEdge(const vctBoolVec &mask);
        // FindIndex: move specified axes until index pulse detected, will set
        // axis position to 0 when done
        void FindIndex(const vctBoolVec &mask);

        // Set absolute position (e.g., for homing); also sets home flag
        // (using ZA) on Galil controller
        void SetHomePosition(const vctDoubleVec &pos);

        // Common methods for sending command to Galil
        bool galil_cmd_common(const char *cmdName, const char *cmdGalil, const vctDoubleVec &goal,
                              bool useOffset);
        bool galil_cmd_common(const char *cmdName, const char *cmdGalil, const vctIntVec &data);

        // Local method to check if robot is in ENABLED state
        bool CheckStateEnabled(const char *cmdName) const;

        // Local method to stop robot if it is moving; also ensures that PTmode is false
        void stop_if_active(const char *cmd);

        // Local method to set PT mode. Note that this method checks the value of the shadow
        // variable PTmode and only changes it if it is different than the requested state.
        void SetPT(bool state);

        // Local method to create boolean array from vctBoolVec, also remapping from robot axis to Galil index
        const bool *GetGalilIndexValid(const vctBoolVec &mask) const;
        // Local method to create axes string for specified array of valid Galil indices
        const char *GetGalilAxes(const bool *galilIndexValid) const;
        // Local method to set outMask based on inMask, clearing any entries that correspond to absolute encoders.
        // The method returns false if the inMask is not the correct length (same length as outMask),
        // or if none of the elements in outMask are true.
        bool CheckHomingMask(const char *cmdName, const vctBoolVec &inMask, vctBoolVec &outMask);

        // Local method to set operating state to fault
        void SetFault();
        // Local method to update operating state and set event if it changed
        void UpdateOperatingState();
        // Local method called from component Run loop
        void RunStateMachine();
    };
    std::vector<RobotData> mRobots;

    struct AnalogInputData {
        vctDoubleVec  values;
        vctDoubleVec  bits2volts;              // Conversion from bits to volts (depends on AQ setting)
        vctBoolVec    isSigned;                // True if Galil provides signed value (depends on AQ setting)
        vctUIntVec    AxisToGalilIndexMap;     // Map from axis number to Galil index
        vctUIntVec    GalilIndexToAxisMap;     // Map from Galil index to axis number
        mtsInterfaceProvided *mInterface;      // Provided interface
    };
    std::vector<AnalogInputData> mAnalogInputs;

    char *mBuffer;                          // Local buffer for building command strings

    // Whether controller supports the LD (limit disable) command
    bool HasLimitDisable() const;

    // Whether controller supports the ZA (user data) command
    bool HasUserDataZA() const;

    // Whether controller supports the HV (homing velocity) command
    bool HasHomingVelocity() const;

    // Local static method to write cmd and axes to buffer
    // Parameters:
    //    buf    Buffer for output
    //    cmd    Galil command string, including space if desired (e.g, "BG ")
    //    axes   Galil axes string (e.g., "ABC")
    // Example output: "BG ABC"
    static char *WriteCmdAxes(char *buf, const char *cmd, const char *axes);

    // Local static method to create cmd followed by comma-separated values
    // Parameters:
    //    buf    Buffer for output
    //    cmd    Galil command string, including space if desired (e.g, "SP ")
    //    data   Data values (indexed by Galil index, so valid values may not be contiguous)
    //    valid  Boolean array indicating which data values are valid
    //    num    Size of data and valid arrays
    // Example output: "SP 1000,,500"
    static char *WriteCmdValues(char *buf, const char *cmd, const int32_t *data, const bool *valid, unsigned int num);

    // Local method to write a query command (e.g., "LD ?,?,?") and parse the result
    //    cmd    Galil command string to use for query, include space if desired (e.g., "LD ")
    //    query  Query string (e.g., "?,?,?" or "?,,?")
    //    data   Vector for storing result of query
    bool QueryCmdValues(const char *cmd, const char *query, vctIntVec &data) const;

    // Local methods to parse returned value
    int QueryValueInt(const char *cmd);
    double QueryValueDouble(const char *cmd);

    void Init();
    void Close();

    static unsigned int GetModelIndex(unsigned int modelType);

    void SetupInterfaces();

    void GetHeader(uint32_t &header) const { header = mHeader; }
    void GetConnected(bool &val) const { val = (mGalil != 0); }
    void GetVersion(std::string &ver) const
    { ver = sawGalilController_VERSION; }

    void SendCommand(const std::string& cmdString);
    void SendCommandRet(const std::string& cmdString, std::string &retString);

    // Abort robot command
    void AbortProgram();
    void AbortMotion();
};

CMN_DECLARE_SERVICES_INSTANTIATION(mtsGalilController)

#endif
