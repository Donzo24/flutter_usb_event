#include "flutter_usb_event_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <windows.h>
#include <dbt.h>
#include <initguid.h>  // Include this for GUID initialization
#include <usbioctl.h>  // Include this for USB device interface GUIDs
#include <memory>
#include <sstream>

// Utility function to convert std::wstring to std::string (UTF-8)
std::string ConvertWStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    // Get the size needed for the conversion
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    
    // Allocate a string of the required size
    std::string strTo(size_needed, 0);
    
    // Convert the wide string to UTF-8
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    
    return strTo;
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

  // Set up device notification filter
  DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
  ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
  NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
  NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
  NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE; // Filter for USB devices

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

        // Convert wide string to UTF-8 string
        std::string device_name = ConvertWStringToString(dev_interface->dbcc_name);

        plugin->channel_->InvokeMethod("onDeviceConnected",
                                       std::make_unique<flutter::EncodableValue>(device_name));
      }
    } else if (plugin && wParam == DBT_DEVICEREMOVECOMPLETE) {
      // Handle device disconnected
      plugin->channel_->InvokeMethod("onDeviceDisconnected",
                                     std::make_unique<flutter::EncodableValue>("Device Disconnected"));
    }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

}  // namespace flutter_usb_event
