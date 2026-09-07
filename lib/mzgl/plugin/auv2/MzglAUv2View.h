#pragma once

// Objective-C class names for the AUv2 Cocoa view factory and host view.
//
// Every one of our .component bundles carries its own copy of these classes,
// and a host may load several of them into one process; the ObjC runtime only
// keeps one class per name. So the names are prefixed per plugin with
// MZGL_AUV2_CLASS_PREFIX (set to the CMake target name by mzgl_add_auv2_plugin).
//
//   MZGL_AUV2_VIEW_FACTORY_CLASS  e.g. GainAUv2_ViewFactory  (AUCocoaUIBase)
//   MZGL_AUV2_HOST_VIEW_CLASS     e.g. GainAUv2_View         (NSView hosting mzgl)
//   MZGL_AUV2_EVENTS_VIEW_CLASS   e.g. GainAUv2_EventsView   (EventsView subclass)
//
// MZGL_AUV2_VIEW_FACTORY_CLASS_NAME is the factory class name as a C string,
// which MzglAUv2Effect hands to the host in kAudioUnitProperty_CocoaUI.

#ifndef MZGL_AUV2_CLASS_PREFIX
#	define MZGL_AUV2_CLASS_PREFIX MzglAUv2
#endif

#define MZGL_AUV2_CONCAT_(a, b) a##b
#define MZGL_AUV2_CONCAT(a, b)	MZGL_AUV2_CONCAT_(a, b)
#define MZGL_AUV2_STR_(x)		#x
#define MZGL_AUV2_STR(x)		MZGL_AUV2_STR_(x)

#define MZGL_AUV2_VIEW_FACTORY_CLASS MZGL_AUV2_CONCAT(MZGL_AUV2_CLASS_PREFIX, _ViewFactory)
#define MZGL_AUV2_HOST_VIEW_CLASS	 MZGL_AUV2_CONCAT(MZGL_AUV2_CLASS_PREFIX, _View)
#define MZGL_AUV2_EVENTS_VIEW_CLASS	 MZGL_AUV2_CONCAT(MZGL_AUV2_CLASS_PREFIX, _EventsView)
#define MZGL_AUV2_RESIZE_GRIP_CLASS	 MZGL_AUV2_CONCAT(MZGL_AUV2_CLASS_PREFIX, _ResizeGrip)

#define MZGL_AUV2_VIEW_FACTORY_CLASS_NAME MZGL_AUV2_STR(MZGL_AUV2_VIEW_FACTORY_CLASS)
