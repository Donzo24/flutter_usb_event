

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

#include <memory>
#include <sstream>

namespace {

class FlutterUsbEventPlugin : public flutter::Plugin {
	public:
	static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

	FlutterUsbEventPlugin(flutter::PluginRegistrarWindows* registrar);

	virtual ~FlutterUsbEventPlugin();

	private:
	// Méthode pour gérer les appels de méthode depuis Dart.
	void HandleMethodCall(const flutter::MethodCall<std::string>& method_call,
							std::unique_ptr<flutter::MethodResult<std::string>> result);

	// Gestion des messages de la fenêtre Windows.
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Identifiant de la fenêtre cachée pour les notifications de périphérique.
	HWND hwnd_;
	flutter::PluginRegistrarWindows* registrar_;
};

// Enregistre le plugin avec le registraire.
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

	// Demander des notifications de périphérique.
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
    // Commencer à écouter, la configuration est déjà faite dans le constructeur.
    result->Success();
  } else if (method_call.method_name().compare("stopListening") == 0) {
    // Arrêter l'écoute, pas de suppression spécifique à faire dans cette implémentation.
    result->Success();
  } else {
    result->NotImplemented();
  }
}

LRESULT CALLBACK FlutterUsbEventPlugin::WindowProc(HWND hwnd, UINT uMsg,
                                                          WPARAM wParam, LPARAM lParam) {
  if (uMsg == WM_CREATE) {
    // Associer la fenêtre à l'instance du plugin.
    SetWindowLongPtr(hwnd, GWLP_USERDATA,
                     reinterpret_cast<LONG_PTR>(
                         reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams));
  } else if (uMsg == WM_DEVICECHANGE) {
    auto plugin = reinterpret_cast<FlutterUsbEventPlugin*>(
        GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (plugin && wParam == DBT_DEVICEARRIVAL) {
      // Appareil connecté
      flutter::EncodableValue args("Lecteur connecté");
      plugin->registrar_->GetRegistrarMessenger()->Send(
          "flutter_usb_event", 0, &args);
    } else if (plugin && wParam == DBT_DEVICEREMOVECOMPLETE) {
      // Appareil déconnecté
      flutter::EncodableValue args("Lecteur déconnecté");
      plugin->registrar_->GetRegistrarMessenger()->Send(
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
