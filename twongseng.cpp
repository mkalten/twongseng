// Copyright (C) 2009  Fajran Iman Rusadi <fajran@gmail.com>

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// Based on multitouch code from http://www.steike.com/code/multitouch/

#include "multitouch.h"
#include "twongseng.h"
#include <iostream>
#include <math.h>
#include <unistd.h>

#include <TuioServer.h>
#include <TcpSender.h>
#include <WebSockSender.h>
#include <TuioPointer.h>
#include <TuioTime.h>

#include <set>
#include <map>

#define D(s) /* std::cout << s << std::endl; */

#define EXISTS(c, v) ((c).find(v) != (c).end())
#define FOREACH(c, i) for (i=(c).begin(); i!=(c).end(); i++)

static MTDeviceRef dev;
static TUIO2::TuioServer* server;
static TUIO2::OscSender* oscSender;
static bool running = false;
static bool verbose = false;
static std::string host("localhost");
static int port = 3333;
static int protocol = TUIO_UDP;
static int dev_id = 0;
static float y_scale;

// Sensitivity
#define MIN_DISTANCE 0.00001f

// Events dispatch interval
#define SAMPLING_INTERVAL 20000
static long lastFrameTimeSec = -1;
static long lastFrameTimeUsec = -1;

// Active fingers
static std::set<int> currentFingers;

// Position of active fingers
static std::map<int, float> lastX;
static std::map<int, float> lastY;

// Finger to TUIO cursors mapping
static std::map<int, TUIO2::TuioPointer*> cursors;

// Check if a position changes is considered as a movement
static inline bool isMoving(int id, float x, float y)
{
	float dx = x - lastX[id];
	float dy = y - lastY[id];
	float distance2 = dx*dx + dy*dy;
	D("distance: id=" << id << ", distance=" << distance2);
	return distance2 > MIN_DISTANCE;
}

// New touch
static void touch_add(int id, float x, float y)
{
	D("touch_add: id=" << id << ", x=" << x << ", y=" << y);
	lastX[id] = x;
	lastY[id] = y;

	TUIO2::TuioObject * tobj = server->createTuioPointer(x, y, 0, 0, 0, 0);
	cursors[id] = tobj->getTuioPointer();
}

// Update touch
static void touch_update(int id, float x, float y)
{
	if (isMoving(id, x, y)) {
		D("touch_update: id=" << id << ", x=" << x << ", y=" << y);
		lastX[id] = x;
		lastY[id] = y;

		TUIO2::TuioPointer* cursor = cursors[id];
		server->updateTuioPointer(cursor, x, y, 0, 0, 0, 0);
	}
}

// Remove touch
static void touch_remove(int id)
{
	lastX.erase(id);
	lastY.erase(id);

	D("touch_remove: id=" << id);

	TUIO2::TuioPointer* cursor = cursors[id];
	server->removeTuioPointer(cursor);
	cursors.erase(id);
}

// Event dispatch guard
static bool sampling_interval_passed()
{
	TUIO2::TuioTime currentTime = TUIO2::TuioTime::getSystemTime();
	long dsec = currentTime.getSeconds() - lastFrameTimeSec;
	long dusec = currentTime.getMicroseconds() - lastFrameTimeUsec;

	bool res = false;

	if (lastFrameTimeSec == -1) {
		lastFrameTimeSec = currentTime.getSeconds();
		lastFrameTimeUsec = currentTime.getMicroseconds();
		res = true;
	}
	else if (dsec > 1) {
		res = true;
	}
	else {
		long delta =  dsec * 1000000 + dusec;
		if (delta > SAMPLING_INTERVAL) {
			res = true;
		}
	}

	return res;
}

// Start creating TUIO messages
static void tuio_frame_begin()
{
	TUIO2::TuioTime currentTime = TUIO2::TuioTime::getSystemTime();
	server->initTuioFrame(currentTime);
}

// Flush TUIO messages
static void tuio_frame_end()
{
	server->stopUntouchedMovingObjects();
	server->commitTuioFrame();
}

typedef void (*MTPathCallbackFunction)(MTDeviceRef device, long pathID, long state, MTTouch* touch);


// Process incoming events
static void callback(MTDeviceRef device, MTTouch touches[], size_t numTouches, double timestamp, size_t frame) {
	
	if (!running || !sampling_interval_passed()) {
		return;
	}

	tuio_frame_begin();

	std::set<int> fingers;

	// Process incoming events
	int i;
	for (i=0; i<numTouches; i++) {
		MTTouch *f = &touches[i];
		int id = f->pathIndex;

		if (f->normalizedVector.position.y>y_scale) y_scale = f->normalizedVector.position.y;
		float x = f->normalizedVector.position.x;
		float y = 1.0f - f->normalizedVector.position.y/y_scale; // reverse y axis
		
		if (x<0) x=0; else if (x>1) x=1;
		if (y<0) y=0; else if (y>1) y=1;
		
		if (EXISTS(currentFingers, id)) {
			// update
			touch_update(id, x, y);
		}
		else {
			// add
			touch_add(id, x, y);
		}
		fingers.insert(id);
	}

	// Remove old events
	std::set<int>::iterator iter;
	FOREACH(currentFingers, iter) {
		int id = *iter;
		if (!EXISTS(fingers, id)) {
			// remove
			touch_remove(id);
		}
	}

	// Update active fingers list
	currentFingers.clear();
	currentFingers.insert(fingers.begin(), fingers.end());

	tuio_frame_end();
}

// Start TUIO server
static void tuio_start()
{
	switch (protocol) {
		case TUIO_TCP: oscSender = new TUIO2::TcpSender(port); break;
		case TUIO_WEB: oscSender = new TUIO2::WebSockSender(port); break;
		default: oscSender = new TUIO2::UdpSender((char*)host.c_str(), port); break;
	}
	server = new TUIO2::TuioServer(oscSender);
	server->setVerbose(verbose);
	server->setSourceName("twongseng");
}

// Release all active fingers
static void release_all_fingers()
{
	tuio_frame_begin();

	std::set<int>::iterator iter;
	FOREACH(currentFingers, iter) {
		touch_remove(*iter);
	}

	tuio_frame_end();
}

// Stop TUIO server
static void tuio_stop()
{
	release_all_fingers();
	delete server;
}

// Start handling multitouch events
static void mt_start()
{
	CFArrayRef devList = MTDeviceCreateList();
	if((CFIndex)CFArrayGetCount(devList) - 1 < dev_id) {
		std::cout << "Could not find external trackpad, defaulting to internal!" << std::endl;
		dev_id = 0;
	}
	dev = (MTDeviceRef)CFArrayGetValueAtIndex(devList, dev_id);
	MTRegisterContactFrameCallback(dev, callback);
	MTDeviceStart(dev, 0);
	
	y_scale = 1;
}

// Stop handling multitouch events
static void mt_stop()
{
	MTUnregisterContactFrameCallback(dev, callback);
	MTDeviceStop(dev);
	MTDeviceRelease(dev);
}

// Set hostname and port that will be used by TUIO server
void twongseng_set_hostname_and_port(const char* _hostname, int _port)
{
	host = _hostname;
	port = _port;
}

// Set hostname and port that will be used by TUIO server
void twongseng_set_protocol(int _protocol)
{
	protocol = _protocol;
}

// Set TUIO server verbosity
void twongseng_set_verbose(int _verbose)
{
	verbose = _verbose != 0;
}

// Set TUIO server verbosity
void twongseng_set_device(int _id)
{
	dev_id = _id;
}

// Start Tongseng
void twongseng_start()
{
	mt_start();
	tuio_start();
	running = true;
}

// Stop Tongseng
void twongseng_stop()
{
	if (running) {
		running = false;
		tuio_stop();
		mt_stop();
	}
}

// List devices
void twongseng_list_devices()
{
	CFArrayRef devList = MTDeviceCreateList();
	CFIndex dev_count = (CFIndex)CFArrayGetCount(devList);
	
	if (dev_count == 0) std::cout << "no devices found" << std::endl;
	else std::cout << "0: default" << std::endl;

	if(dev_count > 1) {
		for (int i=1;i<=dev_count;i++)
			std::cout << i << ": external" << std::endl;
	}
}
