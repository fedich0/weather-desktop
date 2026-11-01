// Runs from a temporary .app with the separate local.windowsnow.smoketest identifier.
#define main windowsnowApplicationMain
#include "../src/main.mm"
#undef main
#include <cassert>

static void pumpEvents() {
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.15]];
}

int main() {
    @autoreleasepool {
        NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
        assert([NSBundle.mainBundle.bundleIdentifier isEqual:@"local.windowsnow.smoketest"]);
        [defaults removePersistentDomainForName:@"local.windowsnow.smoketest"];
        NSApplication *application=NSApplication.sharedApplication;
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
        App *delegate=[App new];
        application.delegate=delegate;
        [delegate applicationDidFinishLaunching:nil];
        pumpEvents();
        ControlPanel *panel=delegate.controls;
        assert(panel.window.isVisible && panel.preview);
        assert(panel.inputs.count==7);
        assert(panel.keys.count>0);
        assert(panel.preview->scene->autumn);
        assert([panel.preview hitTest:NSMakePoint(50,50)]==nil);
        for (NSWindow *overlay in delegate.panels) assert(overlay.ignoresMouseEvents);

        // Exercise both rendering modes and the settings callbacks through real controls.
        NSArray *actions=@[@"Winter",@"Storm",@"Shake",@"Design",@"Flat",@"Frost",@"Liquid",@"Save",@"Fall",@"Recall",@"Weather"];
        for (NSString *name in actions) {
            BOOL found=NO;
            for (StudioKey *key in panel.keys) {
                if ([key.button.identifier isEqual:name]) {
                    [panel press:key.button];found=YES;break;
                }
            }
            assert(found);
            pumpEvents();
        }
        assert(!panel.preview->scene->autumn); // The saved profile contained winter.
        NSSwitch *ground=(NSSwitch *)panel.inputs[@"Ground"];
        ground.state=NSControlStateValueOff;[panel adjust:ground];
        assert(!panel.preview->scene->settings.ground);
        NSSlider *speed=(NSSlider *)panel.inputs[@"Speed"];
        speed.doubleValue=.5;[panel adjust:speed];
        assert(panel.preview->scene->settings.speed==.5f);

        [delegate displayStateChanged:[NSNotification notificationWithName:NSWorkspaceScreensDidSleepNotification object:nil]];
        assert(delegate.displaySleeping && delegate.timer.fireDate.timeIntervalSinceNow>60);
        [delegate displayStateChanged:[NSNotification notificationWithName:NSWorkspaceScreensDidWakeNotification object:nil]];
        assert(!delegate.displaySleeping);
        [panel.window close];
        [delegate applicationShouldHandleReopen:application hasVisibleWindows:YES];
        assert(panel.window.isVisible);
        [delegate applicationWillTerminate:nil];
        for(NSWindow *overlay in delegate.panels) [overlay orderOut:nil];
        [panel.window close];
        [defaults removePersistentDomainForName:@"local.windowsnow.smoketest"];
        puts("AppKit smoke checks passed.");
    }
}
