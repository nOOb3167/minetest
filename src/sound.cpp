/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "sound.h"
#include "util/numeric.h"

// Global DummySoundManager singleton
DummySoundManager       dummySoundManager;
DummySoundManagerGlobal dummySoundManagerGlobal;

ISoundManagerGlobal *g_sound_manager_global = NULL;

unsigned long SimpleSoundSpec::convertOffsetToSampleOffset(
		unsigned long channels, unsigned long bits_per_sample, unsigned long frequency,
		unsigned long buffer_size_bytes, double offset)
{
	const unsigned long BytesPerSampleFrame = channels * (bits_per_sample / 8);
	const unsigned long NumBufferSampleFrames = buffer_size_bytes / BytesPerSampleFrame;
	assert(buffer_size_bytes % BytesPerSampleFrame == 0);
	unsigned long SampleFrameOfOffset = offset * frequency;
	// remember sample frames range [0, NumBufferSampleFrames)
	//   therefore last is NumBufferSampleFrames - 1
	if (offset == -1.0) // special value for 'offset at the very end'
		SampleFrameOfOffset = NumBufferSampleFrames - 1;
	return rangelim(SampleFrameOfOffset, 0, NumBufferSampleFrames - 1);
}

double SimpleSoundSpec::convertOffsetRangeToDeltaTime(
	unsigned long channels, unsigned long bits_per_sample,
	unsigned long buffer_size_bytes, unsigned long frequency,
	double offset_start, double offset_end)
{
	const unsigned long SampleOffsetStart = convertOffsetToSampleOffset(
		channels, bits_per_sample, frequency, buffer_size_bytes, offset_start);
	const unsigned long SampleOffsetEnd = convertOffsetToSampleOffset(
		channels, bits_per_sample, frequency, buffer_size_bytes, offset_end);
	const unsigned long DeltaSamples = SampleOffsetEnd - SampleOffsetStart;
	const double DeltaTime = (double) DeltaSamples / frequency;
	return DeltaTime;
}
