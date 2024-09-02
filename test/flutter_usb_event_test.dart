import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_usb_event/flutter_usb_event.dart';
import 'package:flutter_usb_event/flutter_usb_event_platform_interface.dart';
import 'package:flutter_usb_event/flutter_usb_event_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockFlutterUsbEventPlatform
    with MockPlatformInterfaceMixin
    implements FlutterUsbEventPlatform {

  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final FlutterUsbEventPlatform initialPlatform = FlutterUsbEventPlatform.instance;

  test('$MethodChannelFlutterUsbEvent is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelFlutterUsbEvent>());
  });

  test('getPlatformVersion', () async {
    FlutterUsbEvent flutterUsbEventPlugin = FlutterUsbEvent();
    MockFlutterUsbEventPlatform fakePlatform = MockFlutterUsbEventPlatform();
    FlutterUsbEventPlatform.instance = fakePlatform;

    expect(await flutterUsbEventPlugin.getPlatformVersion(), '42');
  });
}
