//
//  AppDelegate.swift
//  CAPS
//
//  Created by Alex Pawlowski on 8/26/15.
//  Copyright © 2015 Alex Pawlowski. All rights reserved.
//
import Foundation
import Cocoa
import Carbon
import IOKit.hid

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {
    
    @IBOutlet weak var menu: NSMenu!
    @IBOutlet weak var statusMenuItem: NSMenuItem!

    var statusItem: NSStatusItem!
    var enabled: Bool = false
    
    func applicationDidFinishLaunching(aNotification: NSNotification) {
        statusItem = NSStatusBar.systemStatusBar().statusItemWithLength(23.0)
        statusItem.image = NSImage(named: "off")
        statusItem.menu = menu
        statusItem.highlightMode = true
        
        NSDistributedNotificationCenter
            .defaultCenter()
            .addObserver(self,
                selector: "languageChanged:",
                name: "AppleSelectedInputSourcesChangedNotification",
                object: nil)
        
        languageChanged(nil)
    }
    
    func languageChanged(notification: NSNotification?) {
        let source = TISCopyCurrentKeyboardInputSource()
        let raw = TISGetInputSourceProperty(source.takeUnretainedValue(), kTISPropertyInputSourceLanguages);
        if let current = unsafeBitCast(raw, NSArray.self).firstObject as? String {
            switch (current) {
            case "en":
                manipulate_led(UInt32(kHIDUsage_LED_CapsLock), UInt32(1))
                enabled = true
                statusMenuItem.title = "On"
                statusItem.image = NSImage(named: "on")
            default:
                manipulate_led(UInt32(kHIDUsage_LED_CapsLock), UInt32(0))
                enabled = false
                statusMenuItem.title = "Off"
                statusItem.image = NSImage(named: "off")
            }
        }
    }
    
    deinit {
        NSDistributedNotificationCenter.defaultCenter().removeObserver(self)
    }
}

