

// // This must be included before many other Windows headers.
// #include <windows.h>

// // For getPlatformVersion; remove unless needed for your plugin implementation.
// #include <VersionHelpers.h>

// #include <flutter/method_channel.h>
// #include <flutter/plugin_registrar_windows.h>
// #include <flutter/standard_method_codec.h>

// #include <memory>
// #include <sstream>

// namespace flutter_usb_event {

// // static
// void FlutterUsbEventPlugin::RegisterWithRegistrar(
//     flutter::PluginRegistrarWindows *registrar) {
//   auto channel =
//       std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
//           registrar->messenger(), "flutter_usb_event",
//           &flutter::StandardMethodCodec::GetInstance());

//   auto plugin = std::make_unique<FlutterUsbEventPlugin>();

//   channel->SetMethodCallHandler(
//       [plugin_pointer = plugin.get()](const auto &call, auto result) {
//         plugin_pointer->HandleMethodCall(call, std::move(result));
//       });

//   registrar->AddPlugin(std::move(plugin));
// }

// FlutterUsbEventPlugin::FlutterUsbEventPlugin() {}

// FlutterUsbEventPlugin::~FlutterUsbEventPlugin() {}

// void FlutterUsbEventPlugin::HandleMethodCall(
//     const flutter::MethodCall<flutter::EncodableValue> &method_call,
//     std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
//   if (method_call.method_name().compare("getPlatformVersion") == 0) {
//     std::ostringstream version_stream;
//     version_stream << "Windows ";
//     if (IsWindows10OrGreater()) {
//       version_stream << "10+";
//     } else if (IsWindows8OrGreater()) {
//       version_stream << "8";
//     } else if (IsWindows7OrGreater()) {
//       version_stream << "7";
//     }
//     result->Success(flutter::EncodableValue(version_stream.str()));
//   } else {
//     result->NotImplemented();
//   }
// }

// }  // namespace flutter_usb_event

#include "flutter_usb_event_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <windows.h>
#include <dbt.h>
#include <SetupAPI.h>
#include <string>
#include <iostream>

#include <memory>
#include <sstream>

namespace {

class FlutterUsbEventPlugin : public flutter::Plugin {
public:
    static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

    FlutterUsbEventPlugin(flutter::PluginRegistrarWindows* registrar);

    virtual ~FlutterUsbEventPlugin();

private:
    // Method to handle method calls from Dart.
    void HandleMethodCall(const flutter::MethodCall<std::string>& method_call,
                          std::unique_ptr<flutter::MethodResult<std::string>> result);

    // Window procedure to handle Windows messages.
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Hidden window identifier for device notifications.
    HWND hwnd_;
    flutter::PluginRegistrarWindows* registrar_;
};

// Register the plugin with the registrar.
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

    // Request device notifications.
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

    RegisterDeviceNotification(hwnd_, &NotificationFilter,
                               DEVICE_NOTIFY_WINDOW_HANDLE);
}

FlutterUsbEventPlugin::~FlutterUsbEventPlugin() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
}

void FlutterUsbEventPlugin::HandleMethodCall(
    const flutter::MethodCall<std::string>& method_call,
    std::unique_ptr<flutter::MethodResult<std::string>> result) {
    if (method_call.method_name().compare("startListening") == 0) {
        // Start listening, already configured in constructor.
        result->Success();
    } else if (method_call.method_name().compare("stopListening") == 0) {
        // Stop listening, no specific removal needed in this implementation.
        result->Success();
    } else {
        result->NotImplemented();
    }
}

// Function to get the device name.
std::string GetDeviceName(LPARAM lParam) {
    PDEV_BROADCAST_DEVICEINTERFACE deviceInterface = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(lParam);
    HDEVINFO deviceInfo = SetupDiGetClassDevs(&deviceInterface->dbcc_classguid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (deviceInfo == INVALID_HANDLE_VALUE) {
        return "Unknown device";
    }

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    
    if (!SetupDiEnumDeviceInterfaces(deviceInfo, NULL, &deviceInterface->dbcc_classguid, 0, &deviceInterfaceData)) {
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return "Unknown device";
    }

    DWORD requiredSize = 0;
    SetupDiGetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
    deviceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (!SetupDiGetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, deviceDetail, requiredSize, NULL, &devInfoData)) {
        free(deviceDetail);
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return "Unknown device";
    }

    CHAR deviceName[256];
    if (SetupDiGetDeviceRegistryProperty(deviceInfo, &devInfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)deviceName, sizeof(deviceName), NULL)) {
        std::string name(deviceName);
        free(deviceDetail);
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return name;
    }

    free(deviceDetail);
    SetupDiDestroyDeviceInfoList(deviceInfo);
    return "Unknown device";
}

LRESULT CALLBACK FlutterUsbEventPlugin::WindowProc(HWND hwnd, UINT uMsg,
                                                   WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CREATE) {
        // Associate the window with the plugin instance.
        SetWindowLongPtr(hwnd, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(
                             reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams));
    } else if (uMsg == WM_DEVICECHANGE) {
        auto plugin = reinterpret_cast<FlutterUsbEventPlugin*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if (plugin && wParam == DBT_DEVICEARRIVAL) {
            // Device connected
            std::string deviceName = GetDeviceName(lParam);
            flutter::EncodableValue args("Connected: " + deviceName);
            plugin->registrar_->GetMessenger()->Send(
                "flutter_usb_event", 0, &args);
        } else if (plugin && wParam == DBT_DEVICEREMOVECOMPLETE) {
            // Device disconnected
            std::string deviceName = GetDeviceName(lParam);
            flutter::EncodableValue args("Disconnected: " + deviceName);
            plugin->registrar_->GetMessenger()->Send(
                "flutter_usb_event", 0, &args);
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

}  // namespace

void FlutterUsbEventPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
    FlutterUsbEventPlugin::RegisterWithRegistrar(
        flutter::PluginRegistrarManager::GetInstance()
            ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
