
import 'dart:io';

import 'package:flutter/services.dart';

import 'flutter_usb_event_platform_interface.dart';

class FlutterUsbEvent {
    
    static const MethodChannel _channel = MethodChannel('flutter_usb_event');

    Future<String?> getPlatformVersion() {
        return FlutterUsbEventPlatform.instance.getPlatformVersion();
    }

        /// Fonction pour démarrer l'écoute des événements de connexion et de déconnexion USB.
    static Future<void> startListening({
        required Function(String) onDeviceConnected,
        required Function(String) onDeviceDisconnected,
    }) async {
        // Définir le gestionnaire d'appel pour les événements depuis le natif
        _channel.setMethodCallHandler((call) async {
            if (call.method == 'onDeviceConnected') {
                onDeviceConnected(call.arguments as String);
            } else if (call.method == 'onDeviceDisconnected') {
                onDeviceDisconnected(call.arguments as String);
            }
        });

        // Appeler la méthode native pour commencer à écouter
        try {
            if (Platform.isWindows || Platform.isMacOS) {
                await _channel.invokeMethod('startListening');
            } else {
                print("La plateforme actuelle n'est pas supportée pour l'écoute USB.");
            }
        } on PlatformException catch (e) {
            print("Erreur lors du démarrage de l'écoute des périphériques USB : ${e.message}");
        }
    }

    /// Fonction pour arrêter l'écoute des événements USB.
    static Future<void> stopListening() async {
        try {
            if (Platform.isWindows || Platform.isMacOS) {
                await _channel.invokeMethod('stopListening');
            }
            await _channel.invokeMethod('stopListening');
        } on PlatformException catch (e) {
            print("Erreur lors de l'arrêt de l'écoute des périphériques USB : ${e.message}");
        }
    }
}
