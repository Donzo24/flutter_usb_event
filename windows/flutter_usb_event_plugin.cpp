#include "flutter_usb_event_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <windows.h>
#include <dbt.h>
#include <initguid.h>
#include <usbioctl.h>
#include <setupapi.h>
#include <winusb.h>
#include <memory>
#include <sstream>
#include <vector>
#include <iostream>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")

// Helper function to convert wide strings to UTF-8 strings
std::string ConvertWStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);

    return strTo;
}

// Function to open the USB device
HANDLE OpenUsbDevice(const std::string& devicePath) {
    std::wstring devicePathW(devicePath.begin(), devicePath.end());
    
    HANDLE deviceHandle = CreateFileW(
        devicePathW.c_str(),
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL);

    if (deviceHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to open device handle. Error code: " << GetLastError() << std::endl;
        return nullptr;
    }

    return deviceHandle;
}

// Function to initialize WinUSB
bool InitializeWinUsb(HANDLE deviceHandle, WINUSB_INTERFACE_HANDLE &usbHandle) {
    if (!WinUsb_Initialize(deviceHandle, &usbHandle)) {
        std::cerr << "Error: WinUSB initialization failed. Error code: " << GetLastError() << std::endl;
        return false;
    }
    return true;
}

// Function to return device path only
std::string GetDevicePathUsingWinUsb(const std::string& device_path) {
    // Convert device path to WCHAR (wide character string) for CreateFileW
    std::wstring device_path_w(device_path.begin(), device_path.end());

    HANDLE deviceHandle = CreateFileW(
        device_path_w.c_str(),
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL);

    if (deviceHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to open device handle. Error code: " << GetLastError() << std::endl;
        return "Error opening device handle";
    }

    WINUSB_INTERFACE_HANDLE usbHandle;
    if (!WinUsb_Initialize(deviceHandle, &usbHandle)) {
        std::cerr << "Error: WinUSB initialization failed. Error code: " << GetLastError() << std::endl;
        CloseHandle(deviceHandle);
        return "WinUSB initialization failed";
    }

    WinUsb_Free(usbHandle);
    CloseHandle(deviceHandle);

    return device_path;
}

// Function to check if the device is a USB device based on its path
bool IsUsbDevice(const std::string& device_path) {
    // Check if the device path contains "USB#VID" which is typical for USB devices
    return device_path.find("USB#VID") != std::string::npos;
}

namespace flutter_usb_event {

// Static method to register the plugin with the registrar
void FlutterUsbEventPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  auto plugin = std::make_unique<FlutterUsbEventPlugin>(registrar);
  registrar->AddPlugin(std::move(plugin));
}

FlutterUsbEventPlugin::FlutterUsbEventPlugin(
    flutter::PluginRegistrarWindows* registrar)
    : registrar_(registrar), hwnd_(nullptr) {
  const std::wstring window_class_name = L"USB_DEVICE_LISTENER";

  WNDCLASS window_class = {};
  window_class.lpfnWndProc = WindowProc;
  window_class.hInstance = GetModuleHandle(nullptr);
  window_class.lpszClassName = window_class_name.c_str();
  RegisterClass(&window_class);

  hwnd_ = CreateWindow(window_class_name.c_str(), L"", 0, 0, 0, 0, 0, nullptr,
                       nullptr, GetModuleHandle(nullptr), this);

  channel_ = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
      registrar_->messenger(), "flutter_usb_event",
      &flutter::StandardMethodCodec::GetInstance());

  channel_->SetMethodCallHandler(
      [this](const flutter::MethodCall<flutter::EncodableValue>& call,
             std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        HandleMethodCall(call, std::move(result));
      });

  // Set up device notification filter for all USB devices
  DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
  ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
  NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
  NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
  NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;  // For USB devices

  RegisterDeviceNotification(hwnd_, &NotificationFilter,
                             DEVICE_NOTIFY_WINDOW_HANDLE);
}

FlutterUsbEventPlugin::~FlutterUsbEventPlugin() {
  if (hwnd_) {
    DestroyWindow(hwnd_);
  }
}

void FlutterUsbEventPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare("startListening") == 0) {
    result->Success();
  } else if (method_call.method_name().compare("stopListening") == 0) {
    result->Success();
  } else {
    result->NotImplemented();
  }
}

// Static method for handling Windows messages
LRESULT CALLBACK FlutterUsbEventPlugin::WindowProc(HWND hwnd, UINT uMsg,
                                                   WPARAM wParam, LPARAM lParam) {
  if (uMsg == WM_CREATE) {
    SetWindowLongPtr(hwnd, GWLP_USERDATA,
                     reinterpret_cast<LONG_PTR>(
                         reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams));
  } else if (uMsg == WM_DEVICECHANGE) {
    auto plugin = reinterpret_cast<FlutterUsbEventPlugin*>(
        GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (plugin && wParam == DBT_DEVICEARRIVAL) {
      // Handle device connected
      DEV_BROADCAST_HDR* hdr = reinterpret_cast<DEV_BROADCAST_HDR*>(lParam);
      if (hdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
        DEV_BROADCAST_DEVICEINTERFACE* dev_interface =
            reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE*>(hdr);

        std::string device_path = ConvertWStringToString(dev_interface->dbcc_name);

        std::cerr << "Device connected with path: " << device_path << std::endl;  // Debug output

        // Check if the device path matches USB devices
        if (IsUsbDevice(device_path)) {
            // Return the device path directly
            plugin->channel_->InvokeMethod("onDeviceConnected",
                                           std::make_unique<flutter::EncodableValue>(device_path));
        }
      }
    } else if (plugin && wParam == DBT_DEVICEREMOVECOMPLETE) {
      // Handle device disconnected
      DEV_BROADCAST_HDR* hdr = reinterpret_cast<DEV_BROADCAST_HDR*>(lParam);
      if (hdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
        DEV_BROADCAST_DEVICEINTERFACE* dev_interface =
            reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE*>(hdr);

        std::string device_path = ConvertWStringToString(dev_interface->dbcc_name);

        std::cerr << "Device disconnected with path: " << device_path << std::endl;  // Debug output

        // Check if the device path matches USB devices
        if (IsUsbDevice(device_path)) {
            // Return the device path directly
            plugin->channel_->InvokeMethod("onDeviceDisconnected",
                                           std::make_unique<flutter::EncodableValue>(device_path));
        }
      }
    }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

}  // namespace flutter_usb_event
