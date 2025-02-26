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

#include <iomanip>

#include "./fadecandy_driver.h"

namespace fadecandy_driver
{
constexpr int USB_PACKET_SIZE = 64;
constexpr int USB_PRODUCT_ID = 0x607a;
constexpr int USB_VENDOR_ID = 0x1d50;
constexpr int USB_ENDPOINT = 1;
constexpr int INTERFACE_NO = 0x00;

FadecandyDriver::FadecandyDriver()
{
  int r = libusb_init(&context_);
  if (r < 0)
  {
    throw std::runtime_error("Could not start USB session.");
  }
}

FadecandyDriver::~FadecandyDriver()
{
  libusb_exit(context_);
}

std::string FadecandyDriver::connect()
{
  releaseInterface();

  // Find usb device.
  libusb_device_descriptor fadecandy_device_descriptor = findUsbDevice();

  dev_handle_ = libusb_open_device_with_vid_pid(context_, USB_VENDOR_ID, USB_PRODUCT_ID);

  if (dev_handle_ == NULL)
  {
    throw std::runtime_error("Could not open device.");
  }

  // Check if kernel driver, detach
  if (libusb_kernel_driver_active(dev_handle_, INTERFACE_NO) == 1)
  {
    if (libusb_detach_kernel_driver(dev_handle_, INTERFACE_NO) != 0)
    {
      dev_handle_ = NULL;
      throw std::runtime_error("Could not detach kernel driver.");
    }
  }

  // Claim interface
  int r = libusb_claim_interface(dev_handle_, INTERFACE_NO);
  if (r < 0)
  {
    dev_handle_ = NULL;
    throw std::runtime_error("Could not claim device interface.");
  }

  unsigned char serial[USB_PACKET_SIZE];
  libusb_get_string_descriptor_ascii(dev_handle_, fadecandy_device_descriptor.iSerialNumber, serial, USB_PACKET_SIZE);
  std::string serial_number = reinterpret_cast<char*>(serial);

  // Prepare command
  int actual_written;
  const int timeout = 10000;

  std::vector<int> array = makeDefaultLookupTable();
  std::vector<std::vector<unsigned char>> packets = makeLookupTablePackets(array, array, array);
  for (auto& packet : packets)
  {
    r = libusb_bulk_transfer(dev_handle_, USB_ENDPOINT, packet.data(), USB_PACKET_SIZE, &actual_written, timeout);
    if (r != 0 && actual_written != USB_PACKET_SIZE)
    {
      dev_handle_ = NULL;
      throw std::runtime_error("Failed to write data on device.");
    }
  }

  return serial_number;
}

bool FadecandyDriver::isConnected()
{
  return dev_handle_ != NULL;
}

void FadecandyDriver::setColors(std::vector<std::vector<Color>> led_colors)
{
  if (!isConnected())
  {
    throw std::runtime_error("Not connected");
  }

  std::vector<std::vector<unsigned char>> usb_packets = makeVideoUsbPackets(led_colors);
  for (auto& usb_packet : usb_packets)
  {
    int actual_written;
    const int timeout = 10000;
    int r =
        libusb_bulk_transfer(dev_handle_, USB_ENDPOINT, usb_packet.data(), USB_PACKET_SIZE, &actual_written, timeout);
    if (r != 0 || actual_written != USB_PACKET_SIZE)
    {
      releaseInterface();
      throw std::runtime_error("Could not write on the driver.");
    }
  }
}

libusb_device_descriptor FadecandyDriver::findUsbDevice()
{
  libusb_device** device_list = nullptr;
  libusb_device_descriptor fadecandy_device_descriptor;

  unsigned count = libusb_get_device_list(context_, &device_list);
  for (size_t idx = 0; idx < count; ++idx)
  {
    libusb_device* device = device_list[idx];
    libusb_device_descriptor desc;

    int r = libusb_get_device_descriptor(device, &desc);
    if (r < 0)
    {
      libusb_free_device_list(device_list, count);
      throw std::runtime_error("Could not get device descriptor.");
    }
    if (desc.idVendor == USB_VENDOR_ID && desc.idProduct == USB_PRODUCT_ID)
    {
      fadecandy_device_descriptor = desc;
    }
  }
  libusb_free_device_list(device_list, count);
  return fadecandy_device_descriptor;
}

void FadecandyDriver::releaseInterface()
{
  if (isConnected())
  {
    int r = libusb_release_interface(dev_handle_, INTERFACE_NO);
    if (r < 0)
    {
      dev_handle_ = NULL;
      throw std::runtime_error("Could not release device; the device might be disconnected.");
    }
    libusb_close(dev_handle_);
    dev_handle_ = NULL;
  }
}
}  // namespace fadecandy_driver
