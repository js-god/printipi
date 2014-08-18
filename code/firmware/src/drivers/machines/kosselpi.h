#ifndef DRIVERS_KOSSELPI_H
#define DRIVERS_KOSSELPI_H


#include "common/pid.h"
#include "common/filters/lowpassfilter.h"
//#include "motion/exponentialacceleration.h"
#include "motion/constantacceleration.h"
#include "drivers/machines/machine.h"
#include "drivers/axisstepper.h"
#include "drivers/linearstepper.h"
#include "drivers/lineardeltastepper.h"
#include "drivers/rpi/rpiiopin.h"
#include "drivers/a4988.h"
//#include "drivers/rpi/sn754410.h"
#include "drivers/linearcoordmap.h"
#include "drivers/lineardeltacoordmap.h"
//#include "drivers/enabledisabledriver.h"
//#include "drivers/rpi/onepinenabler.h"
//#include "drivers/rpi/leverendstop.h"
#include "drivers/rcthermistor.h"
//#include "drivers/rpi/onepiniodriver.h"
#include "drivers/tempcontrol.h"
#include "drivers/fan.h"
#include <tuple>

//R1000 = distance from (0, 0) (platform center) to each axis, in micrometers (1e-6)
//L1000 = length of the rods that connect each axis to the end effector
//STEPS_M = #of steps for the motor driving each axis (A, B, C) to raise its carriage by 1 meter.
//#define R1000 125000
//#define L1000 215000
//#define H1000 507000
//#define STEPS_M 9000
//#define STEPS_M_EXT 4000

//#define R1000 121000
//#define L1000 222000
//#define H1000 518700
//#define STEPS_M 5200 //no microstepping enabled.
//#define STEPS_M_EXT 10000

#define R1000 111000
#define L1000 221000
//#define H1000 467100
#define H1000 467330
#define BUILDRAD1000 85000
#define STEPS_M 6265*4
#define STEPS_M_EXT 10000*8

//#define MAX_ACCEL1000 300000
#define MAX_ACCEL1000 1200000
//Can reach 160mm/sec at full-stepping (haven't tested the limits)
//75mm/sec uses 75% cpu at quarter-stepping (unoptimized)
//90mm/sec uses 75% cpu at quarter-stepping (optimized - Aug 10)
//70mm/sec uses 50-55% cpu at quarter-stepping, but results in missed steps (Aug 17)
//#define MAX_MOVE_RATE 45
//#define MAX_MOVE_RATE 60
#define MAX_MOVE_RATE 50
#define HOME_RATE 10
//#define MAX_EXT_RATE 12
//#define MAX_EXT_RATE 24
#define MAX_EXT_RATE 60

#define THERM_RA 665
//#define THERM_CAP_PICO 100000
#define THERM_CAP_PICO  2200000
#define VCC_mV 3300
#define THERM_IN_THRESH_mV 1600
#define THERM_T0 25
#define THERM_R0 100000
#define THERM_BETA 3950

/*Used IOs:
  3
  5
  7
  8
  10
  (11)
  (12)
  13
  15
  16
  18
  19
  21
  22
  23
  24
  26
*/
/* Calibrating:
  as y leaves 0 to +side, z increases (should stay level)
    Even more so as it goes to -side.
  as x becomes +size, z increases
  as x becomes -size, z increases
  This points to either R or L being off, but in what way?
    joint to edge of bed is ~43 mm. bed is 170mm, so R is 43 + 85 = 128mm
    L is 215mm as measured BUT math doesn't consider the existence of an effector (so L should be longer?)
  Note: increasing L increases convexity (/\)
  Note: decreasing L increases concavity (\/)
  Note: increasing R increases concavity (\/)
  Note: decreasing R increases convexity (/\)
  Note: decreasing R decreases actual displacement (eg X100 becomes only 90mm from center)
  at 121, 222, 60mm in x dir is really 68mm.
*/
namespace drv {

class KosselPi : public Machine {
	private:
		//typedef rpi::OnePinEnabler<RPI_V2_GPIO_P1_16, 0> _StepperEn; //enable pin is LOW for on, HIGH for off
		//typedef EnableDisableDriver<InvertedPin<rpi::RpiIoPin<RPI_V2_GPIO_P1_16, IoHigh> > > _StepperEn;
		typedef InvertedPin<rpi::RpiIoPin<RPI_V2_GPIO_P1_16, IoHigh> > _StepperEn;
		//typedef rpi::LeverEndstop<RPI_V2_GPIO_P1_18, LOW, BCM2835_GPIO_PUD_DOWN> _EndstopA; //endstop is triggered on HIGH
		//typedef rpi::LeverEndstop<RPI_V2_GPIO_P1_24, LOW, BCM2835_GPIO_PUD_DOWN> _EndstopB;
		//typedef rpi::LeverEndstop<RPI_V2_GPIO_P1_26, LOW, BCM2835_GPIO_PUD_DOWN> _EndstopC;
		typedef Endstop<InvertedPin<rpi::RpiIoPin<RPI_V2_GPIO_P1_18, IoLow, BCM2835_GPIO_PUD_DOWN> > > _EndstopA; //endstop is triggered on HIGH
		typedef Endstop<InvertedPin<rpi::RpiIoPin<RPI_V2_GPIO_P1_24, IoLow, BCM2835_GPIO_PUD_DOWN> > > _EndstopB;
		typedef Endstop<InvertedPin<rpi::RpiIoPin<RPI_V2_GPIO_P1_26, IoLow, BCM2835_GPIO_PUD_DOWN> > > _EndstopC;
		//typedef rpi::RCThermistor<RPI_V2_GPIO_P1_07, THERM_RA, THERM_CAP_PICO, VCC_mV, THERM_IN_THRESH_mV, THERM_T0, THERM_R0, THERM_BETA> _Thermistor;
		typedef RCThermistor<rpi::RpiIoPin<RPI_V2_GPIO_P1_07>, THERM_RA, THERM_CAP_PICO, VCC_mV, THERM_IN_THRESH_mV, THERM_T0, THERM_R0, THERM_BETA> _Thermistor;
		//typedef Fan<rpi::OnePinIODriver<RPI_V2_GPIO_P1_08, 1> > _Fan;
		typedef Fan<rpi::RpiIoPin<RPI_V2_GPIO_P1_08, IoLow> > _Fan;
		//typedef rpi::OnePinIODriver<RPI_V2_GPIO_P1_10, 0> _HotendOut;
		typedef InvertedPin<rpi::RpiIoPin<RPI_V2_GPIO_P1_10, IoHigh> > _HotendOut;
		//typedef matr::Identity3Static _BedLevelT;
		/*typedef matr::Matrix3Static<999991837, 1836, -4040369, 
1836, 999999586, 909083, 
4040369, -909083, 999991424, 1000000000> _BedLevelT;*/
		/*typedef matr::Matrix3Static<999948988, 0, -10100494, 
0, 1000000000, 0, 
10100494, 0, 999948988, 1000000000> _BedLevelT;*/
		/*typedef matr::Matrix3Static<999987246, 0, -5050440, 
0, 1000000000, 0, 
5050440, 0, 999987246, 1000000000> _BedLevelT;*/
		/*typedef matr::Matrix3Static<999997959, 0, -2020197, 
0, 1000000000, 0, 
2020197, 0, 999997959, 1000000000> _BedLevelT; //[-0.002, 0.00, 0.99]*/
		/*typedef matr::Matrix3Static<999993750, 892, -3535330, 
892, 999999872, 505047, 
3535330, -505047, 999993623, 1000000000> _BedLevelT; //[-0.0035, 0.0005, 0.99]*/
		/*typedef matr::Matrix3Static<999989669, 1147, -4545407, 
1147, 999999872, 505045, 
4545407, -505045, 999989542, 1000000000> _BedLevelT; //[-0.0045, 0.0005, 0.99]*/
		/*typedef matr::Matrix3Static<999975003, 1785, -7070529, 
1785, 999999872, 505037, 
7070529, -505037, 999974875, 1000000000> _BedLevelT; //[-0.007, 0.0005, 0.99]*/
		typedef matr::Matrix3Static<999975003, 5356, -7070522, 
5356, 999998852, 1515111, 
7070522, -1515111, 999973855, 1000000000> _BedLevelT; //[-0.007, 0.0015, 0.99]
    public:
        //typedef ExponentialAcceleration<MAX_ACCEL1000> AccelerationProfileT;
        typedef ConstantAcceleration<MAX_ACCEL1000> AccelerationProfileT;

        typedef LinearDeltaCoordMap</*0, 1, 2, 3, */ R1000, L1000, H1000, BUILDRAD1000, STEPS_M, STEPS_M_EXT, _BedLevelT> CoordMapT;
        typedef std::tuple<LinearDeltaStepper<0, CoordMapT, R1000, L1000, STEPS_M, _EndstopA>, LinearDeltaStepper<1, CoordMapT, R1000, L1000, STEPS_M, _EndstopB>, LinearDeltaStepper<2, CoordMapT, R1000, L1000, STEPS_M, _EndstopC>, LinearStepper<STEPS_M_EXT, COORD_E> > AxisStepperTypes;
        typedef std::tuple<
        	//rpi::A4988<RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_23, _StepperEn>, //A tower
        	//rpi::A4988<RPI_V2_GPIO_P1_19, RPI_V2_GPIO_P1_21, _StepperEn>, //B tower
        	//rpi::A4988<RPI_V2_GPIO_P1_13, RPI_V2_GPIO_P1_15, _StepperEn>, //C tower
        	//rpi::A4988<RPI_V2_GPIO_P1_03, RPI_V2_GPIO_P1_05, _StepperEn>, //E coord
        	A4988<rpi::RpiIoPin<RPI_V2_GPIO_P1_22>, rpi::RpiIoPin<RPI_V2_GPIO_P1_23>, _StepperEn>,
        	A4988<rpi::RpiIoPin<RPI_V2_GPIO_P1_19>, rpi::RpiIoPin<RPI_V2_GPIO_P1_21>, _StepperEn>,
        	A4988<rpi::RpiIoPin<RPI_V2_GPIO_P1_13>, rpi::RpiIoPin<RPI_V2_GPIO_P1_15>, _StepperEn>,
        	A4988<rpi::RpiIoPin<RPI_V2_GPIO_P1_03>, rpi::RpiIoPin<RPI_V2_GPIO_P1_05>, _StepperEn>,
        	_Fan,
        	//12000, 3000, 1000 gives osc of ~3 min (20C-80C). Converges.
        	//20000,  600,    0 (50C->80C). Converges. No osc. Takes 2 minutes to progress from 81C to 80C. Peaks at 130C when from (80C->120C). Critically damped. Takes 90 seconds to stabilize *near* target.
        	//12000,  600, 1200 (50C->130C). Peaks 22C above target. Underdamped. 5 mins to converge
        	//18000,  300, 1000 (40C->130C). Overdamped. 4.5 minutes to reach target (& is stabilized when it gets there)
        	TempControl<drv::HotendType, 5, _HotendOut, _Thermistor, PID<18000, 250, 1000, 1000000>, LowPassFilter<3000> >
        	//_EndstopA, _EndstopB, _EndstopC
        	> IODriverTypes;
        //std::tuple<_EndstopA, _EndstopB, _EndstopC> _endstops;
        inline float defaultMoveRate() const { //in mm/sec
        	return MAX_MOVE_RATE;
        }
        //currently have to be satisfied with mins/maxes - can't achieve more without muddying the interface, and I see little reason for having more.
        inline float maxRetractRate() const { //in mm/sec
        	return MAX_EXT_RATE;
        }
        inline float maxExtrudeRate() const { //in mm/sec
        	return MAX_EXT_RATE;
        }
        inline float clampMoveRate(float inp) const {
        	return std::min(inp, defaultMoveRate());//ensure we never move too fast.
        }
        inline float clampHomeRate(float /*inp*/) const {
        	return HOME_RATE;
        }
        inline bool doHomeBeforeFirstMovement() const {
        	return true; //if we get a G1 before the first G28, then yes - we want to home first!
        }
};

}

#endif
