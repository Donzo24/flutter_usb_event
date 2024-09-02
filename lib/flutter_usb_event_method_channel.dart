import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'flutter_usb_event_platform_interface.dart';

/// An implementation of [FlutterUsbEventPlatform] that uses method channels.
class MethodChannelFlutterUsbEvent extends FlutterUsbEventPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel('flutter_usb_event');

  @override
  Future<String?> getPlatformVersion() async {
    final version = await methodChannel.invokeMethod<String>('getPlatformVersion');
    return version;
  }
}
