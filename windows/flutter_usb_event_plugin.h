#ifndef FLUTTER_PLUGIN_FLUTTER_USB_EVENT_PLUGIN_H_
#define FLUTTER_PLUGIN_FLUTTER_USB_EVENT_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <windows.h>
#include <memory>

namespace flutter_usb_event {

class FlutterUsbEventPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);
  explicit FlutterUsbEventPlugin(flutter::PluginRegistrarWindows* registrar);
  virtual ~FlutterUsbEventPlugin();

 private:
  // Method to handle method calls from Dart.
  void HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue>& method_call,
                        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  // Add a friend declaration for the test class to access private members
  friend class FlutterUsbEventPluginTest;

  // Declare the WindowProc as static so it can be used as a callback function
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  HWND hwnd_;
  flutter::PluginRegistrarWindows* registrar_;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
};

}  // namespace flutter_usb_event

#endif  // FLUTTER_PLUGIN_FLUTTER_USB_EVENT_PLUGIN_H_
