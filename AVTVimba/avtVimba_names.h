/* ********************************************************************
Plugin "AVTVimba" for itom software
URL : http ://www.uni-stuttgart.de/ito
Copyright(C) 2016, Institut fuer Technische Optik (ITO),
                   Universitaet Stuttgart, Germany;
                   IPROM, TU Braunschweig, Germany

This file is part of a plugin for the measurement software itom.

This itom - plugin is free software; you can redistribute it and / or modify it
under the terms of the GNU Library General Public Licence as published by
the Free Software Foundation; either version 2 of the Licence, or(at
your option) any later version.

itom and its plugins are distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU Library
General Public Licence for more details.

You should have received a copy of the GNU Library General Public License
along with itom.If not, see <http://www.gnu.org/licenses/>.
*********************************************************************** */


#if defined(USE_API_5)
    #define P_STREAMBPS_NAME "StreamBytesPerSecond"
#else
    #define P_STREAMBPS_NAME "StreamBytesPerSecond"
#endif

#define P_DEVICELINKLIMIT_NAME "DeviceLinkThroughputLimit"
#define P_DEVICELINKLIMITMODE_NAME "DeviceLinkThroughputLimitMode"
#define P_EXPOSURETIME_NAME "ExposureTime"
#define P_FRAMERATE_NAME "AcquisitionFrameRate"