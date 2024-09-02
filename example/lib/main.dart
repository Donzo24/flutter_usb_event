import 'package:flutter/material.dart';
import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_usb_event/flutter_usb_event.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
    String? _usbStatus;

    @override
    void initState() {
        super.initState();
        initPlatformState();
    }

    @override
    void dispose() {
        FlutterUsbEvent.stopListening();  // Arrêter l'écoute lors de la destruction du widget
        super.dispose();
    }

    // Platform messages are asynchronous, so we initialize in an async method.
    Future<void> initPlatformState() async {
        FlutterUsbEvent.startListening(
            onDeviceConnected: (deviceName) {
                if (deviceName.contains('ACR122U')) {
                    setState(() {
                        _usbStatus = 'ACR122U device connected: $deviceName';
                    });
                } else {
                    setState(() {
                        _usbStatus = 'Other device connected: $deviceName';
                    });
                }
            },
            onDeviceDisconnected: (deviceName) {
                if (deviceName.contains('ACR122U')) {
                    setState(() {
                        _usbStatus = 'ACR122U device disconnected: $deviceName';
                    });
                } else {
                    setState(() {
                        _usbStatus = 'Other device disconnected: $deviceName';
                    });
                }
            },
        );
    }

    @override
    Widget build(BuildContext context) {
        return MaterialApp(
            home: Scaffold(
                appBar: AppBar(
                    title: const Text('RFID Reader Event Listener'),
                ),
                body: Center(
                    child: Text(_usbStatus ?? 'No USB device detected.'),
                ),
            ),
        );
    }
}
