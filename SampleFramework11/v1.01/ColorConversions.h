#pragma once

#include "SF11_Math.h"

// -- color functions
double Blackbody(double wl, double temp);
SampleFramework11::Float3 XYZ2RGB(double X, double Y, double Z);
SampleFramework11::Float3 ComputeBlackbody(double temperature);

void ConvertsRGBToSpectrum(float* spectrum, uint32 numsamples, 
                           SampleFramework11::Float3 sRGB);
SampleFramework11::Float3 ConvertSpectrumTosRGB(float* spectrum, uint32 numsamples,
                                                uint32 lowerBound, uint32 upperBound,
                                                uint32 waveInc);