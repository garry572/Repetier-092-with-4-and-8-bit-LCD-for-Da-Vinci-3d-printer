/*
    This file is part of Repetier-Firmware.

    Repetier-Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Repetier-Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Repetier-Firmware.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "Repetier.h"

extern void playsound(int tone,int duration);
#if USE_ADVANCE
uint8_t Printer::minExtruderSpeed;            ///< Timer delay for start extruder speed
uint8_t Printer::maxExtruderSpeed;            ///< Timer delay for end extruder speed
volatile int Printer::extruderStepsNeeded; ///< This many extruder steps are still needed, <0 = reverse steps needed.
//uint8_t Printer::extruderAccelerateDelay;     ///< delay between 2 speec increases
#endif
uint8_t Printer::unitIsInches = 0; ///< 0 = Units are mm, 1 = units are inches.
//Stepper Movement Variables
float Printer::axisStepsPerMM[E_AXIS_ARRAY] = {XAXIS_STEPS_PER_MM,YAXIS_STEPS_PER_MM,ZAXIS_STEPS_PER_MM,1}; ///< Number of steps per mm needed.
float Printer::invAxisStepsPerMM[E_AXIS_ARRAY]; ///< Inverse of axisStepsPerMM for faster conversion
float Printer::maxFeedrate[E_AXIS_ARRAY] = {MAX_FEEDRATE_X, MAX_FEEDRATE_Y, MAX_FEEDRATE_Z}; ///< Maximum allowed feedrate.
float Printer::homingFeedrate[Z_AXIS_ARRAY] = {HOMING_FEEDRATE_X, HOMING_FEEDRATE_Y, HOMING_FEEDRATE_Z};
#if RAMP_ACCELERATION
//  float max_start_speed_units_per_second[E_AXIS_ARRAY] = MAX_START_SPEED_UNITS_PER_SECOND; ///< Speed we can use, without acceleration.
float Printer::maxAccelerationMMPerSquareSecond[E_AXIS_ARRAY] = {MAX_ACCELERATION_UNITS_PER_SQ_SECOND_X,MAX_ACCELERATION_UNITS_PER_SQ_SECOND_Y,MAX_ACCELERATION_UNITS_PER_SQ_SECOND_Z}; ///< X, Y, Z and E max acceleration in mm/s^2 for printing moves or retracts
float Printer::maxTravelAccelerationMMPerSquareSecond[E_AXIS_ARRAY] = {MAX_TRAVEL_ACCELERATION_UNITS_PER_SQ_SECOND_X,MAX_TRAVEL_ACCELERATION_UNITS_PER_SQ_SECOND_Y,MAX_TRAVEL_ACCELERATION_UNITS_PER_SQ_SECOND_Z}; ///< X, Y, Z max acceleration in mm/s^2 for travel moves
/** Acceleration in steps/s^3 in printing mode.*/
unsigned long Printer::maxPrintAccelerationStepsPerSquareSecond[E_AXIS_ARRAY];
/** Acceleration in steps/s^2 in movement mode.*/
unsigned long Printer::maxTravelAccelerationStepsPerSquareSecond[E_AXIS_ARRAY];
#endif
#if NONLINEAR_SYSTEM
long Printer::currentDeltaPositionSteps[E_TOWER_ARRAY];
uint8_t lastMoveID = 0; // Last move ID
#endif
signed char Printer::zBabystepsMissing = 0;
uint8_t Printer::relativeCoordinateMode = false;  ///< Determines absolute (false) or relative Coordinates (true).
uint8_t Printer::relativeExtruderCoordinateMode = false;  ///< Determines Absolute or Relative E Codes while in Absolute Coordinates mode. E is always relative in Relative Coordinates mode.

long Printer::currentPositionSteps[E_AXIS_ARRAY];
float Printer::currentPosition[Z_AXIS_ARRAY];
float Printer::lastCmdPos[Z_AXIS_ARRAY];
long Printer::destinationSteps[E_AXIS_ARRAY];
float Printer::coordinateOffset[Z_AXIS_ARRAY] = {0,0,0};
uint8_t Printer::flag0 = 0;
uint8_t Printer::flag1 = 0;
uint8_t Printer::flaghome = 0;
bool Printer::btop_Cover_open=false;
uint8_t Printer::debugLevel = 6; ///< Bitfield defining debug output. 1 = echo, 2 = info, 4 = error, 8 = dry run., 16 = Only communication, 32 = No moves
uint8_t Printer::stepsPerTimerCall = 1;
uint8_t Printer::menuMode = 0;
float Printer::extrudeMultiplyError = 0;

#if FEATURE_AUTOLEVEL
float Printer::autolevelTransformation[9]; ///< Transformation matrix
#endif
uint32_t Printer::interval;           ///< Last step duration in ticks.
uint32_t Printer::timer;              ///< used for acceleration/deceleration timing
uint32_t Printer::stepNumber;         ///< Step number in current move.
#if USE_ADVANCE
#if ENABLE_QUADRATIC_ADVANCE
int32_t Printer::advanceExecuted;             ///< Executed advance steps
#endif
int Printer::advanceStepsSet;
#endif
#if NONLINEAR_SYSTEM
int32_t Printer::maxDeltaPositionSteps;
floatLong Printer::deltaDiagonalStepsSquaredA;
floatLong Printer::deltaDiagonalStepsSquaredB;
floatLong Printer::deltaDiagonalStepsSquaredC;
float Printer::deltaMaxRadiusSquared;
float Printer::radius0;
int32_t Printer::deltaFloorSafetyMarginSteps = 0;
int32_t Printer::deltaAPosXSteps;
int32_t Printer::deltaAPosYSteps;
int32_t Printer::deltaBPosXSteps;
int32_t Printer::deltaBPosYSteps;
int32_t Printer::deltaCPosXSteps;
int32_t Printer::deltaCPosYSteps;
int32_t Printer::realDeltaPositionSteps[TOWER_ARRAY];
int16_t Printer::travelMovesPerSecond;
int16_t Printer::printMovesPerSecond;
#endif
#if FEATURE_Z_PROBE || MAX_HARDWARE_ENDSTOP_Z || NONLINEAR_SYSTEM
int32_t Printer::stepsRemainingAtZHit;
#endif
#if DRIVE_SYSTEM==DELTA
int32_t Printer::stepsRemainingAtXHit;
int32_t Printer::stepsRemainingAtYHit;
#endif
#if SOFTWARE_LEVELING
int32_t Printer::levelingP1[3];
int32_t Printer::levelingP2[3];
int32_t Printer::levelingP3[3];
#endif
float Printer::minimumSpeed;               ///< lowest allowed speed to keep integration error small
float Printer::minimumZSpeed;
int32_t Printer::xMaxSteps;                   ///< For software endstops, limit of move in positive direction.
int32_t Printer::yMaxSteps;                   ///< For software endstops, limit of move in positive direction.
int32_t Printer::zMaxSteps;                   ///< For software endstops, limit of move in positive direction.
int32_t Printer::xMinSteps;                   ///< For software endstops, limit of move in negative direction.
int32_t Printer::yMinSteps;                   ///< For software endstops, limit of move in negative direction.
int32_t Printer::zMinSteps;                   ///< For software endstops, limit of move in negative direction.
float Printer::xLength;
float Printer::xMin;
float Printer::yLength;
float Printer::yMin;
float Printer::zLength;
float Printer::zMin;
float Printer::feedrate;                   ///< Last requested feedrate.
int Printer::feedrateMultiply;             ///< Multiplier for feedrate in percent (factor 1 = 100)
unsigned int Printer::extrudeMultiply;     ///< Flow multiplier in percdent (factor 1 = 100)
float Printer::maxJerk;                    ///< Maximum allowed jerk in mm/s
#if DRIVE_SYSTEM!=DELTA
float Printer::maxZJerk;                   ///< Maximum allowed jerk in z direction in mm/s
#endif
float Printer::offsetX;                     ///< X-offset for different extruder positions.
float Printer::offsetY;                     ///< Y-offset for different extruder positions.
speed_t Printer::vMaxReached;         ///< Maximumu reached speed
uint32_t Printer::msecondsPrinting;            ///< Milliseconds of printing time (means time with heated extruder)
float Printer::filamentPrinted;            ///< mm of filament printed since counting started
uint8_t Printer::wasLastHalfstepping;         ///< Indicates if last move had halfstepping enabled
#if ENABLE_BACKLASH_COMPENSATION
float Printer::backlashX;
float Printer::backlashY;
float Printer::backlashZ;
uint8_t Printer::backlashDir;
#endif
#ifdef DEBUG_STEPCOUNT
int32_t Printer::totalStepsRemaining;
#endif
float Printer::memoryX;
float Printer::memoryY;
float Printer::memoryZ;
float Printer::memoryE;
float Printer::memoryF = -1;
#if GANTRY
int8_t Printer::motorX;
int8_t Printer::motorYorZ;
#endif
#ifdef DEBUG_SEGMENT_LENGTH
    float Printer::maxRealSegmentLength = 0;
#endif
#ifdef DEBUG_REAL_JERK
    float Printer::maxRealJerk = 0;
#endif
#ifdef DEBUG_PRINT
int debugWaitLoop = 0;
#endif

#if ENABLE_CLEAN_NOZZLE 
void Printer::cleanNozzle(bool restoreposition)
	{
	//we save current configuration and position
	uint8_t tmp_extruderid=Extruder::current->id;
	float tmp_x = currentPosition[X_AXIS];
	float tmp_y = currentPosition[Y_AXIS];
	float tmp_z = currentPosition[Z_AXIS];
	
	//ensure homing is done and select E0
	if(!Printer::isHomed()) Printer::homeAxis(true,true,true);
    else 
        {//put proper position in case position has been manualy changed no need to home Z as cannot be manualy changed and in case of something on plate it could be catastrophic
            Printer::homeAxis(true,true,false);
        }
    // move Z to zMin + 15 if under this position to be sure nozzle do not touch metal holder
    if (currentPosition[Z_AXIS] < zMin+15) moveToReal(IGNORE_COORDINATE,IGNORE_COORDINATE,zMin+15,IGNORE_COORDINATE,homingFeedrate[0]);
    Commands::waitUntilEndOfAllMoves();
	UI_STATUS_UPD_RAM(UI_TEXT_CLEANING_NOZZLE);
        #if DAVINCI ==1
	//first step noze
	moveToReal(xMin+CLEAN_X-ENDSTOP_X_BACK_ON_HOME,yMin,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//second step noze
	moveToReal(xMin-ENDSTOP_X_BACK_ON_HOME,yMin,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//third step noze
	moveToReal(xMin+CLEAN_X-ENDSTOP_X_BACK_ON_HOME,yMin,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//fourth step noze
	moveToReal(xMin-ENDSTOP_X_BACK_ON_HOME,yMin,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//fifth step noze
	moveToReal(xMin+CLEAN_X-ENDSTOP_X_BACK_ON_HOME,yMin,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//sixth step noze
	moveToReal(xMin-ENDSTOP_X_BACK_ON_HOME,yMin,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//first step Z probe
	moveToReal(xMin,yMin + CLEAN_Y-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//second step Z probe
	moveToReal(xMin,yMin-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//third step Z probe
	moveToReal(xMin,yMin+CLEAN_Y-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//fourth step Z probe
	moveToReal(xMin,yMin-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
        //fifth step Z probe
	moveToReal(xMin,yMin+CLEAN_Y-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//sixth step Z probe
	moveToReal(xMin,yMin-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	#else
	//first step
	moveToReal(xMin + CLEAN_X-ENDSTOP_X_BACK_ON_HOME,yMin + CLEAN_Y-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//second step
	moveToReal(xMin-ENDSTOP_X_BACK_ON_HOME,yMin-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//third step
	moveToReal(xMin+CLEAN_X-ENDSTOP_X_BACK_ON_HOME,yMin+CLEAN_Y-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//fourth step
	moveToReal(xMin-ENDSTOP_X_BACK_ON_HOME,yMin-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
	//move out to be sure first drop go to purge box
        moveToReal(xLength-2,yMin+CLEAN_Y-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
        moveToReal(xLength-2,yMin-ENDSTOP_Y_BACK_ON_HOME,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
        Commands::waitUntilEndOfAllMoves();
        //first step
        moveToReal(xLength-20,IGNORE_COORDINATE,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
        //second step
        moveToReal(xLength,IGNORE_COORDINATE,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
        //third step
        moveToReal(xLength-20,IGNORE_COORDINATE,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
         //fourth step
        moveToReal(xLength,IGNORE_COORDINATE,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
        
        Commands::waitUntilEndOfAllMoves();
	//back to original position and original extruder
        //X,Y first then Z
	if (restoreposition)
		{
		moveToReal(tmp_x,tmp_y,IGNORE_COORDINATE,IGNORE_COORDINATE,homingFeedrate[0]);
		moveToReal(IGNORE_COORDINATE,IGNORE_COORDINATE,tmp_z,IGNORE_COORDINATE,homingFeedrate[0]);
		Commands::waitUntilEndOfAllMoves();
		Extruder::selectExtruderById(tmp_extruderid);
		}
        #endif
	}
#endif


#if !NONLINEAR_SYSTEM
void Printer::constrainDestinationCoords()
{
    if(isNoDestinationCheck()) return;
#if min_software_endstop_x
    if (destinationSteps[X_AXIS] < xMinSteps) Printer::destinationSteps[X_AXIS] = Printer::xMinSteps;
#endif
#if min_software_endstop_y
    if (destinationSteps[Y_AXIS] < yMinSteps) Printer::destinationSteps[Y_AXIS] = Printer::yMinSteps;
#endif
#if min_software_endstop_z
    if (destinationSteps[Z_AXIS] < zMinSteps && !isZProbingActive()) Printer::destinationSteps[Z_AXIS] = Printer::zMinSteps;
#endif

#if max_software_endstop_x
    if (destinationSteps[X_AXIS] > Printer::xMaxSteps) Printer::destinationSteps[X_AXIS] = Printer::xMaxSteps;
#endif
#if max_software_endstop_y
    if (destinationSteps[Y_AXIS] > Printer::yMaxSteps) Printer::destinationSteps[Y_AXIS] = Printer::yMaxSteps;
#endif
#if max_software_endstop_z
    if (destinationSteps[Z_AXIS] > Printer::zMaxSteps && !isZProbingActive()) Printer::destinationSteps[Z_AXIS] = Printer::zMaxSteps;
#endif
}
#endif

bool Printer::isPositionAllowed(float x,float y,float z) {
    if(isNoDestinationCheck())  return true;
    bool allowed = true;
#if DRIVE_SYSTEM == DELTA
    allowed &= (z >= 0) && (z <= zLength + 0.05 + ENDSTOP_Z_BACK_ON_HOME);
    allowed &= (x * x + y * y <= deltaMaxRadiusSquared);
#endif // DRIVE_SYSTEM
    if(!allowed) {
        Printer::updateCurrentPosition(true);
        Commands::printCurrentPosition(PSTR("isPositionAllowed "));
    }
    return allowed;
}
void Printer::updateDerivedParameter()
{
#if DRIVE_SYSTEM == DELTA
    travelMovesPerSecond = EEPROM::deltaSegmentsPerSecondMove();
    printMovesPerSecond = EEPROM::deltaSegmentsPerSecondPrint();
    axisStepsPerMM[X_AXIS] = axisStepsPerMM[Y_AXIS] = axisStepsPerMM[Z_AXIS];
    maxAccelerationMMPerSquareSecond[X_AXIS] = maxAccelerationMMPerSquareSecond[Y_AXIS] = maxAccelerationMMPerSquareSecond[Z_AXIS];
    homingFeedrate[X_AXIS] = homingFeedrate[Y_AXIS] = homingFeedrate[Z_AXIS];
    maxFeedrate[X_AXIS] = maxFeedrate[Y_AXIS] = maxFeedrate[Z_AXIS];
    maxTravelAccelerationMMPerSquareSecond[X_AXIS] = maxTravelAccelerationMMPerSquareSecond[Y_AXIS] = maxTravelAccelerationMMPerSquareSecond[Z_AXIS];
    zMaxSteps = axisStepsPerMM[Z_AXIS] * (zLength);
    towerAMinSteps = axisStepsPerMM[A_TOWER] * xMin;
    towerBMinSteps = axisStepsPerMM[B_TOWER] * yMin;
    towerCMinSteps = axisStepsPerMM[C_TOWER] * zMin;
    //radius0 = EEPROM::deltaHorizontalRadius();
    float radiusA = radius0 + EEPROM::deltaRadiusCorrectionA();
    float radiusB = radius0 + EEPROM::deltaRadiusCorrectionB();
    float radiusC = radius0 + EEPROM::deltaRadiusCorrectionC();
    deltaAPosXSteps = floor(radiusA * cos(EEPROM::deltaAlphaA() * M_PI/180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaAPosYSteps = floor(radiusA * sin(EEPROM::deltaAlphaA() * M_PI/180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaBPosXSteps = floor(radiusB * cos(EEPROM::deltaAlphaB() * M_PI/180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaBPosYSteps = floor(radiusB * sin(EEPROM::deltaAlphaB() * M_PI/180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaCPosXSteps = floor(radiusC * cos(EEPROM::deltaAlphaC() * M_PI/180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaCPosYSteps = floor(radiusC * sin(EEPROM::deltaAlphaC() * M_PI/180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaDiagonalStepsSquaredA.l = static_cast<uint32_t>((EEPROM::deltaDiagonalCorrectionA() + EEPROM::deltaDiagonalRodLength())*axisStepsPerMM[Z_AXIS]);
    deltaDiagonalStepsSquaredB.l = static_cast<uint32_t>((EEPROM::deltaDiagonalCorrectionB() + EEPROM::deltaDiagonalRodLength())*axisStepsPerMM[Z_AXIS]);
    deltaDiagonalStepsSquaredC.l = static_cast<uint32_t>((EEPROM::deltaDiagonalCorrectionC() + EEPROM::deltaDiagonalRodLength())*axisStepsPerMM[Z_AXIS]);
    if(deltaDiagonalStepsSquaredA.l > 65534 || 2 * radius0*axisStepsPerMM[Z_AXIS] > 65534)
    {
        setLargeMachine(true);
#ifdef SUPPORT_64_BIT_MATH
        deltaDiagonalStepsSquaredA.L = RMath::sqr(static_cast<uint64_t>(deltaDiagonalStepsSquaredA.l));
        deltaDiagonalStepsSquaredB.L = RMath::sqr(static_cast<uint64_t>(deltaDiagonalStepsSquaredB.l));
        deltaDiagonalStepsSquaredC.L = RMath::sqr(static_cast<uint64_t>(deltaDiagonalStepsSquaredC.l));
#else
        deltaDiagonalStepsSquaredA.f = RMath::sqr(static_cast<float>(deltaDiagonalStepsSquaredA.l));
        deltaDiagonalStepsSquaredB.f = RMath::sqr(static_cast<float>(deltaDiagonalStepsSquaredB.l));
        deltaDiagonalStepsSquaredC.f = RMath::sqr(static_cast<float>(deltaDiagonalStepsSquaredC.l));
#endif
    }
    else {
        setLargeMachine(false);
        deltaDiagonalStepsSquaredA.l = RMath::sqr(deltaDiagonalStepsSquaredA.l);
        deltaDiagonalStepsSquaredB.l = RMath::sqr(deltaDiagonalStepsSquaredB.l);
        deltaDiagonalStepsSquaredC.l = RMath::sqr(deltaDiagonalStepsSquaredC.l);
    }
    deltaMaxRadiusSquared = RMath::sqr(EEPROM::deltaMaxRadius());
    long cart[Z_AXIS_ARRAY], delta[TOWER_ARRAY];
    cart[X_AXIS] = cart[Y_AXIS] = 0;
    cart[Z_AXIS] = zMaxSteps;
    transformCartesianStepsToDeltaSteps(cart, delta);
    maxDeltaPositionSteps = delta[0];
    xMaxSteps = yMaxSteps = zMaxSteps;
    xMinSteps = yMinSteps = zMinSteps = 0;
    deltaFloorSafetyMarginSteps = DELTA_FLOOR_SAFETY_MARGIN_MM * axisStepsPerMM[Z_AXIS];
#elif DRIVE_SYSTEM == TUGA
    deltaDiagonalStepsSquared.l = uint32_t(EEPROM::deltaDiagonalRodLength()*axisStepsPerMM[X_AXIS]);
    if(deltaDiagonalStepsSquared.l>65534)
    {
        setLargeMachine(true);
        deltaDiagonalStepsSquared.f = float(deltaDiagonalStepsSquared.l)*float(deltaDiagonalStepsSquared.l);
    }
    else
        deltaDiagonalStepsSquared.l = deltaDiagonalStepsSquared.l*deltaDiagonalStepsSquared.l;
    deltaBPosXSteps = long(EEPROM::deltaDiagonalRodLength()*axisStepsPerMM[X_AXIS]);
    xMaxSteps = (long)(axisStepsPerMM[X_AXIS]*(xMin+xLength));
    yMaxSteps = (long)(axisStepsPerMM[Y_AXIS]*yLength);
    zMaxSteps = (long)(axisStepsPerMM[Z_AXIS]*(zMin+zLength));
    xMinSteps = (long)(axisStepsPerMM[X_AXIS]*xMin);
    yMinSteps = 0;
    zMinSteps = (long)(axisStepsPerMM[Z_AXIS]*zMin);
#else
    xMaxSteps = (long)(axisStepsPerMM[X_AXIS]*(xMin+xLength));
    yMaxSteps = (long)(axisStepsPerMM[Y_AXIS]*(yMin+yLength));
    zMaxSteps = (long)(axisStepsPerMM[Z_AXIS]*(zMin+zLength));
    xMinSteps = (long)(axisStepsPerMM[X_AXIS]*xMin);
    yMinSteps = (long)(axisStepsPerMM[Y_AXIS]*yMin);
    zMinSteps = (long)(axisStepsPerMM[Z_AXIS]*zMin);
    // For which directions do we need backlash compensation
#if ENABLE_BACKLASH_COMPENSATION
    backlashDir &= XYZ_DIRPOS;
    if(backlashX!=0) backlashDir |= 8;
    if(backlashY!=0) backlashDir |= 16;
    if(backlashZ!=0) backlashDir |= 32;
#endif
#endif
    for(uint8_t i=0; i<E_AXIS_ARRAY; i++)
    {
        invAxisStepsPerMM[i] = 1.0f/axisStepsPerMM[i];
#ifdef RAMP_ACCELERATION
        /** Acceleration in steps/s^3 in printing mode.*/
        maxPrintAccelerationStepsPerSquareSecond[i] = maxAccelerationMMPerSquareSecond[i] * axisStepsPerMM[i];
        /** Acceleration in steps/s^2 in movement mode.*/
        maxTravelAccelerationStepsPerSquareSecond[i] = maxTravelAccelerationMMPerSquareSecond[i] * axisStepsPerMM[i];
#endif
    }
    float accel = RMath::max(maxAccelerationMMPerSquareSecond[X_AXIS],maxTravelAccelerationMMPerSquareSecond[X_AXIS]);
    minimumSpeed = accel*sqrt(2.0f/(axisStepsPerMM[X_AXIS]*accel));
    accel = RMath::max(maxAccelerationMMPerSquareSecond[Z_AXIS],maxTravelAccelerationMMPerSquareSecond[Z_AXIS]);
    minimumZSpeed = accel*sqrt(2.0f/(axisStepsPerMM[Z_AXIS]*accel));
    Printer::updateAdvanceFlags();
}
/**
  \brief Stop heater and stepper motors. Disable power,if possible.
*/
void Printer::kill(uint8_t only_steppers)
{
    if(areAllSteppersDisabled() && only_steppers) return;
    if(Printer::isAllKilled()) return;
    setAllSteppersDiabled();
    disableXStepper();
    disableYStepper();
    disableZStepper();
    Extruder::disableAllExtruderMotors();
#if FAN_BOARD_PIN>-1
    pwm_pos[NUM_EXTRUDER+1] = 0;
#endif // FAN_BOARD_PIN
    if(!only_steppers)
    {
        for(uint8_t i=0; i<NUM_TEMPERATURE_LOOPS; i++)
            Extruder::setTemperatureForExtruder(0,i);
        Extruder::setHeatedBedTemperature(0);
        UI_STATUS_UPD(UI_TEXT_KILLED);
#if defined(PS_ON_PIN) && PS_ON_PIN>-1
        //pinMode(PS_ON_PIN,INPUT);
        SET_OUTPUT(PS_ON_PIN); //GND
        WRITE(PS_ON_PIN, (POWER_INVERTING ? LOW : HIGH));
        Printer::setPowerOn(false);
#endif
        Printer::setAllKilled(true);
    }
    else UI_STATUS_UPD(UI_TEXT_STEPPER_DISABLED);
}

void Printer::updateAdvanceFlags()
{
    Printer::setAdvanceActivated(false);
#if USE_ADVANCE
    for(uint8_t i=0; i<NUM_EXTRUDER; i++)
    {
        if(extruder[i].advanceL!=0)
        {
            Printer::setAdvanceActivated(true);
        }
#if ENABLE_QUADRATIC_ADVANCE
        if(extruder[i].advanceK!=0) Printer::setAdvanceActivated(true);
#endif
    }
#endif
}

// This is for untransformed move to coordinates in printers absolute cartesian space
uint8_t Printer::moveTo(float x,float y,float z,float e,float f)
{
    if(x != IGNORE_COORDINATE)
        destinationSteps[X_AXIS] = (x + Printer::offsetX) * axisStepsPerMM[X_AXIS];
    if(y != IGNORE_COORDINATE)
        destinationSteps[Y_AXIS] = (y + Printer::offsetY) * axisStepsPerMM[Y_AXIS];
    if(z != IGNORE_COORDINATE)
        destinationSteps[Z_AXIS] = z * axisStepsPerMM[Z_AXIS];
    if(e != IGNORE_COORDINATE)
        destinationSteps[E_AXIS] = e * axisStepsPerMM[E_AXIS];
    if(f != IGNORE_COORDINATE)
        feedrate = f;
#if NONLINEAR_SYSTEM
    // Disable software endstop or we get wrong distances when length < real length
    if (!PrintLine::queueDeltaMove(ALWAYS_CHECK_ENDSTOPS, true, false)) {
      Com::printWarningFLN(PSTR("moveTo / queueDeltaMove returns error"));
      return 0;
    }
#else
    PrintLine::queueCartesianMove(ALWAYS_CHECK_ENDSTOPS, true);
#endif
    updateCurrentPosition(false);
    return 1;
}

// Move to transformed cartesian coordinates, mapping real (model) space to printer space
uint8_t Printer::moveToReal(float x, float y, float z, float e, float f)
{
    if(x == IGNORE_COORDINATE)
        x = currentPosition[X_AXIS];
    else
        currentPosition[X_AXIS] = x;
    if(y == IGNORE_COORDINATE)
        y = currentPosition[Y_AXIS];
    else
        currentPosition[Y_AXIS] = y;
    if(z == IGNORE_COORDINATE)
        z = currentPosition[Z_AXIS];
    else
        currentPosition[Z_AXIS] = z;
#if FEATURE_AUTOLEVEL
    if(isAutolevelActive())
        transformToPrinter(x + Printer::offsetX, y + Printer::offsetY, z, x, y, z);
    else
#endif // FEATURE_AUTOLEVEL
    {
        x += Printer::offsetX;
        y += Printer::offsetY;
    }
    // There was conflicting use of IGNOR_COORDINATE
    destinationSteps[X_AXIS] = static_cast<int32_t>(floor(x * axisStepsPerMM[X_AXIS] + 0.5f));
    destinationSteps[Y_AXIS] = static_cast<int32_t>(floor(y * axisStepsPerMM[Y_AXIS] + 0.5f));
    destinationSteps[Z_AXIS] = static_cast<int32_t>(floor(z * axisStepsPerMM[Z_AXIS] + 0.5f));
    if(e != IGNORE_COORDINATE && !Printer::debugDryrun()
#if MIN_EXTRUDER_TEMP > 30
                && (Extruder::current->tempControl.currentTemperatureC > MIN_EXTRUDER_TEMP || Printer::isColdExtrusionAllowed())
#endif
       )
        destinationSteps[E_AXIS] = e * axisStepsPerMM[E_AXIS];
    if(f != IGNORE_COORDINATE)
        feedrate = f;

#if NONLINEAR_SYSTEM
    if (!PrintLine::queueDeltaMove(ALWAYS_CHECK_ENDSTOPS, true, true)) {
        Com::printWarningFLN(PSTR("moveToReal / queueDeltaMove returns error"));
        SHOWM(x); SHOWM(y); SHOWM(z);
        return 0;
    }
#else
    PrintLine::queueCartesianMove(ALWAYS_CHECK_ENDSTOPS, true);
#endif
    return 1;
}

void Printer::setOrigin(float xOff, float yOff, float zOff)
{
    coordinateOffset[X_AXIS] = xOff;
    coordinateOffset[Y_AXIS] = yOff;
    coordinateOffset[Z_AXIS] = zOff;
}

void Printer::updateCurrentPosition(bool copyLastCmd)
{
    currentPosition[X_AXIS] = static_cast<float>(currentPositionSteps[X_AXIS]) * invAxisStepsPerMM[X_AXIS];
    currentPosition[Y_AXIS] = static_cast<float>(currentPositionSteps[Y_AXIS]) * invAxisStepsPerMM[Y_AXIS];
    currentPosition[Z_AXIS] = static_cast<float>(currentPositionSteps[Z_AXIS]) * invAxisStepsPerMM[Z_AXIS];
#if FEATURE_AUTOLEVEL
    if(isAutolevelActive())
        transformFromPrinter(currentPosition[X_AXIS] - Printer::offsetX, currentPosition[Y_AXIS] - Printer::offsetY, currentPosition[Z_AXIS],
                             currentPosition[X_AXIS], currentPosition[Y_AXIS], currentPosition[Z_AXIS]);
    else
#endif // FEATURE_AUTOLEVEL
    {
        currentPosition[X_AXIS] -= Printer::offsetX;
        currentPosition[Y_AXIS] -= Printer::offsetY;
    }
    if(copyLastCmd) {
        lastCmdPos[X_AXIS] = currentPosition[X_AXIS];
        lastCmdPos[Y_AXIS] = currentPosition[Y_AXIS];
        lastCmdPos[Z_AXIS] = currentPosition[Z_AXIS];
    }
}

/**
  \brief Sets the destination coordinates to values stored in com.

  For the computation of the destination, the following facts are considered:
  - Are units inches or mm.
  - Reltive or absolute positioning with special case only extruder relative.
  - Offset in x and y direction for multiple extruder support.
*/

uint8_t Printer::setDestinationStepsFromGCode(GCode *com)
{
    register int32_t p;
    float x, y, z;
    if(!relativeCoordinateMode)
    {
        if(com->hasX()) lastCmdPos[X_AXIS] = currentPosition[X_AXIS] = convertToMM(com->X) - coordinateOffset[X_AXIS];
        if(com->hasY()) lastCmdPos[Y_AXIS] = currentPosition[Y_AXIS] = convertToMM(com->Y) - coordinateOffset[Y_AXIS];
        if(com->hasZ()) lastCmdPos[Z_AXIS] = currentPosition[Z_AXIS] = convertToMM(com->Z) - coordinateOffset[Z_AXIS];
    }
    else
    {
        if(com->hasX()) currentPosition[X_AXIS] = (lastCmdPos[X_AXIS] += convertToMM(com->X));
        if(com->hasY()) currentPosition[Y_AXIS] = (lastCmdPos[Y_AXIS] += convertToMM(com->Y));
        if(com->hasZ()) currentPosition[Z_AXIS] = (lastCmdPos[Z_AXIS] += convertToMM(com->Z));
    }
#if FEATURE_AUTOLEVEL
    if(isAutolevelActive())
    {
        transformToPrinter(lastCmdPos[X_AXIS] + Printer::offsetX, lastCmdPos[Y_AXIS] + Printer::offsetY, lastCmdPos[Z_AXIS], x, y, z);
    }
    else
#endif // FEATURE_AUTOLEVEL
    {
        x = lastCmdPos[X_AXIS] + Printer::offsetX;
        y = lastCmdPos[Y_AXIS] + Printer::offsetY;
        z = lastCmdPos[Z_AXIS];
    }
    destinationSteps[X_AXIS] = static_cast<int32_t>(floor(x * axisStepsPerMM[X_AXIS] + 0.5f));
    destinationSteps[Y_AXIS] = static_cast<int32_t>(floor(y * axisStepsPerMM[Y_AXIS] + 0.5f));
    destinationSteps[Z_AXIS] = static_cast<int32_t>(floor(z * axisStepsPerMM[Z_AXIS] + 0.5f));
    if(com->hasE() && !Printer::debugDryrun())
    {
        p = convertToMM(com->E * axisStepsPerMM[E_AXIS]);
        if(relativeCoordinateMode || relativeExtruderCoordinateMode)
        {
            if(
#if MIN_EXTRUDER_TEMP > 30
                (Extruder::current->tempControl.currentTemperatureC < MIN_EXTRUDER_TEMP && !Printer::isColdExtrusionAllowed()) ||
#endif
                fabs(com->E) > EXTRUDE_MAXLENGTH)
                p = 0;
            destinationSteps[E_AXIS] = currentPositionSteps[E_AXIS] + p;
        }
        else
        {
            if(
#if MIN_EXTRUDER_TEMP > 30
                Extruder::current->tempControl.currentTemperatureC<MIN_EXTRUDER_TEMP ||
#endif
                fabs(p - currentPositionSteps[E_AXIS]) > EXTRUDE_MAXLENGTH * axisStepsPerMM[E_AXIS])
                currentPositionSteps[E_AXIS] = p;
            destinationSteps[E_AXIS] = p;
        }
    }
    else Printer::destinationSteps[E_AXIS] = Printer::currentPositionSteps[E_AXIS];
    if(com->hasF())
    {
        if(unitIsInches)
            feedrate = com->F * 0.0042333f * (float)feedrateMultiply;  // Factor is 25.5/60/100
        else
            feedrate = com->F * (float)feedrateMultiply * 0.00016666666f;
    }
    if(!Printer::isPositionAllowed(lastCmdPos[X_AXIS],lastCmdPos[Y_AXIS], lastCmdPos[Z_AXIS])) {
        currentPositionSteps[E_AXIS] = destinationSteps[E_AXIS];
        return false; // ignore move
    }
    return !com->hasNoXYZ() || (com->hasE() && destinationSteps[E_AXIS] != currentPositionSteps[E_AXIS]); // ignore unproductive moves
}

void Printer::setup()
{
#if FEATURE_WATCHDOG
    HAL::startWatchdog();
#endif // FEATURE_WATCHDOG
#if FEATURE_CONTROLLER == CONTROLLER_VIKI
    HAL::delayMilliseconds(100);
#endif // FEATURE_CONTROLLER
    //HAL::delayMilliseconds(500);  // add a delay at startup to give hardware time for initalization
    HAL::hwSetup();
#ifdef ANALYZER
// Channel->pin assignments
#if ANALYZER_CH0>=0
    SET_OUTPUT(ANALYZER_CH0);
#endif
#if ANALYZER_CH1>=0
    SET_OUTPUT(ANALYZER_CH1);
#endif
#if ANALYZER_CH2>=0
    SET_OUTPUT(ANALYZER_CH2);
#endif
#if ANALYZER_CH3>=0
    SET_OUTPUT(ANALYZER_CH3);
#endif
#if ANALYZER_CH4>=0
    SET_OUTPUT(ANALYZER_CH4);
#endif
#if ANALYZER_CH5>=0
    SET_OUTPUT(ANALYZER_CH5);
#endif
#if ANALYZER_CH6>=0
    SET_OUTPUT(ANALYZER_CH6);
#endif
#if ANALYZER_CH7>=0
    SET_OUTPUT(ANALYZER_CH7);
#endif
#endif

#if defined(ENABLE_POWER_ON_STARTUP) && ENABLE_POWER_ON_STARTUP && (PS_ON_PIN>-1)
    SET_OUTPUT(PS_ON_PIN); //GND
    WRITE(PS_ON_PIN, (POWER_INVERTING ? HIGH : LOW));
    Printer::setPowerOn(true);
#else
#if PS_ON_PIN>-1
    Printer::setPowerOn(false);
#else
    Printer::setPowerOn(true);
#endif
#endif
#if SDSUPPORT
    //power to SD reader
#if SDPOWER > -1
    SET_OUTPUT(SDPOWER);
    WRITE(SDPOWER,HIGH);
#endif
#if defined(SDCARDDETECT) && SDCARDDETECT>-1
    SET_INPUT(SDCARDDETECT);
    PULLUP(SDCARDDETECT,HIGH);
#endif
#endif

    //Initialize Step Pins
    SET_OUTPUT(X_STEP_PIN);
    SET_OUTPUT(Y_STEP_PIN);
    SET_OUTPUT(Z_STEP_PIN);

    //Initialize Dir Pins
#if X_DIR_PIN>-1
    SET_OUTPUT(X_DIR_PIN);
#endif
#if Y_DIR_PIN>-1
    SET_OUTPUT(Y_DIR_PIN);
#endif
#if Z_DIR_PIN>-1
    SET_OUTPUT(Z_DIR_PIN);
#endif

    //Steppers default to disabled.
#if X_ENABLE_PIN > -1
    SET_OUTPUT(X_ENABLE_PIN);
    WRITE(X_ENABLE_PIN,!X_ENABLE_ON);
#endif
#if Y_ENABLE_PIN > -1
    SET_OUTPUT(Y_ENABLE_PIN);
    WRITE(Y_ENABLE_PIN,!Y_ENABLE_ON);
#endif
#if Z_ENABLE_PIN > -1
    SET_OUTPUT(Z_ENABLE_PIN);
    WRITE(Z_ENABLE_PIN,!Z_ENABLE_ON);
#endif
#if FEATURE_TWO_XSTEPPER
    SET_OUTPUT(X2_STEP_PIN);
    SET_OUTPUT(X2_DIR_PIN);
#if X2_ENABLE_PIN > -1
    SET_OUTPUT(X2_ENABLE_PIN);
    WRITE(X2_ENABLE_PIN,!X_ENABLE_ON);
#endif
#endif

#if FEATURE_TWO_YSTEPPER
    SET_OUTPUT(Y2_STEP_PIN);
    SET_OUTPUT(Y2_DIR_PIN);
#if Y2_ENABLE_PIN > -1
    SET_OUTPUT(Y2_ENABLE_PIN);
    WRITE(Y2_ENABLE_PIN,!Y_ENABLE_ON);
#endif
#endif

#if FEATURE_TWO_ZSTEPPER
    SET_OUTPUT(Z2_STEP_PIN);
    SET_OUTPUT(Z2_DIR_PIN);
#if X2_ENABLE_PIN > -1
    SET_OUTPUT(Z2_ENABLE_PIN);
    WRITE(Z2_ENABLE_PIN,!Z_ENABLE_ON);
#endif
#endif
#if defined(FIL_SENSOR1_PIN)
SET_INPUT(FIL_SENSOR1_PIN);
#endif
#if defined(FIL_SENSOR2_PIN)
SET_INPUT(FIL_SENSOR2_PIN);
#endif
    //endstop pullups
#if MIN_HARDWARE_ENDSTOP_X
#if X_MIN_PIN>-1
    SET_INPUT(X_MIN_PIN);
#if ENDSTOP_PULLUP_X_MIN
    PULLUP(X_MIN_PIN,HIGH);
#endif
#else
#error You have defined hardware x min endstop without pin assignment. Set pin number for X_MIN_PIN
#endif
#endif
#if MIN_HARDWARE_ENDSTOP_Y
#if Y_MIN_PIN>-1
    SET_INPUT(Y_MIN_PIN);
#if ENDSTOP_PULLUP_Y_MIN
    PULLUP(Y_MIN_PIN,HIGH);
#endif
#else
#error You have defined hardware y min endstop without pin assignment. Set pin number for Y_MIN_PIN
#endif
#endif
#if MIN_HARDWARE_ENDSTOP_Z
#if Z_MIN_PIN>-1
    SET_INPUT(Z_MIN_PIN);
#if ENDSTOP_PULLUP_Z_MIN
    PULLUP(Z_MIN_PIN,HIGH);
#endif
#else
#error You have defined hardware z min endstop without pin assignment. Set pin number for Z_MIN_PIN
#endif
#endif
#if MAX_HARDWARE_ENDSTOP_X
#if X_MAX_PIN>-1
    SET_INPUT(X_MAX_PIN);
#if ENDSTOP_PULLUP_X_MAX
    PULLUP(X_MAX_PIN,HIGH);
#endif
#else
#error You have defined hardware x max endstop without pin assignment. Set pin number for X_MAX_PIN
#endif
#endif
#if MAX_HARDWARE_ENDSTOP_Y
#if Y_MAX_PIN>-1
    SET_INPUT(Y_MAX_PIN);
#if ENDSTOP_PULLUP_Y_MAX
    PULLUP(Y_MAX_PIN,HIGH);
#endif
#else
#error You have defined hardware y max endstop without pin assignment. Set pin number for Y_MAX_PIN
#endif
#endif
#if MAX_HARDWARE_ENDSTOP_Z
#if Z_MAX_PIN>-1
    SET_INPUT(Z_MAX_PIN);
#if ENDSTOP_PULLUP_Z_MAX
    PULLUP(Z_MAX_PIN,HIGH);
#endif
#else
#error You have defined hardware z max endstop without pin assignment. Set pin number for Z_MAX_PIN
#endif
#endif
#if FEATURE_Z_PROBE && Z_PROBE_PIN>-1
    SET_INPUT(Z_PROBE_PIN);
#if Z_PROBE_PULLUP
    PULLUP(Z_PROBE_PIN,HIGH);
#endif
#endif // FEATURE_FEATURE_Z_PROBE
#if FAN_PIN>-1 && FEATURE_FAN_CONTROL
    SET_OUTPUT(FAN_PIN);
    WRITE(FAN_PIN,LOW);
#endif
#if FAN_BOARD_PIN>-1
    SET_OUTPUT(FAN_BOARD_PIN);
    WRITE(FAN_BOARD_PIN,LOW);
#endif
#if defined(EXT0_HEATER_PIN) && EXT0_HEATER_PIN>-1
    SET_OUTPUT(EXT0_HEATER_PIN);
    WRITE(EXT0_HEATER_PIN,HEATER_PINS_INVERTED);
#endif
#if defined(EXT1_HEATER_PIN) && EXT1_HEATER_PIN>-1 && NUM_EXTRUDER>1
    SET_OUTPUT(EXT1_HEATER_PIN);
    WRITE(EXT1_HEATER_PIN,HEATER_PINS_INVERTED);
#endif
#if defined(EXT2_HEATER_PIN) && EXT2_HEATER_PIN>-1 && NUM_EXTRUDER>2
    SET_OUTPUT(EXT2_HEATER_PIN);
    WRITE(EXT2_HEATER_PIN,HEATER_PINS_INVERTED);
#endif
#if defined(EXT3_HEATER_PIN) && EXT3_HEATER_PIN>-1 && NUM_EXTRUDER>3
    SET_OUTPUT(EXT3_HEATER_PIN);
    WRITE(EXT3_HEATER_PIN,HEATER_PINS_INVERTED);
#endif
#if defined(EXT4_HEATER_PIN) && EXT4_HEATER_PIN>-1 && NUM_EXTRUDER>4
    SET_OUTPUT(EXT4_HEATER_PIN);
    WRITE(EXT4_HEATER_PIN,HEATER_PINS_INVERTED);
#endif
#if defined(EXT5_HEATER_PIN) && EXT5_HEATER_PIN>-1 && NUM_EXTRUDER>5
    SET_OUTPUT(EXT5_HEATER_PIN);
    WRITE(EXT5_HEATER_PIN,HEATER_PINS_INVERTED);
#endif
#if defined(EXT0_EXTRUDER_COOLER_PIN) && EXT0_EXTRUDER_COOLER_PIN>-1
    SET_OUTPUT(EXT0_EXTRUDER_COOLER_PIN);
    WRITE(EXT0_EXTRUDER_COOLER_PIN,LOW);
#endif
#if defined(EXT1_EXTRUDER_COOLER_PIN) && EXT1_EXTRUDER_COOLER_PIN>-1 && NUM_EXTRUDER>1
    SET_OUTPUT(EXT1_EXTRUDER_COOLER_PIN);
    WRITE(EXT1_EXTRUDER_COOLER_PIN,LOW);
#endif
#if defined(EXT2_EXTRUDER_COOLER_PIN) && EXT2_EXTRUDER_COOLER_PIN>-1 && NUM_EXTRUDER>2
    SET_OUTPUT(EXT2_EXTRUDER_COOLER_PIN);
    WRITE(EXT2_EXTRUDER_COOLER_PIN,LOW);
#endif
#if defined(EXT3_EXTRUDER_COOLER_PIN) && EXT3_EXTRUDER_COOLER_PIN>-1 && NUM_EXTRUDER>3
    SET_OUTPUT(EXT3_EXTRUDER_COOLER_PIN);
    WRITE(EXT3_EXTRUDER_COOLER_PIN,LOW);
#endif
#if defined(EXT4_EXTRUDER_COOLER_PIN) && EXT4_EXTRUDER_COOLER_PIN>-1 && NUM_EXTRUDER>4
    SET_OUTPUT(EXT4_EXTRUDER_COOLER_PIN);
    WRITE(EXT4_EXTRUDER_COOLER_PIN,LOW);
#endif
#if defined(EXT5_EXTRUDER_COOLER_PIN) && EXT5_EXTRUDER_COOLER_PIN>-1 && NUM_EXTRUDER>5
    SET_OUTPUT(EXT5_EXTRUDER_COOLER_PIN);
    WRITE(EXT5_EXTRUDER_COOLER_PIN,LOW);
#endif
#if CASE_LIGHTS_PIN>=0
    SET_OUTPUT(CASE_LIGHTS_PIN);
    WRITE(CASE_LIGHTS_PIN, EEPROM::buselight);
#endif // CASE_LIGHTS_PIN
#if GANTRY
    Printer::motorX = 0;
    Printer::motorYorZ = 0;
#endif

#if STEPPER_CURRENT_CONTROL!=CURRENT_CONTROL_MANUAL
    motorCurrentControlInit(); // Set current if it is firmware controlled
#endif
    microstepInit();
#if FEATURE_AUTOLEVEL
    resetTransformationMatrix(true);
#endif // FEATURE_AUTOLEVEL
    feedrate = 50; ///< Current feedrate in mm/s.
    feedrateMultiply = 100;
    extrudeMultiply = 100;
    lastCmdPos[X_AXIS] = lastCmdPos[Y_AXIS] = lastCmdPos[Z_AXIS] = 0;
#if USE_ADVANCE
#if ENABLE_QUADRATIC_ADVANCE
    advanceExecuted = 0;
#endif
    advanceStepsSet = 0;
#endif
    for(uint8_t i=0; i<NUM_EXTRUDER+3; i++) pwm_pos[i]=0;
    maxJerk = MAX_JERK;
#if DRIVE_SYSTEM!=DELTA
    maxZJerk = MAX_ZJERK;
#endif
    offsetX = offsetY = 0;
    interval = 5000;
    stepsPerTimerCall = 1;
    msecondsPrinting = 0;
    filamentPrinted = 0;
    flag0 = PRINTER_FLAG0_STEPPER_DISABLED;
    xLength = X_MAX_LENGTH;
    yLength = Y_MAX_LENGTH;
    zLength = Z_MAX_LENGTH;
    xMin = X_MIN_POS;
    yMin = Y_MIN_POS;
    zMin = Z_MIN_POS;
#if NONLINEAR_SYSTEM
    radius0 = ROD_RADIUS;
#endif
    wasLastHalfstepping = 0;
#if ENABLE_BACKLASH_COMPENSATION
    backlashX = X_BACKLASH;
    backlashY = Y_BACKLASH;
    backlashZ = Z_BACKLASH;
    backlashDir = 0;
#endif
#if USE_ADVANCE
    extruderStepsNeeded = 0;
#endif
    EEPROM::initBaudrate();
    HAL::serialSetBaudrate(baudrate);
    Com::printFLN(Com::tStart);
    //UI_INITIALIZE;
    HAL::showStartReason();
    Extruder::initExtruder();
#if SDSUPPORT
#ifdef SDEEPROM
    HAL::setupSdEeprom();
#endif
    sd.initsd();
#endif
     HAL::loadVirtualEEPROM();
    // sets autoleveling in eeprom init
    EEPROM::init(); // Read settings from eeprom if wanted
    for(uint8_t i=0; i<E_AXIS_ARRAY; i++) {
      currentPositionSteps[i] = 0;
      currentPosition[i] = 0.0;
    }
//setAutolevelActive(false); // fixme delete me
    //Commands::printCurrentPosition(PSTR("Printer::setup 0 "));
    updateDerivedParameter();
    Commands::checkFreeMemory();
    Commands::writeLowestFreeRAM();
    HAL::setupTimer();
    UI_INITIALIZE;
#if NONLINEAR_SYSTEM
    transformCartesianStepsToDeltaSteps(Printer::currentPositionSteps, Printer::currentDeltaPositionSteps);

#if DELTA_HOME_ON_POWER
    homeAxis(true,true,true);
#endif
    Commands::printCurrentPosition(PSTR("Printer::setup "));
#endif // DRIVE_SYSTEM
    Extruder::selectExtruderById(0);
#if SDSUPPORT
    sd.autoPrint();
#endif
    playsound(880,100);
    playsound(1479,150);
    playsound(1174,100);
    playsound(2349,150);

}

void Printer::defaultLoopActions()
{

    Commands::checkForPeriodicalActions(true);  //check heater every n milliseconds

    UI_MEDIUM; // do check encoder
    millis_t curtime = HAL::timeInMilliseconds();
    if(PrintLine::hasLines() || isMenuMode(MENU_MODE_SD_PAUSED))
        previousMillisCmd = curtime;
    else
    {
        curtime -= previousMillisCmd;
        if(maxInactiveTime != 0 && curtime >  maxInactiveTime )
            Printer::kill(false);
        else
            Printer::setAllKilled(false); // prevent repeated kills
        if(stepperInactiveTime != 0 && curtime >  stepperInactiveTime )
            Printer::kill(true);
    }
#if SDCARDDETECT>-1 && SDSUPPORT
    sd.automount();
#endif
#if defined(SDEEPROM)
    if (!HAL::syncSdEeprom())
    {
        Com::printFLN(PSTR("SD EEPROM write error"));
    }
#endif
    DEBUG_MEMORY;
}

void Printer::MemoryPosition()
{
    Commands::waitUntilEndOfAllMoves();
    updateCurrentPosition(false);
    realPosition(memoryX, memoryY, memoryZ);
    memoryE = currentPositionSteps[E_AXIS] * invAxisStepsPerMM[E_AXIS];
    memoryF = feedrate;
}

void Printer::GoToMemoryPosition(bool x, bool y, bool z, bool e, float feed)
{
    if(memoryF < 0) return; // Not stored before call, so we ignor eit
    bool all = !(x || y || z);
    moveToReal((all || x ? (lastCmdPos[X_AXIS] = memoryX) : IGNORE_COORDINATE)
               ,(all || y ?(lastCmdPos[Y_AXIS] = memoryY) : IGNORE_COORDINATE)
               ,(all || z ? (lastCmdPos[Z_AXIS] = memoryZ) : IGNORE_COORDINATE)
               ,(e ? memoryE : IGNORE_COORDINATE),
               feed);
    feedrate = memoryF;
    updateCurrentPosition(true);
}

#if DRIVE_SYSTEM == DELTA
void Printer::deltaMoveToTopEndstops(float feedrate)
{
    for (uint8_t i=0; i<3; i++)
        Printer::currentPositionSteps[i] = 0;
    transformCartesianStepsToDeltaSteps(currentPositionSteps, currentDeltaPositionSteps);
    PrintLine::moveRelativeDistanceInSteps(0,0,zMaxSteps*1.5,0,feedrate, true, true);
    offsetX = 0;
    offsetY = 0;
}
void Printer::homeXAxis()
{
	setHomedX(true);
    destinationSteps[X_AXIS] = 0;
    if (!PrintLine::queueDeltaMove(true,false,false)) {
      Com::printWarningFLN(PSTR("homeXAxis / queueDeltaMove returns error"));
    }
}
void Printer::homeYAxis()
{
	setHomedY(true);
    Printer::destinationSteps[Y_AXIS] = 0;
    if (!PrintLine::queueDeltaMove(true,false,false)) {
      Com::printWarningFLN(PSTR("homeYAxis / queueDeltaMove returns error"));
    }
}
void Printer::homeZAxis() // Delta z homing
{
    setHomedZ(true);
    SHOT("homeZAxis ");
    deltaMoveToTopEndstops(Printer::homingFeedrate[Z_AXIS]);
    PrintLine::moveRelativeDistanceInSteps(0,0,2*axisStepsPerMM[Z_AXIS]*-ENDSTOP_Z_BACK_MOVE,0,Printer::homingFeedrate[Z_AXIS]/ENDSTOP_X_RETEST_REDUCTION_FACTOR, true, false);
    deltaMoveToTopEndstops(Printer::homingFeedrate[Z_AXIS]/ENDSTOP_Z_RETEST_REDUCTION_FACTOR);
#if defined(ENDSTOP_Z_BACK_ON_HOME)
    if(ENDSTOP_Z_BACK_ON_HOME > 0)
        PrintLine::moveRelativeDistanceInSteps(0,0,axisStepsPerMM[Z_AXIS]*-ENDSTOP_Z_BACK_ON_HOME * Z_HOME_DIR,0,homingFeedrate[Z_AXIS],true,false);
#endif
    // Correct different endstop heights
    // These can be adjusted by two methods. You can use offsets stored by determining the center
    // or you can use the xyzMinSteps from G100 calibration. Both have the same effect but only one
    // should be measuredas both have the same effect.
    long dx = -xMinSteps-EEPROM::deltaTowerXOffsetSteps();
    long dy = -yMinSteps-EEPROM::deltaTowerYOffsetSteps();
    long dz = -zMinSteps-EEPROM::deltaTowerZOffsetSteps();
    long dm = RMath::min(dx,dy,dz);
    //Com::printFLN(Com::tTower1,dx);
    //Com::printFLN(Com::tTower2,dy);
    //Com::printFLN(Com::tTower3,dz);
    dx -= dm; // now all dxyz are positive
    dy -= dm;
    dz -= dm;
    currentPositionSteps[X_AXIS] = 0;
    currentPositionSteps[Y_AXIS] = 0;
    currentPositionSteps[Z_AXIS] = zMaxSteps;
    transformCartesianStepsToDeltaSteps(currentPositionSteps,currentDeltaPositionSteps);
    currentDeltaPositionSteps[A_TOWER] -= dx;
    currentDeltaPositionSteps[B_TOWER] -= dy;
    currentDeltaPositionSteps[C_TOWER] -= dz;
    PrintLine::moveRelativeDistanceInSteps(0,0,dm,0,homingFeedrate[Z_AXIS],true,false);
    currentPositionSteps[X_AXIS] = 0;
    currentPositionSteps[Y_AXIS] = 0;
    currentPositionSteps[Z_AXIS] = zMaxSteps; // Extruder is now exactly in the delta center
    coordinateOffset[X_AXIS] = 0;
    coordinateOffset[Y_AXIS] = 0;
    coordinateOffset[Z_AXIS] = 0;
    transformCartesianStepsToDeltaSteps(currentPositionSteps, currentDeltaPositionSteps);
    realDeltaPositionSteps[A_TOWER] = currentDeltaPositionSteps[A_TOWER];
    realDeltaPositionSteps[B_TOWER] = currentDeltaPositionSteps[B_TOWER];
    realDeltaPositionSteps[C_TOWER] = currentDeltaPositionSteps[C_TOWER];
    //maxDeltaPositionSteps = currentDeltaPositionSteps[X_AXIS];
#if defined(ENDSTOP_Z_BACK_ON_HOME)
    if(ENDSTOP_Z_BACK_ON_HOME > 0)
        maxDeltaPositionSteps += axisStepsPerMM[Z_AXIS]*ENDSTOP_Z_BACK_ON_HOME;
#endif
    Extruder::selectExtruderById(Extruder::current->id);
}
// This home axis is for delta
void Printer::homeAxis(bool xaxis,bool yaxis,bool zaxis) // Delta homing code
{
    setHomed(true)
    SHOT("homeAxis ");
    bool autoLevel = isAutolevelActive();
    setAutolevelActive(false);
    long steps;
    setHomed(true);
    bool homeallaxis = (xaxis && yaxis && zaxis) || (!xaxis && !yaxis && !zaxis);
    if (!(X_MAX_PIN > -1 && Y_MAX_PIN > -1 && Z_MAX_PIN > -1
          && MAX_HARDWARE_ENDSTOP_X && MAX_HARDWARE_ENDSTOP_Y && MAX_HARDWARE_ENDSTOP_Z))
    {
        Com::printErrorFLN(PSTR("Hardware setup inconsistent. Delta cannot home wihtout max endstops."));
    }
    // The delta has to have home capability to zero and set position,
    // so the redundant check is only an opportunity to
    // gratuitously fail due to incorrect settings.
    // The following movements would be meaningless unless it was zeroed for example.
    UI_STATUS_UPD(UI_TEXT_HOME_DELTA);
    // Homing Z axis means that you must home X and Y
    if (homeallaxis || zaxis)
    {
        homeZAxis();
    }
    else
    {
        if (xaxis) Printer::destinationSteps[X_AXIS] = 0;
        if (yaxis) Printer::destinationSteps[Y_AXIS] = 0;
        if (!PrintLine::queueDeltaMove(true,false,false)) {
           Com::printWarningFLN(PSTR("homeAxis / queueDeltaMove returns error"));
        }
    }

    moveToReal(0,0,Printer::zLength,IGNORE_COORDINATE,homingFeedrate[Z_AXIS]); // Move to designed coordinates including translation
    updateCurrentPosition(true);
    UI_CLEAR_STATUS
    Commands::printCurrentPosition(PSTR("homeAxis "));
    setAutolevelActive(autoLevel);
}
#else
#if DRIVE_SYSTEM==TUGA  // Tuga printer homing
void Printer::homeXAxis()
{
    long steps;
    setHomedX(true);
    if ((MIN_HARDWARE_ENDSTOP_X && X_MIN_PIN > -1 && X_HOME_DIR==-1 && MIN_HARDWARE_ENDSTOP_Y && Y_MIN_PIN > -1 && Y_HOME_DIR==-1) ||
            (MAX_HARDWARE_ENDSTOP_X && X_MAX_PIN > -1 && X_HOME_DIR==1 && MAX_HARDWARE_ENDSTOP_Y && Y_MAX_PIN > -1 && Y_HOME_DIR==1))
    {
        long offX = 0,offY = 0;
#if NUM_EXTRUDER>1
        for(uint8_t i=0; i<NUM_EXTRUDER; i++)
        {
#if X_HOME_DIR < 0
            offX = RMath::max(offX,extruder[i].xOffset);
            offY = RMath::max(offY,extruder[i].yOffset);
#else
            offX = RMath::min(offX,extruder[i].xOffset);
            offY = RMath::min(offY,extruder[i].yOffset);
#endif
        }
        // Reposition extruder that way, that all extruders can be selected at home pos.
#endif
        UI_STATUS_UPD(UI_TEXT_HOME_X);
        steps = (Printer::xMaxSteps-Printer::xMinSteps) * X_HOME_DIR;
        currentPositionSteps[X_AXIS] = -steps;
        currentPositionSteps[Y_AXIS] = 0;
        transformCartesianStepsToDeltaSteps(currentPositionSteps, currentDeltaPositionSteps);
        PrintLine::moveRelativeDistanceInSteps(2*steps,0,0,0,homingFeedrate[X_AXIS],true,true);
        currentPositionSteps[X_AXIS] = (X_HOME_DIR == -1) ? xMinSteps-offX : xMaxSteps+offX;
        currentPositionSteps[Y_AXIS] = 0; //(Y_HOME_DIR == -1) ? yMinSteps-offY : yMaxSteps+offY;
        //PrintLine::moveRelativeDistanceInSteps(axisStepsPerMM[X_AXIS]*-ENDSTOP_X_BACK_MOVE * X_HOME_DIR,axisStepsPerMM[Y_AXIS]*-ENDSTOP_X_BACK_MOVE * Y_HOME_DIR,0,0,homingFeedrate[X_AXIS]/ENDSTOP_X_RETEST_REDUCTION_FACTOR,true,false);
        // PrintLine::moveRelativeDistanceInSteps(axisStepsPerMM[X_AXIS]*2*ENDSTOP_X_BACK_MOVE * X_HOME_DIR,axisStepsPerMM[Y_AXIS]*2*ENDSTOP_X_BACK_MOVE * Y_HOME_DIR,0,0,homingFeedrate[X_AXIS]/ENDSTOP_X_RETEST_REDUCTION_FACTOR,true,true);
        PrintLine::moveRelativeDistanceInSteps(axisStepsPerMM[X_AXIS]*-ENDSTOP_X_BACK_MOVE * X_HOME_DIR,0,0,0,homingFeedrate[X_AXIS]/ENDSTOP_X_RETEST_REDUCTION_FACTOR,true,false);
        PrintLine::moveRelativeDistanceInSteps(axisStepsPerMM[X_AXIS]*2*ENDSTOP_X_BACK_MOVE * X_HOME_DIR,0,0,0,homingFeedrate[X_AXIS]/ENDSTOP_X_RETEST_REDUCTION_FACTOR,true,true);
#if defined(ENDSTOP_X_BACK_ON_HOME)
        if(ENDSTOP_X_BACK_ON_HOME > 0)
            PrintLine::moveRelativeDistanceInSteps(axisStepsPerMM[X_AXIS]*-ENDSTOP_X_BACK_ON_HOME * X_HOME_DIR,0,0,0,homingFeedrate[X_AXIS],true,false);
        // PrintLine::moveRelativeDistanceInSteps(axisStepsPerMM[X_AXIS]*-ENDSTOP_X_BACK_ON_HOME * X_HOME_DIR,axisStepsPerMM[Y_AXIS]*-ENDSTOP_Y_BACK_ON_HOME * Y_HOME_DIR,0,0,homingFeedrate[X_AXIS],true,false);
#endif
        currentPositionSteps[X_AXIS] = (X_HOME_DIR == -1) ? xMinSteps-offX : xMaxSteps+offX;
        currentPositionSteps[Y_AXIS] = 0; //(Y_HOME_DIR == -1) ? yMinSteps-offY : yMaxSteps+offY;
        coordinateOffset[X_AXIS] = 0;
        coordinateOffset[Y_AXIS] = 0;
        transformCartesianStepsToDeltaSteps(currentPositionSteps, currentDeltaPositionSteps);
#if NUM_EXTRUDER>1
        PrintLine::moveRelativeDistanceInSteps((Extruder::current->xOffset-offX) * X_HOME_DIR,(Extruder::current->yOffset-offY) * Y_HOME_DIR,0,0,homingFeedrate[X_AXIS],true,false);
#endif
    }
}
void Printer::homeYAxis()
{
    // Dummy function x and y homing must occur together
    setHomedY(true);
}
#else // cartesian printer
void Printer::homeXAxis()
{
    long steps;
    setHomedX(true);
	Extruder::selectExtruderById(0,false);
    if ((MIN_HARDWARE_ENDSTOP_X && X_MIN_PIN > -1 && X_HOME_DIR==-1) || (MAX_HARDWARE_ENDSTOP_X && X_MAX_PIN > -1 && X_HOME_DIR==1))
    {
        long offX = 0;
#if NUM_EXTRUDER>1
        for(uint8_t i=0; i<NUM_EXTRUDER; i++)
#if X_HOME_DIR < 0
            offX = RMath::max(offX,extruder[i].xOffset);
#else
            offX = RMath::min(offX,extruder[i].xOffset);
#endif
        // Reposition extruder that way, that all extruders can be selected at home pos.
#endif
        UI_STATUS_UPD(UI_TEXT_HOME_X);
        steps = (Printer::xMaxSteps-Printer::xMinSteps) * X_HOME_DIR;
        currentPositionSteps[X_AXIS] = -steps;
        PrintLine::moveRelativeDistanceInSteps(2*steps,0,0,0,homingFeedrate[X_AXIS],true,true);
        currentPositionSteps[X_AXIS] = (X_HOME_DIR == -1) ? xMinSteps-offX : xMaxSteps + offX;
        PrintLine::moveRelativeDistanceInSteps(axisStepsPerMM[X_AXIS] * -ENDSTOP_X_BACK_MOVE * X_HOME_DIR,0,0,0,homingFeedrate[X_AXIS] / ENDSTOP_X_RETEST_REDUCTION_FACTOR,true,false);
        PrintLine::moveRelativeDistanceInSteps(axisStepsPerMM[X_AXIS] * 2 * ENDSTOP_X_BACK_MOVE * X_HOME_DIR,0,0,0,homingFeedrate[X_AXIS] / ENDSTOP_X_RETEST_REDUCTION_FACTOR,true,true);
#if defined(ENDSTOP_X_BACK_ON_HOME)
        if(ENDSTOP_X_BACK_ON_HOME > 0)
            PrintLine::moveRelativeDistanceInSteps(axisStepsPerMM[X_AXIS] * -ENDSTOP_X_BACK_ON_HOME * X_HOME_DIR,0,0,0,homingFeedrate[X_AXIS],true,false);
#endif
        currentPositionSteps[X_AXIS] = (X_HOME_DIR == -1) ? xMinSteps-offX : xMaxSteps+offX;
#if NUM_EXTRUDER>1
        PrintLine::moveRelativeDistanceInSteps((Extruder::current->xOffset-offX) * X_HOME_DIR,0,0,0,homingFeedrate[X_AXIS],true,false);
#endif
    }
}
void Printer::homeYAxis()
{
    long steps;
    setHomedY(true);
	Extruder::selectExtruderById(0,false);
    if ((MIN_HARDWARE_ENDSTOP_Y && Y_MIN_PIN > -1 && Y_HOME_DIR==-1) || (MAX_HARDWARE_ENDSTOP_Y && Y_MAX_PIN > -1 && Y_HOME_DIR==1))
    {
        long offY = 0;
#if NUM_EXTRUDER>1
        for(uint8_t i=0; i<NUM_EXTRUDER; i++)
#if Y_HOME_DIR<0
            offY = RMath::max(offY,extruder[i].yOffset);
#else
            offY = RMath::min(offY,extruder[i].yOffset);
#endif
        // Reposition extruder that way, that all extruders can be selected at home pos.
#endif
        UI_STATUS_UPD(UI_TEXT_HOME_Y);
        steps = (yMaxSteps-Printer::yMinSteps) * Y_HOME_DIR;
        currentPositionSteps[Y_AXIS] = -steps;
        PrintLine::moveRelativeDistanceInSteps(0,2*steps,0,0,homingFeedrate[Y_AXIS],true,true);
        currentPositionSteps[Y_AXIS] = (Y_HOME_DIR == -1) ? yMinSteps-offY : yMaxSteps+offY;
        PrintLine::moveRelativeDistanceInSteps(0,axisStepsPerMM[Y_AXIS]*-ENDSTOP_Y_BACK_MOVE * Y_HOME_DIR,0,0,homingFeedrate[Y_AXIS]/ENDSTOP_X_RETEST_REDUCTION_FACTOR,true,false);
        PrintLine::moveRelativeDistanceInSteps(0,axisStepsPerMM[Y_AXIS]*2*ENDSTOP_Y_BACK_MOVE * Y_HOME_DIR,0,0,homingFeedrate[Y_AXIS]/ENDSTOP_X_RETEST_REDUCTION_FACTOR,true,true);
#if defined(ENDSTOP_Y_BACK_ON_HOME)
        if(ENDSTOP_Y_BACK_ON_HOME > 0)
            PrintLine::moveRelativeDistanceInSteps(0,axisStepsPerMM[Y_AXIS]*-ENDSTOP_Y_BACK_ON_HOME * Y_HOME_DIR,0,0,homingFeedrate[Y_AXIS],true,false);
#endif
        currentPositionSteps[Y_AXIS] = (Y_HOME_DIR == -1) ? yMinSteps-offY : yMaxSteps+offY;
#if NUM_EXTRUDER>1
        PrintLine::moveRelativeDistanceInSteps(0,(Extruder::current->yOffset-offY) * Y_HOME_DIR,0,0,homingFeedrate[Y_AXIS],true,false);
#endif
    }
}
#endif

void Printer::homeZAxis() // cartesian homing
{
    long steps;
    setHomedZ(true);
    if ((MIN_HARDWARE_ENDSTOP_Z && Z_MIN_PIN > -1 && Z_HOME_DIR==-1) || (MAX_HARDWARE_ENDSTOP_Z && Z_MAX_PIN > -1 && Z_HOME_DIR==1))
    {
        UI_STATUS_UPD(UI_TEXT_HOME_Z);
        steps = (zMaxSteps - zMinSteps) * Z_HOME_DIR;
        currentPositionSteps[Z_AXIS] = -steps;
        PrintLine::moveRelativeDistanceInSteps(0,0,2*steps,0,homingFeedrate[Z_AXIS],true,true);
        currentPositionSteps[Z_AXIS] = (Z_HOME_DIR == -1) ? zMinSteps : zMaxSteps;
        PrintLine::moveRelativeDistanceInSteps(0,0,axisStepsPerMM[Z_AXIS]*-ENDSTOP_Z_BACK_MOVE * Z_HOME_DIR,0,homingFeedrate[Z_AXIS]/ENDSTOP_Z_RETEST_REDUCTION_FACTOR,true,false);
        PrintLine::moveRelativeDistanceInSteps(0,0,axisStepsPerMM[Z_AXIS]*2*ENDSTOP_Z_BACK_MOVE * Z_HOME_DIR,0,homingFeedrate[Z_AXIS]/ENDSTOP_Z_RETEST_REDUCTION_FACTOR,true,true);
#if defined(ENDSTOP_Z_BACK_ON_HOME)
        if(ENDSTOP_Z_BACK_ON_HOME > 0)
            PrintLine::moveRelativeDistanceInSteps(0,0,axisStepsPerMM[Z_AXIS]*-ENDSTOP_Z_BACK_ON_HOME * Z_HOME_DIR,0,homingFeedrate[Z_AXIS],true,false);
#endif
        currentPositionSteps[Z_AXIS] = (Z_HOME_DIR == -1) ? zMinSteps : zMaxSteps;
#if DRIVE_SYSTEM==TUGA
        currentDeltaPositionSteps[C_TOWER] = currentPositionSteps[Z_AXIS];
#endif
    }
}

void Printer::homeAxis(bool xaxis,bool yaxis,bool zaxis) // home non-delta printer
{
    float startX,startY,startZ;
    realPosition(startX,startY,startZ);
    setHomed(true);
#if !defined(HOMING_ORDER)
#define HOMING_ORDER HOME_ORDER_XYZ
#endif
#if HOMING_ORDER==HOME_ORDER_XYZ
    if(xaxis) homeXAxis();
    if(yaxis) homeYAxis();
    if(zaxis) homeZAxis();
#elif HOMING_ORDER==HOME_ORDER_XZY
    if(xaxis) homeXAxis();
    if(zaxis) homeZAxis();
    if(yaxis) homeYAxis();
#elif HOMING_ORDER==HOME_ORDER_YXZ
    if(yaxis) homeYAxis();
    if(xaxis) homeXAxis();
    if(zaxis) homeZAxis();
#elif HOMING_ORDER==HOME_ORDER_YZX
    if(yaxis) homeYAxis();
    if(zaxis) homeZAxis();
    if(xaxis) homeXAxis();
#elif HOMING_ORDER==HOME_ORDER_ZXY
    if(zaxis) homeZAxis();
    if(xaxis) homeXAxis();
    if(yaxis) homeYAxis();
#elif HOMING_ORDER==HOME_ORDER_ZYX
    if(zaxis) homeZAxis();
    if(yaxis) homeYAxis();
    if(xaxis) homeXAxis();
#endif
    if(xaxis)
    {
        if(X_HOME_DIR < 0) startX = Printer::xMin;
        else startX = Printer::xMin+Printer::xLength;
    }
    if(yaxis)
    {
        if(Y_HOME_DIR < 0) startY = Printer::yMin;
        else startY = Printer::yMin+Printer::yLength;
    }
    if(zaxis)
    {
        if(Z_HOME_DIR < 0) startZ = Printer::zMin;
        else startZ = Printer::zMin+Printer::zLength;
    }
    updateCurrentPosition(true);
    moveToReal(startX, startY, startZ, IGNORE_COORDINATE, homingFeedrate[X_AXIS]);
    UI_CLEAR_STATUS
    Commands::printCurrentPosition(PSTR("homeAxis "));
}
#endif  // Not delta printer

void Printer::zBabystep() {
    bool dir = zBabystepsMissing > 0;
    if(dir) zBabystepsMissing--; else zBabystepsMissing++;
#if DRIVE_SYSTEM == 3
        Printer::enableXStepper();
        Printer::enableYStepper();
#endif
        Printer::enableZStepper();
        Printer::unsetAllSteppersDisabled();
#if DRIVE_SYSTEM == 3
        bool xDir = Printer::getXDirection();
        bool yDir = Printer::getYDirection();
#endif
        bool zDir = Printer::getZDirection();
#if DRIVE_SYSTEM == 3
        Printer::setXDirection(dir);
        Printer::setYDirection(dir);
#endif
        Printer::setZDirection(dir);
#if defined(DIRECTION_DELAY) && DIRECTION_DELAY > 0
        HAL::delayMicroseconds(DIRECTION_DELAY);
#else
        HAL::delayMicroseconds(1);
#endif
#if DRIVE_SYSTEM == 3
        WRITE(X_STEP_PIN,HIGH);
#if FEATURE_TWO_XSTEPPER
        WRITE(X2_STEP_PIN,HIGH);
#endif
        WRITE(Y_STEP_PIN,HIGH);
#if FEATURE_TWO_YSTEPPER
        WRITE(Y2_STEP_PIN,HIGH);
#endif
#endif
        WRITE(Z_STEP_PIN,HIGH);
#if FEATURE_TWO_ZSTEPPER
        WRITE(Z2_STEP_PIN,HIGH);
#endif
        HAL::delayMicroseconds(STEPPER_HIGH_DELAY + 2);
        Printer::endXYZSteps();
#if DRIVE_SYSTEM == 3
        Printer::setXDirection(xDir);
        Printer::setYDirection(yDir);
#endif
        Printer::setZDirection(zDir);
#if defined(DIRECTION_DELAY) && DIRECTION_DELAY > 0
        HAL::delayMicroseconds(DIRECTION_DELAY);
#endif
        //HAL::delayMicroseconds(STEPPER_HIGH_DELAY + 1);
}


void Printer::setAutolevelActive(bool on)
{
#if FEATURE_AUTOLEVEL
    if(on == isAutolevelActive()) return;
    flag0 = (on ? flag0 | PRINTER_FLAG0_AUTOLEVEL_ACTIVE : flag0 & ~PRINTER_FLAG0_AUTOLEVEL_ACTIVE);
    if(on)
        Com::printInfoFLN(Com::tAutolevelEnabled);
    else
        Com::printInfoFLN(Com::tAutolevelDisabled);
    updateCurrentPosition(false);
#endif // FEATURE_AUTOLEVEL    if(isAutolevelActive()==on) return;
}
#if MAX_HARDWARE_ENDSTOP_Z
float Printer::runZMaxProbe()
{
#if NONLINEAR_SYSTEM
    long startZ = realDeltaPositionSteps[Z_AXIS] = currentDeltaPositionSteps[Z_AXIS]; // update real
#endif
    Commands::waitUntilEndOfAllMoves();
    long probeDepth = 2*(Printer::zMaxSteps-Printer::zMinSteps);
    stepsRemainingAtZHit = -1;
    setZProbingActive(true);
    PrintLine::moveRelativeDistanceInSteps(0,0,probeDepth,0,EEPROM::zProbeSpeed(),true,true);
    if(stepsRemainingAtZHit < 0)
    {
        Com::printErrorFLN(Com::tZProbeFailed);
        return -1;
    }
    setZProbingActive(false);
    currentPositionSteps[Z_AXIS] -= stepsRemainingAtZHit;
#if NONLINEAR_SYSTEM
    probeDepth -= (realDeltaPositionSteps[Z_AXIS] - startZ);
#else
    probeDepth -= stepsRemainingAtZHit;
#endif
    float distance = (float)probeDepth * invAxisStepsPerMM[Z_AXIS];
    Com::printF(Com::tZProbeMax,distance);
    Com::printF(Com::tSpaceXColon,realXPosition());
    Com::printFLN(Com::tSpaceYColon,realYPosition());
    PrintLine::moveRelativeDistanceInSteps(0,0,-probeDepth,0,EEPROM::zProbeSpeed(),true,true);
    return distance;
}
#endif

#if FEATURE_Z_PROBE
float Printer::runZProbe(bool first,bool last,uint8_t repeat,bool runStartScript)
{
    float oldOffX =  Printer::offsetX;
    float oldOffY =  Printer::offsetY;
    if(first)
    {
        if(runStartScript)
            GCode::executeFString(Com::tZProbeStartScript);
        Printer::offsetX = -EEPROM::zProbeXOffset();
        Printer::offsetY = -EEPROM::zProbeYOffset();
        PrintLine::moveRelativeDistanceInSteps((Printer::offsetX - oldOffX) * Printer::axisStepsPerMM[X_AXIS],
                                               (Printer::offsetY - oldOffY) * Printer::axisStepsPerMM[Y_AXIS],0,0,EEPROM::zProbeXYSpeed(),true,ALWAYS_CHECK_ENDSTOPS);
    }
    Commands::waitUntilEndOfAllMoves();
    int32_t sum = 0,probeDepth,shortMove = (int32_t)((float)Z_PROBE_SWITCHING_DISTANCE * axisStepsPerMM[Z_AXIS]);
    int32_t lastCorrection = currentPositionSteps[Z_AXIS];
#if NONLINEAR_SYSTEM
    realDeltaPositionSteps[Z_AXIS] = currentDeltaPositionSteps[Z_AXIS]; // update real
#endif
    int32_t updateZ = 0;
    for(uint8_t r = 0; r < repeat; r++)
    {
        probeDepth = 2 * (Printer::zMaxSteps - Printer::zMinSteps);
        stepsRemainingAtZHit = -1;
        int32_t offx = axisStepsPerMM[X_AXIS] * EEPROM::zProbeXOffset();
        int32_t offy = axisStepsPerMM[Y_AXIS] * EEPROM::zProbeYOffset();
        //PrintLine::moveRelativeDistanceInSteps(-offx,-offy,0,0,EEPROM::zProbeXYSpeed(),true,true);
        waitForZProbeStart();
        setZProbingActive(true);
        PrintLine::moveRelativeDistanceInSteps(0,0,-probeDepth,0,EEPROM::zProbeSpeed(),true,true);
        if(stepsRemainingAtZHit < 0)
        {
            Com::printErrorFLN(Com::tZProbeFailed);
            return -1;
        }
        setZProbingActive(false);
#if NONLINEAR_SYSTEM
        stepsRemainingAtZHit = realDeltaPositionSteps[C_TOWER] - currentDeltaPositionSteps[C_TOWER];
#endif
#if DRIVE_SYSTEM == DELTA
        currentDeltaPositionSteps[A_TOWER] += stepsRemainingAtZHit;
        currentDeltaPositionSteps[B_TOWER] += stepsRemainingAtZHit;
        currentDeltaPositionSteps[C_TOWER] += stepsRemainingAtZHit;
#endif
        currentPositionSteps[Z_AXIS] += stepsRemainingAtZHit; // now current position is correct
        if(r == 0 && first) {// Modify start z position on first probe hit to speed the ZProbe process
            int32_t newLastCorrection = currentPositionSteps[Z_AXIS] + (int32_t)((float)EEPROM::zProbeBedDistance() * axisStepsPerMM[Z_AXIS]);
            if(newLastCorrection < lastCorrection) {
                    updateZ = lastCorrection - newLastCorrection;
                lastCorrection = newLastCorrection;
            }
        }
        sum += lastCorrection - currentPositionSteps[Z_AXIS];
        if(r + 1 < repeat)
            PrintLine::moveRelativeDistanceInSteps(0,0,shortMove,0,EEPROM::zProbeSpeed(),true,false);
    }
    float distance = (float)sum * invAxisStepsPerMM[Z_AXIS] / (float)repeat + EEPROM::zProbeHeight();
    Com::printF(Com::tZProbe,distance);
    Com::printF(Com::tSpaceXColon,realXPosition());
    Com::printFLN(Com::tSpaceYColon,realYPosition());
    // Go back to start position
    PrintLine::moveRelativeDistanceInSteps(0,0,lastCorrection - currentPositionSteps[Z_AXIS],0,EEPROM::zProbeSpeed(),true,false);
    //PrintLine::moveRelativeDistanceInSteps(offx,offy,0,0,EEPROM::zProbeXYSpeed(),true,true);

    if(last)
    {
        oldOffX = Printer::offsetX;
        oldOffY = Printer::offsetY;
        GCode::executeFString(Com::tZProbeEndScript);
        if(Extruder::current)
        {
            Printer::offsetX = -Extruder::current->xOffset * Printer::invAxisStepsPerMM[X_AXIS];
            Printer::offsetY = -Extruder::current->yOffset * Printer::invAxisStepsPerMM[Y_AXIS];
        }
        PrintLine::moveRelativeDistanceInSteps((Printer::offsetX - oldOffX)*Printer::axisStepsPerMM[X_AXIS],
                                               (Printer::offsetY - oldOffY)*Printer::axisStepsPerMM[Y_AXIS],0,0,EEPROM::zProbeXYSpeed(),true,ALWAYS_CHECK_ENDSTOPS);
    }
    return distance;
}

void Printer::waitForZProbeStart()
{
#if Z_PROBE_WAIT_BEFORE_TEST
    if(isZProbeHit()) return;
#if UI_DISPLAY_TYPE != NO_DISPLAY
    uid.setStatusP(Com::tHitZProbe);
    uid.refreshPage();
#endif
#ifdef DEBUG_PRINT
    debugWaitLoop = 3;
#endif
    while(!isZProbeHit())
    {
        defaultLoopActions();
    }
#ifdef DEBUG_PRINT
    debugWaitLoop = 4;
#endif
    HAL::delayMilliseconds(30);
    while(isZProbeHit())
    {
        defaultLoopActions();
    }
    HAL::delayMilliseconds(30);
    UI_CLEAR_STATUS;
#endif
}
#endif

#if FEATURE_AUTOLEVEL
void Printer::transformToPrinter(float x,float y,float z,float &transX,float &transY,float &transZ)
{
#if FEATURE_AXISCOMP
    // Axis compensation:
    x = x + y * EEPROM::axisCompTanXY() + z * EEPROM::axisCompTanXZ();
    y = y + z * EEPROM::axisCompTanYZ();
#endif
    transX = x*autolevelTransformation[0]+y*autolevelTransformation[3]+z*autolevelTransformation[6];
    transY = x*autolevelTransformation[1]+y*autolevelTransformation[4]+z*autolevelTransformation[7];
    transZ = x*autolevelTransformation[2]+y*autolevelTransformation[5]+z*autolevelTransformation[8];
}

void Printer::transformFromPrinter(float x,float y,float z,float &transX,float &transY,float &transZ)
{
    transX = x*autolevelTransformation[0]+y*autolevelTransformation[1]+z*autolevelTransformation[2];
    transY = x*autolevelTransformation[3]+y*autolevelTransformation[4]+z*autolevelTransformation[5];
    transZ = x*autolevelTransformation[6]+y*autolevelTransformation[7]+z*autolevelTransformation[8];
#if FEATURE_AXISCOMP
    // Axis compensation:
    transY = transY - transZ * EEPROM::axisCompTanYZ();
    transX = transX - transY * EEPROM::axisCompTanXY() - transZ * EEPROM::axisCompTanXZ();
#endif
}

void Printer::resetTransformationMatrix(bool silent)
{
    autolevelTransformation[0] = autolevelTransformation[4] = autolevelTransformation[8] = 1;
    autolevelTransformation[1] = autolevelTransformation[2] = autolevelTransformation[3] =
                                     autolevelTransformation[5] = autolevelTransformation[6] = autolevelTransformation[7] = 0;
    if(!silent)
        Com::printInfoFLN(Com::tAutolevelReset);
}

void Printer::buildTransformationMatrix(float h1,float h2,float h3)
{
    float ax = EEPROM::zProbeX2()-EEPROM::zProbeX1();
    float ay = EEPROM::zProbeY2()-EEPROM::zProbeY1();
    float az = h1-h2;
    float bx = EEPROM::zProbeX3()-EEPROM::zProbeX1();
    float by = EEPROM::zProbeY3()-EEPROM::zProbeY1();
    float bz = h1-h3;
    // First z direction
    autolevelTransformation[6] = ay*bz-az*by;
    autolevelTransformation[7] = az*bx-ax*bz;
    autolevelTransformation[8] = ax*by-ay*bx;
    float len = sqrt(autolevelTransformation[6]*autolevelTransformation[6]+autolevelTransformation[7]*autolevelTransformation[7]+autolevelTransformation[8]*autolevelTransformation[8]);
    if(autolevelTransformation[8]<0) len = -len;
    autolevelTransformation[6] /= len;
    autolevelTransformation[7] /= len;
    autolevelTransformation[8] /= len;
    autolevelTransformation[3] = 0;
    autolevelTransformation[4] = autolevelTransformation[8];
    autolevelTransformation[5] = -autolevelTransformation[7];
    // cross(y,z)
    autolevelTransformation[0] = autolevelTransformation[4]*autolevelTransformation[8]-autolevelTransformation[5]*autolevelTransformation[7];
    autolevelTransformation[1] = autolevelTransformation[5]*autolevelTransformation[6]-autolevelTransformation[3]*autolevelTransformation[8];
    autolevelTransformation[2] = autolevelTransformation[3]*autolevelTransformation[7]-autolevelTransformation[4]*autolevelTransformation[6];
    len = sqrt(autolevelTransformation[0]*autolevelTransformation[0]+autolevelTransformation[2]*autolevelTransformation[2]);
    autolevelTransformation[0] /= len;
    autolevelTransformation[2] /= len;
    len = sqrt(autolevelTransformation[4]*autolevelTransformation[4]+autolevelTransformation[5]*autolevelTransformation[5]);
    autolevelTransformation[4] /= len;
    autolevelTransformation[5] /= len;
    Com::printArrayFLN(Com::tTransformationMatrix,autolevelTransformation,9,6);
}
#endif

