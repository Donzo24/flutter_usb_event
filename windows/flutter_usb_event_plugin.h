#ifndef FLUTTER_PLUGIN_FLUTTER_USB_EVENT_PLUGIN_H_
#define FLUTTER_PLUGIN_FLUTTER_USB_EVENT_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace flutter_usb_event {

class FlutterUsbEventPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  FlutterUsbEventPlugin();

  virtual ~FlutterUsbEventPlugin();

  // Disallow copy and assign.
  FlutterUsbEventPlugin(const FlutterUsbEventPlugin&) = delete;
  FlutterUsbEventPlugin& operator=(const FlutterUsbEventPlugin&) = delete;

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace flutter_usb_event

#endif  // FLUTTER_PLUGIN_FLUTTER_USB_EVENT_PLUGIN_H_
