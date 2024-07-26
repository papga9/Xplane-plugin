// Downloaded from https://developer.x-plane.com/code-sample/beacons-and-strobes/


/*
/*

	Beacon and strobe example plugin.
	
	This plugin overrides X-Plane's default flash patterns for the beacons and strobes.  It provides a multi-part pulse for the strobes,
	and sets the beacons to run at different rates.

*/

#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"

#include <string.h>
#include <math.h>

// The time stamp that the strobes last fired.
float strobe1_t = 0.0, strobe2_t = 0.0;

/* This is the period of the entire strobe sequence. */
#define STROBE_TIME 1.0

/* This is the time between the first and second strobe firing. */
#define STROBE_OFFSET 0.2

static XPLMDataRef override_beacons_and_strobes = NULL;

// These tell us if the strobes and beacons are really on.  They take into consideration both the switch positions and electrical failures.	
static XPLMDataRef beacon_lights_on = NULL;
static XPLMDataRef strobe_lights_on = NULL;

// When we are overriding beacons and strobes, we are responsible for writing each of these per frame. 
// These are four-item arrays, so we can make multiple strobe flash patterns and separately running beacons.
static XPLMDataRef beacon_brightness_ratio = NULL;
static XPLMDataRef strobe_brightness_ratio = NULL;
// Set this to 1 if ANY strobe is on - this is what makes the clouds light up, etc.
static XPLMDataRef strobe_flash_now = NULL;

// Deferred init.  Plugins should wait until the sim is really running to do final
// initialization.  See http://www.xsquawkbox.net/xpsdk/mediawiki/DeferredInitialization
// for more info.
static float deferred_init(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon)
{
	XPLMSetDatai(override_beacons_and_strobes,1);
	
	strobe1_t = XPLMGetElapsedTime();
	strobe2_t = strobe1_t + STROBE_OFFSET;
	return 0;
}

// This is our main per-frame lighting function.  This is where we do the work X-plane would normally
// do to calculate what the beacons and strobes are doing.
static float lights_per_frame(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon)
{
	float now = XPLMGetElapsedTime();
	float beacons[4] = { 0 };
	float flash[4] = { 0 };
	int	any_flash = 0;	
	int strobes_on = XPLMGetDatai(strobe_lights_on);
	
	if(beacon_lights_on == NULL || strobe_lights_on == NULL || beacon_brightness_ratio == NULL)
	{
		XPLMDebugString("We are missing our datarefs.\n");
		return 0.0f;
	}

	// Beacon lighting calculation.  If the beacons are on, use sine and cosine for the two
	// beacons...and use a slightly different scale so they run at different spededs.

	if(XPLMGetDatai(beacon_lights_on))
	{
		beacons[0] = sin(now * 1.5);
		beacons[1] = cos(now);
	}
	XPLMSetDatavf(beacon_brightness_ratio,beacons,0,4);	

	// Strobes.  Each time we are past our "strobe time", flash for one frame, then go out again.
	// Note that this is a TERRIBLE strobing algorithm: at high fps the strobes are on for TINY amounts of time.
	// In your strobing algorithm, make sure the strobes are on for the longer of one frame or a certain minimum time.
	// Some users do run at 80+ fps!

	
	if ((now - strobe1_t) > STROBE_TIME)
	{
		flash[0] = strobes_on;
		any_flash |= strobes_on;
		strobe1_t += STROBE_TIME;
	}
	if ((now - strobe2_t) > STROBE_TIME)
	{
		flash[1] = strobes_on;
		any_flash |= strobes_on;
		strobe2_t += STROBE_TIME;
	}

	XPLMSetDatavf(strobe_brightness_ratio,flash,0,4);
	XPLMSetDatai(strobe_flash_now, any_flash);

	return -1.0;	
}


PLUGIN_API int XPluginStart(
						char *		outName,
						char *		outSig,
						char *		outDesc)
{
	strcpy(outName, "Custom Beacons/Strobes Example");
	strcpy(outSig, "xplanesdk.examples.custom_beacons_strobes");
	strcpy(outDesc, "A plugin that demonstrates custom beacon/strobe patterns.");

	override_beacons_and_strobes = XPLMFindDataRef("sim/flightmodel2/lights/override_beacons_and_strobes");
	
	if(override_beacons_and_strobes == NULL)
	{
		XPLMDebugString("Beacon and strobe plugin disabled - this version of x-plane doesn't support the feature.\n");
		return 0;
	}

	beacon_lights_on = XPLMFindDataRef("sim/cockpit/electrical/beacon_lights_on");
	strobe_lights_on = XPLMFindDataRef("sim/cockpit/electrical/strobe_lights_on");
	beacon_brightness_ratio = XPLMFindDataRef("sim/flightmodel2/lights/beacon_brightness_ratio");
	strobe_brightness_ratio = XPLMFindDataRef("sim/flightmodel2/lights/strobe_brightness_ratio");
	strobe_flash_now = XPLMFindDataRef("sim/flightmodel2/lights/strobe_flash_now");	

	XPLMRegisterFlightLoopCallback(deferred_init, -1.0, NULL);
	XPLMRegisterFlightLoopCallback(lights_per_frame, -2.0, NULL);
	
	
	return 1;
}

PLUGIN_API void	XPluginStop(void)
{
	XPLMUnregisterFlightLoopCallback(deferred_init,NULL);
	XPLMUnregisterFlightLoopCallback(lights_per_frame,NULL);
	XPLMSetDatai(override_beacons_and_strobes,0);

}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(
					XPLMPluginID	inFromWho,
					int				inMessage,
					void *			inParam)
{
}

