// import Cocoa
// import FlutterMacOS

// public class FlutterUsbEventPlugin: NSObject, FlutterPlugin {
//   public static func register(with registrar: FlutterPluginRegistrar) {
//     let channel = FlutterMethodChannel(name: "flutter_usb_event", binaryMessenger: registrar.messenger)
//     let instance = FlutterUsbEventPlugin()
//     registrar.addMethodCallDelegate(instance, channel: channel)
//   }

//   public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
//     switch call.method {
//     case "getPlatformVersion":
//       result("macOS " + ProcessInfo.processInfo.operatingSystemVersionString)
//     default:
//       result(FlutterMethodNotImplemented)
//     }
//   }
// }

import Cocoa
import FlutterMacOS
import IOKit
import IOKit.usb

public class FlutterUsbEventPlugin: NSObject, FlutterPlugin {
    var channel: FlutterMethodChannel?  // Retirer 'private' pour permettre l'accès à partir des callbacks

    private var notificationPort: IONotificationPortRef?
    private var addedIterator: io_iterator_t = 0
    private var removedIterator: io_iterator_t = 0

    public static func register(with registrar: FlutterPluginRegistrar) {
        let channel = FlutterMethodChannel(name: "flutter_usb_event", binaryMessenger: registrar.messenger)
        let instance = FlutterUsbEventPlugin()
        instance.channel = channel
        registrar.addMethodCallDelegate(instance, channel: channel)
        instance.startListening()
    }

    public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        if call.method == "startListening" {
            startListening()
            result("Listening started")
        } else {
            result(FlutterMethodNotImplemented)
        }
    }

    private func startListening() {
        // Créer une correspondance de dictionnaire pour les périphériques USB
        guard let matchingDict = IOServiceMatching(kIOUSBDeviceClassName) else {
            print("Erreur : IOServiceMatching a renvoyé NULL.")
            return
        }

        // Créer un port de notification
        notificationPort = IONotificationPortCreate(kIOMasterPortDefault)
        guard let notificationPort = notificationPort else {
            print("Erreur : IONotificationPortCreate a renvoyé NULL.")
            return
        }

        let runLoopSource = IONotificationPortGetRunLoopSource(notificationPort).takeUnretainedValue()
        CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, CFRunLoopMode.defaultMode)

        let selfPointer = UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque())

        // Configurer l'observateur pour les périphériques ajoutés
        let addResult = IOServiceAddMatchingNotification(notificationPort,
                                                         kIOFirstMatchNotification,
                                                         matchingDict,
                                                         deviceAddedCallback,
                                                         selfPointer,
                                                         &addedIterator)

        if addResult != KERN_SUCCESS {
            print("Erreur : IOServiceAddMatchingNotification (ajout) a échoué.")
            return
        }

        // Appeler la fonction de rappel pour les périphériques déjà connectés
        deviceAddedCallback(refcon: selfPointer, iterator: addedIterator)

        // Configurer l'observateur pour les périphériques retirés
        let removeResult = IOServiceAddMatchingNotification(notificationPort,
                                                            kIOTerminatedNotification,
                                                            matchingDict,
                                                            deviceRemovedCallback,
                                                            selfPointer,
                                                            &removedIterator)

        if removeResult != KERN_SUCCESS {
            print("Erreur : IOServiceAddMatchingNotification (retrait) a échoué.")
            return
        }

        // Appeler la fonction de rappel pour les périphériques déjà retirés
        deviceRemovedCallback(refcon: selfPointer, iterator: removedIterator)
    }

    private func stopListening() {
        if addedIterator != 0 {
            IOObjectRelease(addedIterator)
            addedIterator = 0
        }

        if removedIterator != 0 {
            IOObjectRelease(removedIterator)
            removedIterator = 0
        }

        if let port = notificationPort {
            IONotificationPortDestroy(port)
            notificationPort = nil
        }
    }
}

func deviceAddedCallback(refcon: UnsafeMutableRawPointer?, iterator: io_iterator_t) {
    let plugin = Unmanaged<FlutterUsbEventPlugin>.fromOpaque(refcon!).takeUnretainedValue()
    var usbDevice: io_object_t

    while true {
        usbDevice = IOIteratorNext(iterator)
        if usbDevice == 0 {
            break
        }

		// Fetch the device name from the IORegistry
        if let deviceName = IORegistryEntryCreateCFProperty(usbDevice, kUSBProductString as CFString, kCFAllocatorDefault, 0)?.takeUnretainedValue() as? String {
            plugin.channel?.invokeMethod("onDeviceConnected", arguments: deviceName)
        } else {
            plugin.channel?.invokeMethod("onDeviceConnected", arguments: "Unknown device")
        }

        // plugin.channel?.invokeMethod("onDeviceConnected", arguments: "Lecteur connecté")
        IOObjectRelease(usbDevice)
    }
}

func deviceRemovedCallback(refcon: UnsafeMutableRawPointer?, iterator: io_iterator_t) {
    let plugin = Unmanaged<FlutterUsbEventPlugin>.fromOpaque(refcon!).takeUnretainedValue()
    var usbDevice: io_object_t

    while true {
        usbDevice = IOIteratorNext(iterator)
        if usbDevice == 0 {
            break
        }

		// Fetch the device name from the IORegistry
        if let deviceName = IORegistryEntryCreateCFProperty(usbDevice, kUSBProductString as CFString, kCFAllocatorDefault, 0)?.takeUnretainedValue() as? String {
            plugin.channel?.invokeMethod("onDeviceDisconnected", arguments: deviceName)
        } else {
            plugin.channel?.invokeMethod("onDeviceDisconnected", arguments: "Unknown device")
        }

        // plugin.channel?.invokeMethod("onDeviceDisconnected", arguments: "Lecteur déconnecté")
        IOObjectRelease(usbDevice)
    }
}

