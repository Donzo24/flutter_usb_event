import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'flutter_usb_event_method_channel.dart';

abstract class FlutterUsbEventPlatform extends PlatformInterface {
  /// Constructs a FlutterUsbEventPlatform.
  FlutterUsbEventPlatform() : super(token: _token);

  static final Object _token = Object();

  static FlutterUsbEventPlatform _instance = MethodChannelFlutterUsbEvent();

  /// The default instance of [FlutterUsbEventPlatform] to use.
  ///
  /// Defaults to [MethodChannelFlutterUsbEvent].
  static FlutterUsbEventPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [FlutterUsbEventPlatform] when
  /// they register themselves.
  static set instance(FlutterUsbEventPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<String?> getPlatformVersion() {
    throw UnimplementedError('platformVersion() has not been implemented.');
  }
}
