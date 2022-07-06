//
//  maths.hpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

float mapf(float inp, float inMin, float inMax, float outMin, float outMax, bool clamp = false);

double mapd(double inp, double inMin, double inMax, double outMin, double outMax, bool clamp = false);


int clampi(int inp, int from, int to);
long clampl(long inp, long from, long to);
float clampf(float inp, float from, float to);
double clampd(double inp, double from, double to);

float nearestPow2(float v);

// random [0,1]
float randuf();

// random [-1,1]
float randf();
float randf(float to);
float randf(float from, float to);
int randi(int to);
int randi(int from, int to);
bool randb();
float rms2db(float inp);
float db2rms(float db);

float smoothstep(float x);
float smootherstep(float x);
