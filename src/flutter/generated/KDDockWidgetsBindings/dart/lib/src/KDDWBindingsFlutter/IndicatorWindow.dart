/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/
import 'dart:ffi' as ffi;
import 'package:ffi/ffi.dart';
import '../TypeHelpers.dart';
import '../../Bindings.dart';
import '../../Bindings_KDDWBindingsCore.dart' as KDDWBindingsCore;
import '../../Bindings_KDDWBindingsFlutter.dart' as KDDWBindingsFlutter;
import '../../LibraryLoader.dart';
import '../../FinalizerHelpers.dart';

var _dylib = Library.instance().dylib;

class IndicatorWindow
    extends KDDWBindingsCore.ClassicIndicatorWindowViewInterface {
  IndicatorWindow.fromCppPointer(var cppPointer, [var needsAutoDelete = false])
      : super.fromCppPointer(cppPointer, needsAutoDelete) {}
  IndicatorWindow.init() : super.init() {}
  factory IndicatorWindow.fromCache(var cppPointer, [needsAutoDelete = false]) {
    if (KDDWBindingsCore.ClassicIndicatorWindowViewInterface.isCached(
        cppPointer)) {
      var instance = KDDWBindingsCore.ClassicIndicatorWindowViewInterface
          .s_dartInstanceByCppPtr[cppPointer.address];
      if (instance != null) return instance as IndicatorWindow;
    }
    return IndicatorWindow.fromCppPointer(cppPointer, needsAutoDelete);
  } //IndicatorWindow(KDDockWidgets::Core::ClassicIndicators * classicIndicators)
  IndicatorWindow(KDDWBindingsCore.ClassicIndicators? classicIndicators)
      : super.init() {
    final voidstar_Func_voidstar func = _dylib
        .lookup<ffi.NativeFunction<voidstar_Func_voidstar_FFI>>(
            'c_KDDockWidgets__flutter__IndicatorWindow__constructor_ClassicIndicators')
        .asFunction();
    thisCpp = func(
        classicIndicators == null ? ffi.nullptr : classicIndicators.thisCpp);
    KDDWBindingsCore.ClassicIndicatorWindowViewInterface
        .s_dartInstanceByCppPtr[thisCpp.address] = this;
    registerCallbacks();
  }
  static int hover_calledFromC(
      ffi.Pointer<void> thisCpp, ffi.Pointer<void> globalPos) {
    var dartInstance = KDDWBindingsCore.ClassicIndicatorWindowViewInterface
        .s_dartInstanceByCppPtr[thisCpp.address] as IndicatorWindow;
    if (dartInstance == null) {
      print(
          "Dart instance not found for IndicatorWindow::hover(QPoint globalPos)! (${thisCpp.address})");
      throw Error();
    }
    final result = dartInstance.hover(QPoint.fromCppPointer(globalPos));
    return result;
  }

  static int isWindow_calledFromC(ffi.Pointer<void> thisCpp) {
    var dartInstance = KDDWBindingsCore.ClassicIndicatorWindowViewInterface
        .s_dartInstanceByCppPtr[thisCpp.address] as IndicatorWindow;
    if (dartInstance == null) {
      print(
          "Dart instance not found for IndicatorWindow::isWindow() const! (${thisCpp.address})");
      throw Error();
    }
    final result = dartInstance.isWindow();
    return result ? 1 : 0;
  }

  static ffi.Pointer<void> posForIndicator_calledFromC(
      ffi.Pointer<void> thisCpp, int arg__1) {
    var dartInstance = KDDWBindingsCore.ClassicIndicatorWindowViewInterface
        .s_dartInstanceByCppPtr[thisCpp.address] as IndicatorWindow;
    if (dartInstance == null) {
      print(
          "Dart instance not found for IndicatorWindow::posForIndicator(KDDockWidgets::DropLocation arg__1) const! (${thisCpp.address})");
      throw Error();
    }
    final result = dartInstance.posForIndicator(arg__1);
    return result.thisCpp;
  }

  static void raise_calledFromC(ffi.Pointer<void> thisCpp) {
    var dartInstance = KDDWBindingsCore.ClassicIndicatorWindowViewInterface
        .s_dartInstanceByCppPtr[thisCpp.address] as IndicatorWindow;
    if (dartInstance == null) {
      print(
          "Dart instance not found for IndicatorWindow::raise()! (${thisCpp.address})");
      throw Error();
    }
    dartInstance.raise();
  }

  static void resize_calledFromC(
      ffi.Pointer<void> thisCpp, ffi.Pointer<void> arg__1) {
    var dartInstance = KDDWBindingsCore.ClassicIndicatorWindowViewInterface
        .s_dartInstanceByCppPtr[thisCpp.address] as IndicatorWindow;
    if (dartInstance == null) {
      print(
          "Dart instance not found for IndicatorWindow::resize(QSize arg__1)! (${thisCpp.address})");
      throw Error();
    }
    dartInstance.resize(QSize.fromCppPointer(arg__1));
  }

  static void setGeometry_calledFromC(
      ffi.Pointer<void> thisCpp, ffi.Pointer<void> arg__1) {
    var dartInstance = KDDWBindingsCore.ClassicIndicatorWindowViewInterface
        .s_dartInstanceByCppPtr[thisCpp.address] as IndicatorWindow;
    if (dartInstance == null) {
      print(
          "Dart instance not found for IndicatorWindow::setGeometry(QRect arg__1)! (${thisCpp.address})");
      throw Error();
    }
    dartInstance.setGeometry(QRect.fromCppPointer(arg__1));
  }

  static void setObjectName_calledFromC(
      ffi.Pointer<void> thisCpp, ffi.Pointer<void>? arg__1) {
    var dartInstance = KDDWBindingsCore.ClassicIndicatorWindowViewInterface
        .s_dartInstanceByCppPtr[thisCpp.address] as IndicatorWindow;
    if (dartInstance == null) {
      print(
          "Dart instance not found for IndicatorWindow::setObjectName(const QString & arg__1)! (${thisCpp.address})");
      throw Error();
    }
    dartInstance.setObjectName(QString.fromCppPointer(arg__1).toDartString());
  }

  static void setVisible_calledFromC(ffi.Pointer<void> thisCpp, int arg__1) {
    var dartInstance = KDDWBindingsCore.ClassicIndicatorWindowViewInterface
        .s_dartInstanceByCppPtr[thisCpp.address] as IndicatorWindow;
    if (dartInstance == null) {
      print(
          "Dart instance not found for IndicatorWindow::setVisible(bool arg__1)! (${thisCpp.address})");
      throw Error();
    }
    dartInstance.setVisible(arg__1 != 0);
  }

  static void updatePositions_calledFromC(ffi.Pointer<void> thisCpp) {
    var dartInstance = KDDWBindingsCore.ClassicIndicatorWindowViewInterface
        .s_dartInstanceByCppPtr[thisCpp.address] as IndicatorWindow;
    if (dartInstance == null) {
      print(
          "Dart instance not found for IndicatorWindow::updatePositions()! (${thisCpp.address})");
      throw Error();
    }
    dartInstance.updatePositions();
  }

  void release() {
    final void_Func_voidstar func = _dylib
        .lookup<ffi.NativeFunction<void_Func_voidstar_FFI>>(
            'c_KDDockWidgets__flutter__IndicatorWindow__destructor')
        .asFunction();
    func(thisCpp);
  }

  String cFunctionSymbolName(int methodId) {
    switch (methodId) {
      case 1796:
        return "c_KDDockWidgets__flutter__IndicatorWindow__hover_QPoint";
      case 1797:
        return "c_KDDockWidgets__flutter__IndicatorWindow__isWindow";
      case 1798:
        return "c_KDDockWidgets__flutter__IndicatorWindow__posForIndicator_DropLocation";
      case 1799:
        return "c_KDDockWidgets__flutter__IndicatorWindow__raise";
      case 1800:
        return "c_KDDockWidgets__flutter__IndicatorWindow__resize_QSize";
      case 1801:
        return "c_KDDockWidgets__flutter__IndicatorWindow__setGeometry_QRect";
      case 1802:
        return "c_KDDockWidgets__flutter__IndicatorWindow__setObjectName_QString";
      case 1803:
        return "c_KDDockWidgets__flutter__IndicatorWindow__setVisible_bool";
      case 1804:
        return "c_KDDockWidgets__flutter__IndicatorWindow__updatePositions";
    }
    return super.cFunctionSymbolName(methodId);
  }

  static String methodNameFromId(int methodId) {
    switch (methodId) {
      case 1796:
        return "hover";
      case 1797:
        return "isWindow";
      case 1798:
        return "posForIndicator";
      case 1799:
        return "raise";
      case 1800:
        return "resize";
      case 1801:
        return "setGeometry";
      case 1802:
        return "setObjectName";
      case 1803:
        return "setVisible";
      case 1804:
        return "updatePositions";
    }
    throw Error();
  }

  void registerCallbacks() {
    assert(thisCpp != null);
    final RegisterMethodIsReimplementedCallback registerCallback = _dylib
        .lookup<ffi.NativeFunction<RegisterMethodIsReimplementedCallback_FFI>>(
            'c_KDDockWidgets__flutter__IndicatorWindow__registerVirtualMethodCallback')
        .asFunction();
    const callbackExcept1796 = 0;
    final callback1796 =
        ffi.Pointer.fromFunction<int_Func_voidstar_voidstar_FFI>(
            KDDWBindingsFlutter.IndicatorWindow.hover_calledFromC,
            callbackExcept1796);
    registerCallback(thisCpp, callback1796, 1796);
    const callbackExcept1797 = 0;
    final callback1797 = ffi.Pointer.fromFunction<bool_Func_voidstar_FFI>(
        KDDWBindingsFlutter.IndicatorWindow.isWindow_calledFromC,
        callbackExcept1797);
    registerCallback(thisCpp, callback1797, 1797);
    final callback1798 =
        ffi.Pointer.fromFunction<voidstar_Func_voidstar_ffi_Int32_FFI>(
            KDDWBindingsFlutter.IndicatorWindow.posForIndicator_calledFromC);
    registerCallback(thisCpp, callback1798, 1798);
    final callback1799 = ffi.Pointer.fromFunction<void_Func_voidstar_FFI>(
        KDDWBindingsFlutter.IndicatorWindow.raise_calledFromC);
    registerCallback(thisCpp, callback1799, 1799);
    final callback1800 =
        ffi.Pointer.fromFunction<void_Func_voidstar_voidstar_FFI>(
            KDDWBindingsFlutter.IndicatorWindow.resize_calledFromC);
    registerCallback(thisCpp, callback1800, 1800);
    final callback1801 =
        ffi.Pointer.fromFunction<void_Func_voidstar_voidstar_FFI>(
            KDDWBindingsFlutter.IndicatorWindow.setGeometry_calledFromC);
    registerCallback(thisCpp, callback1801, 1801);
    final callback1802 =
        ffi.Pointer.fromFunction<void_Func_voidstar_voidstar_FFI>(
            KDDWBindingsFlutter.IndicatorWindow.setObjectName_calledFromC);
    registerCallback(thisCpp, callback1802, 1802);
    final callback1803 =
        ffi.Pointer.fromFunction<void_Func_voidstar_ffi_Int8_FFI>(
            KDDWBindingsFlutter.IndicatorWindow.setVisible_calledFromC);
    registerCallback(thisCpp, callback1803, 1803);
    final callback1804 = ffi.Pointer.fromFunction<void_Func_voidstar_FFI>(
        KDDWBindingsFlutter.IndicatorWindow.updatePositions_calledFromC);
    registerCallback(thisCpp, callback1804, 1804);
  }
}