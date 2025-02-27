/*
 * Copyright (c) 2021 Eurotec, Netherlands
 * All rights reserved.
 *
 * Author: Jad Haj Mustafa
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <vector>

namespace fadecandy_driver
{
//!
//! \brief The color struct contains the r, g and b colors
//!
struct Color
{
  Color(int r, int g, int b);

  int r_;
  int g_;
  int b_;
};

//!
//! \brief makeVideoUsbPackets Construct the USB packets to set all LED
//! strips to the given colors.
//! To simplify things, we always send values for all 8 * 64 LEDs. If the
//! physical strips are shorter, or there are less then 8 strips, the extra
//! data doesn't do anything.
//! If the user gives us values for less than the total number of strips, or
//! less than the total number of LEDs in any given strip, all unspecified
//! LEDs are left dark.
//!
std::vector<std::vector<unsigned char>> makeVideoUsbPackets(const std::vector<std::vector<Color>>& led_array_colors);

//!
//! \brief makeLookupTablePackets Create USB packets for a simple color
//! lookup table. The entire red lookup table comes first, then the entire
//! green channel, then the entire red channel.
//!
std::vector<std::vector<unsigned char>> makeLookupTablePackets(const std::vector<int>& red_lookup_values,
                                                               const std::vector<int>& green_lookup_values,
                                                               const std::vector<int>& blue_lookup_values);
//!
//! \brief makeLookupTablePackets Return lookup tables as 3 lists of lookup
//! values - one for the red channel, one for the green channel, and one for
//! the blue channel.
//!
std::vector<int> makeDefaultLookupTable();
}  // namespace fadecandy_driver
