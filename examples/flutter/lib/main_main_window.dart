/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

import 'package:KDDockWidgets/DockWidget.dart';
import 'package:KDDockWidgets/MainWindow.dart';
import 'package:flutter/material.dart';
import 'package:KDDockWidgets/WindowOverlayWidget.dart' as KDDW;
import 'package:KDDockWidgets/Platform.dart' as KDDW;
import 'package:KDDockWidgetsBindings/Bindings.dart' as KDDWBindings;

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demos',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: const MyHomePage(title: 'Flutter Demo Home Page'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({Key? key, required this.title}) : super(key: key);

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  late MainWindow mainWindow;

  _MyHomePageState() {
    KDDW.Platform();
    mainWindow = MainWindow("myWindow1",
        options:
            KDDWBindings.KDDockWidgets_MainWindowOption.MainWindowOption_None,
        parent: null);

    mainWindow.m_fillsParent = true;

    final dw1 = DockWidget("dw1");
    final dw2 = DockWidget("dw2");
    final dw3 = DockWidget("dw3");

    mainWindow.m_controller.addDockWidget(
        dw1.dockWidget(), KDDWBindings.KDDockWidgets_Location.Location_OnRight,
        relativeTo: null, initialOption: KDDWBindings.InitialOption());

    mainWindow.m_controller.addDockWidget(
        dw2.dockWidget(), KDDWBindings.KDDockWidgets_Location.Location_OnLeft,
        relativeTo: null, initialOption: KDDWBindings.InitialOption());

    dw3.dockWidget().setFloating(true);
    dw3.dockWidget().open();
  }

  void _incrementCounter() {}

  @override
  Widget build(BuildContext context) {
    final windowOverlay = KDDW.WindowOverlayWidget();

    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: Center(
          child: Stack(children: [mainWindow.flutterWidget, windowOverlay])),
      floatingActionButton: FloatingActionButton(
        onPressed: _incrementCounter,
        tooltip: 'Increment',
        child: const Icon(Icons.add),
      ),
    );
  }
}