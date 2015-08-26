//
//  keyboard_leds.c
//  LEDThing
//
//  Created by Alex Pawlowski on 8/26/15.
//  Copyright © 2015 Alex Pawlowski. All rights reserved.
//

#include "keyboard_leds.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDLib.h>
#include <fnmatch.h>

const int maxLeds = 3;
const char* ledNames[] = { "num", "caps", "scroll" };
const char* stateSymbol[] = {"-", "+" };
typedef enum { NoChange = -1, Off, On } LedState;

void setAllKeyboards(LedState changes[]);
CFMutableDictionaryRef getKeyboardDictionary();

Boolean verbose = false;
const char * nameMatch;

int manipulate_led(UInt32 whichLED, UInt32 value)
{
    LedState changes[] = { NoChange, NoChange, NoChange, NoChange };
    changes[whichLED] = value;
    setAllKeyboards(changes);
    return 0;
}

Boolean isKeyboardDevice(struct __IOHIDDevice *device)
{
    return IOHIDDeviceConformsTo(device, kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
}

void setKeyboard(struct __IOHIDDevice *device, CFDictionaryRef keyboardDictionary, LedState changes[])
{
    CFStringRef deviceNameRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
    if (!deviceNameRef) return;
    
    const char * deviceName = CFStringGetCStringPtr(deviceNameRef, kCFStringEncodingUTF8);
    
    if (nameMatch && fnmatch(nameMatch, deviceName, 0) != 0)
        return;
    
    if (verbose)
        printf("\n \"%s\" ", deviceName);
    
    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, keyboardDictionary, kIOHIDOptionsTypeNone);
    if (elements) {
        for (CFIndex elementIndex = 0; elementIndex < CFArrayGetCount(elements); elementIndex++) {
            IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, elementIndex);
            if (element && kHIDPage_LEDs == IOHIDElementGetUsagePage(element)) {
                uint32_t led = IOHIDElementGetUsage(element);
                
                if (led >= maxLeds) break;
                
                // Get current keyboard led status
                IOHIDValueRef currentValue = 0;
                IOHIDDeviceGetValue(device, element, &currentValue);
                long current = IOHIDValueGetIntegerValue(currentValue);
                CFRelease(currentValue);
                
                // Should we try to set the led?
                if (changes[led] != NoChange && changes[led] != current) {
                    IOHIDValueRef newValue = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, element, 0, changes[led]);
                    if (newValue) {
                        IOReturn changeResult = IOHIDDeviceSetValue(device, element, newValue);
                        
                        // Was the change successful?
                        if (kIOReturnSuccess == changeResult && verbose) {
                            printf("%s%s ", stateSymbol[changes[led]], ledNames[led - 1]);
                        }
                        CFRelease(newValue);
                    }
                } else if (verbose) {
                    printf("%s%s ", stateSymbol[current], ledNames[led - 1]);
                }
            }
        }
        CFRelease(elements);
    }
    
    if (verbose)
        printf("\n");
}

void setAllKeyboards(LedState changes[])
{
    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager) {
        fprintf(stderr, "Failed to create IOHID manager.\n");
        return;
    }
    
    CFDictionaryRef keyboard = getKeyboardDictionary();
    if (!keyboard) {
        fprintf(stderr, "Failed to get dictionary usage page for kHIDUsage_GD_Keyboard.\n");
        return;
    }
    
    IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
    IOHIDManagerSetDeviceMatching(manager, keyboard);
    
    CFSetRef devices = IOHIDManagerCopyDevices(manager);
    if (devices) {
        CFIndex deviceCount = CFSetGetCount(devices);
        if (deviceCount == 0) {
            fprintf(stderr, "Could not find any keyboard devices.\n");
        }
        else {
            // Loop through all keyboards attempting to get or display led state
            IOHIDDeviceRef *deviceRefs = malloc(sizeof(IOHIDDeviceRef) * deviceCount);
            if (deviceRefs) {
                CFSetGetValues(devices, (const void **) deviceRefs);
                for (CFIndex deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
                    if (isKeyboardDevice(deviceRefs[deviceIndex]))
                        setKeyboard(deviceRefs[deviceIndex], keyboard, changes);
                
                free(deviceRefs);
            }
        }
        
        CFRelease(devices);
    }
    
    CFRelease(keyboard);
}

CFMutableDictionaryRef getKeyboardDictionary()
{
    CFMutableDictionaryRef result = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    if (!result) return result;
    
    UInt32 inUsagePage = kHIDPage_GenericDesktop;
    UInt32 inUsage = kHIDUsage_GD_Keyboard;
    
    CFNumberRef page = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &inUsagePage);
    if (page) {
        CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), page);
        CFRelease(page);
        
        CFNumberRef usage = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &inUsage);
        if (usage) {
            CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), usage);
            CFRelease(usage);
        }
    }
    return result;
}