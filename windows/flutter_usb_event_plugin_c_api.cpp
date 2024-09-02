#include "include/flutter_usb_event/flutter_usb_event_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "flutter_usb_event_plugin.h"

void FlutterUsbEventPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  flutter_usb_event::FlutterUsbEventPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
