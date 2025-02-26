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

#include <libusb-1.0/libusb.h>
#include <string>
#include <vector>

#include "./util.h"

namespace fadecandy_driver
{
class FadecandyDriver
{
public:
  FadecandyDriver();
  ~FadecandyDriver();

  //!
  //! \brief connect Initialize the Fadecandy device
  //! \return Serial number
  //!
  std::string connect();

  //!
  //! \brief isConnected
  //! \return True if connected, False otherwise
  //!
  bool isConnected();

  //!
  //! \brief setColors Transfer the LED color stream to the driver
  //!
  void setColors(std::vector<std::vector<Color>> led_colors);

private:
  //!
  //! \brief context_ Lib USB Context
  //!
  libusb_context* context_ = NULL;

  //!
  //! \brief dev_handle_ USB device handle
  //!
  libusb_device_handle* dev_handle_ = NULL;

  //!
  //! \brief findUsbDevice Search the fadcandy device with particular vendor and
  //! product id
  //!
  libusb_device_descriptor findUsbDevice();

  //!
  //! \brief releaseInterface Release fadecandy device interface
  //!
  void releaseInterface();
};
}  // namespace fadecandy_driver
