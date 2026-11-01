#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>
#include <memory>
#include <unistd.h>
#include "Powder.hpp"
#include <cstring>

struct BankCache {
    std::vector<float> depth;
    std::vector<uint8_t> visible;
    size_t count=0;
    uint64_t revision=0, contentRevision=0;
    int start=-1, end=-1;
    NSImage *image=nil;
};

static snow::Settings preferences() {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    return {snow::bounded([d floatForKey:@"Speed"],.4f,2.5f,1),snow::bounded([d floatForKey:@"Accumulation"],0.f,1.f,1),
            snow::bounded([d floatForKey:@"Wind"],-80.f,80.f,0), bool([d boolForKey:@"Ground"]), snow::bounded([d floatForKey:@"Density"],.2f,2.5f,1)};
}
@interface SnowView : NSView {
@public
    std::unique_ptr<snow::Scene> scene;
    std::map<uint32_t,BankCache> bankCache;
    NSImage *sprites[3];
    NSImage *foliage[6];
}
@end

@implementation SnowView
- (BOOL)isFlipped { return YES; }
- (NSView *)hitTest:(NSPoint)point { return nil; }
- (BOOL)isOpaque { return NO; }
- (instancetype)initWithFrame:(NSRect)r {
    if ((self = [super initWithFrame:r])) {
        for (int i=0;i<3;++i) {
            NSString *name=[NSString stringWithFormat:@"flake%d",i];
            sprites[i]=[[NSImage alloc] initWithContentsOfURL:[NSBundle.mainBundle URLForResource:name withExtension:@"png"]];
        }
        for (int i=0;i<6;++i) {
            NSString *name=[NSString stringWithFormat:@"leaf%d",i];
            foliage[i]=[[NSImage alloc] initWithContentsOfURL:[NSBundle.mainBundle URLForResource:name withExtension:@"png"]];
        }
        scene = std::make_unique<snow::Scene>(r.size.width, r.size.height, arc4random(),
            [[NSUserDefaults standardUserDefaults] boolForKey:@"Autumn"]);
        scene->configure(preferences());
    }
    return self;
}
- (void)drawBank:(const snow::Bank &)b {
    int start=std::max(0,int(std::floor(-b.frame.x)));
    int end=std::min(int(std::ceil(b.frame.w)),int(std::ceil(NSWidth(self.bounds)-b.frame.x)));
    if (end<=start || b.frame.y < -7 || b.frame.y-snow::sky>NSHeight(self.bounds)) return;
    std::vector<uint8_t> visible(b.depth.size(),0);
    for (int i=start/8; i<=std::min(int(b.depth.size())-1,(end-1)/8); ++i)
        visible[i]=!b.frame.id || scene->exposed(b.frame.id,b.frame.x+i*8+4,b.frame.y);
    auto &cache=bankCache[b.frame.id];
    if (cache.depth!=b.depth || cache.visible!=visible || cache.start!=start || cache.end!=end) {
        cache.depth=b.depth; cache.visible=visible; cache.start=start; cache.end=end;
        auto strip=snow::paintPowder(b,start,end,visible);
        NSBitmapImageRep *rep=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
            pixelsWide:strip.width pixelsHigh:snow::stripHeight bitsPerSample:8 samplesPerPixel:4
            hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
            bitmapFormat:NSBitmapFormatAlphaNonpremultiplied bytesPerRow:strip.width*4 bitsPerPixel:32];
        if (rep) {
            memcpy(rep.bitmapData,strip.rgba.data(),strip.rgba.size());
            cache.image=[[NSImage alloc] initWithSize:NSMakeSize(end-start,snow::stripHeight/2)];
            [cache.image addRepresentation:rep];
        }
    }
    [cache.image drawInRect:NSMakeRect(b.frame.x+start,b.frame.y-snow::sky,end-start,snow::stripHeight/2)
        fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
}
- (void)paintLeaf:(snow::LeafPose)leaf {
    CGContextRef c=NSGraphicsContext.currentContext.CGContext;
    CGContextSaveGState(c);
    CGContextTranslateCTM(c,leaf.x,leaf.y);
    CGContextRotateCTM(c,leaf.angle);
    CGFloat width=leaf.size*leaf.squash;
    [foliage[leaf.sprite] drawInRect:NSMakeRect(-width/2,-leaf.size/2,width,leaf.size)
        fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
    CGContextRestoreGState(c);
}
- (void)drawLeafBank:(const snow::Bank &)b {
    if (b.leaves.empty()) { bankCache.erase(b.frame.id); return; }
    CGFloat x=std::max<CGFloat>(0,b.frame.x-16), y=b.frame.y-42;
    CGFloat end=std::min<CGFloat>(NSWidth(self.bounds),b.frame.x+b.frame.w+16);
    if(end<=x || y>=NSHeight(self.bounds) || y+56<=0) return;
    auto &cache=bankCache[b.frame.id];
    uint64_t revision=b.frame.id?scene->geometryVersion:0;
    if(!cache.image || cache.revision!=revision || cache.contentRevision!=b.revision || cache.count>b.leaves.size()) {
        cache.image=[[NSImage alloc] initWithSize:NSMakeSize(end-x,56)];
        cache.count=0; cache.revision=revision; cache.contentRevision=b.revision;
    }
    if(cache.count<b.leaves.size()) {
        [cache.image lockFocusFlipped:YES];
        scene->drawPile(b,cache.count,[&](snow::LeafPose leaf) {
            leaf.x-=x; leaf.y-=y; [self paintLeaf:leaf];
        });
        [cache.image unlockFocus];
        cache.count=b.leaves.size();
    }
    [cache.image drawInRect:NSMakeRect(x,y,end-x,56) fromRect:NSZeroRect
        operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
}
- (void)drawRect:(NSRect)dirty {
    CGContextClearRect(NSGraphicsContext.currentContext.CGContext,dirty);
    for (auto it=bankCache.begin();it!=bankCache.end();) {
        if (it->first && !scene->banks.count(it->first)) it=bankCache.erase(it);
        else ++it;
    }
    if(scene->autumn) {
        [self drawLeafBank:scene->floor];
        for(const auto &entry:scene->banks) [self drawLeafBank:entry.second];
        scene->drawAirborne([&](snow::LeafPose leaf){[self paintLeaf:leaf];});
        return;
    }
    [self drawBank:scene->floor];
    for (const auto &entry : scene->banks) [self drawBank:entry.second];
    for (const auto &f : scene->flakes) {
        int kind=snow::flakeKind(f);
        float side=snow::flakeSide(f);
        [sprites[kind] drawInRect:NSMakeRect(f.x-side/2,f.y-side/2,side,side)
            fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
    }
}
@end

#include "Controls.inc"

@interface App : NSObject <NSApplicationDelegate>
@property(strong) NSMutableArray<NSWindow *> *panels;
@property(strong) NSStatusItem *status;
@property(strong) NSTimer *timer;
@property(strong) ControlPanel *controls;
@property BOOL eco;
@property BOOL paused;
@property BOOL displaySleeping;
@property BOOL sessionInactive;
@property double lastFrame, lastScan;
@end

@implementation App
- (void)applicationDidFinishLaunching:(NSNotification *)note {
    NSMenu *mainMenu=[NSMenu new];
    NSMenuItem *appItem=[mainMenu addItemWithTitle:@"Weather Desktop" action:nil keyEquivalent:@""];
    NSMenu *appMenu=[[NSMenu alloc] initWithTitle:@"Weather Desktop"];
    NSMenuItem *preferencesItem=[appMenu addItemWithTitle:@"Настройки…" action:@selector(openControls:) keyEquivalent:@","];
    preferencesItem.target=self;
    [appMenu addItem:NSMenuItem.separatorItem];
    [appMenu addItemWithTitle:@"Завершить Weather Desktop" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu=appMenu;
    NSApp.mainMenu=mainMenu;
    self.status = [NSStatusBar.systemStatusBar statusItemWithLength:NSVariableStatusItemLength];
    self.status.button.title = @"❄";
    self.status.button.toolTip = @"Weather Desktop";
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"Weather Desktop"];
    NSMenuItem *settings=[menu addItemWithTitle:@"Настройки…" action:@selector(openControls:) keyEquivalent:@","];
    settings.target=self;
    [menu addItem:NSMenuItem.separatorItem];
    for (NSString *title in @[@"Снег", @"Осенние листья"]) {
        NSMenuItem *mode=[menu addItemWithTitle:title action:@selector(chooseSeason:) keyEquivalent:@""];
        mode.target=self;
        mode.tag=[title isEqualToString:@"Осенние листья"];
    }
    [menu addItem:NSMenuItem.separatorItem];
    NSMenuItem *pause = [menu addItemWithTitle:@"Пауза" action:@selector(toggle:) keyEquivalent:@""];
    pause.target = self;
    [menu addItemWithTitle:@"Завершить Weather Desktop" action:@selector(terminate:) keyEquivalent:@"q"];
    self.status.menu = menu;
    [[NSUserDefaults standardUserDefaults] registerDefaults:@{@"Autumn":@YES,@"Speed":@1,@"Accumulation":@1,@"Wind":@0,@"Ground":@YES,@"Eco":@NO,@"Density":@1,@"GlassMode":@2,@"GlassTint":@0.55}];
    [self refreshSeason];
    [self rebuild];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(screensChanged:)
        name:NSApplicationDidChangeScreenParametersNotification object:nil];
    NSNotificationCenter *workspace=NSWorkspace.sharedWorkspace.notificationCenter;
    [workspace addObserver:self selector:@selector(displayStateChanged:) name:NSWorkspaceScreensDidSleepNotification object:nil];
    [workspace addObserver:self selector:@selector(displayStateChanged:) name:NSWorkspaceScreensDidWakeNotification object:nil];
    [workspace addObserver:self selector:@selector(displayStateChanged:) name:NSWorkspaceSessionDidResignActiveNotification object:nil];
    [workspace addObserver:self selector:@selector(displayStateChanged:) name:NSWorkspaceSessionDidBecomeActiveNotification object:nil];
    [self scheduleFrames];
    [self openControls:nil];
}
- (BOOL)applicationShouldHandleReopen:(NSApplication *)app hasVisibleWindows:(BOOL)visible {
    [self openControls:nil];
    return NO;
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app { return NO; }
- (void)updateTimerState {
    BOOL active=!self.displaySleeping && !self.sessionInactive && (!self.paused || self.controls.window.isVisible);
    self.timer.fireDate=active?NSDate.date:NSDate.distantFuture;
}
- (void)displayStateChanged:(NSNotification *)note {
    if ([note.name isEqual:NSWorkspaceScreensDidSleepNotification]) self.displaySleeping=YES;
    if ([note.name isEqual:NSWorkspaceScreensDidWakeNotification]) self.displaySleeping=NO;
    if ([note.name isEqual:NSWorkspaceSessionDidResignActiveNotification]) self.sessionInactive=YES;
    if ([note.name isEqual:NSWorkspaceSessionDidBecomeActiveNotification]) self.sessionInactive=NO;
    self.lastFrame=self.lastScan=0;
    [self updateTimerState];
}
- (void)applicationWillTerminate:(NSNotification *)note {
    [self.timer invalidate];
    [NSNotificationCenter.defaultCenter removeObserver:self];
    [NSWorkspace.sharedWorkspace.notificationCenter removeObserver:self];
}
- (void)scheduleFrames {
    [self.timer invalidate];
    self.eco=[NSUserDefaults.standardUserDefaults boolForKey:@"Eco"];
    self.timer = [NSTimer timerWithTimeInterval:(self.eco?1.0/20:1.0/30) target:self selector:@selector(tick:)
        userInfo:nil repeats:YES];
    self.timer.tolerance=self.eco?.007:.002;
    [NSRunLoop.mainRunLoop addTimer:self.timer forMode:NSRunLoopCommonModes];
    [self updateTimerState];
}
- (void)openControls:(id)sender {
    if(!self.controls) {
        self.controls=[ControlPanel new];
        __weak App *owner=self;
        self.controls.onChange=^(NSString *key){[owner changed:key];};
        [self.controls open];
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(controlsClosing:)
            name:NSWindowWillCloseNotification object:self.controls.window];
    } else [self.controls open];
    [self updateTimerState];
}
- (void)controlsClosing:(NSNotification *)note {
    if(self.paused) self.timer.fireDate=NSDate.distantFuture;
}
- (void)changed:(NSString *)key {
    if([key isEqualToString:@"Shake"]) {
        for(NSWindow *panel in self.panels) {
            SnowView *view=(SnowView *)panel.contentView;
            view->scene->shakeWindows();
        }
        SnowView *view=self.controls.preview;
        if(view) view->scene->shakeWindows();
    } else if([key isEqualToString:@"Autumn"]) {
        [self refreshSeason]; [self rebuild];
        if(self.controls.window) [self.controls refreshPreview];
    } else if([key isEqualToString:@"Eco"]) {
        [self scheduleFrames];
    } else {
        auto next=preferences();
        for(NSWindow *p in self.panels) {
            SnowView *v=(SnowView *)p.contentView;
            v->scene->configure(next);
            v->bankCache.clear();
        }
        SnowView *v=self.controls.preview;
        if(v) { v->scene->configure(next); v->bankCache.clear(); }
    }
}
- (void)refreshSeason {
    BOOL autumn=[[NSUserDefaults standardUserDefaults] boolForKey:@"Autumn"];
    self.status.button.title=autumn?@"🍁":@"❄";
    for(NSMenuItem *item in self.status.menu.itemArray)
        if(item.action==@selector(chooseSeason:)) item.state=(item.tag==autumn)?NSControlStateValueOn:NSControlStateValueOff;
}
- (void)chooseSeason:(NSMenuItem *)item {
    if(item.state==NSControlStateValueOn) return;
    [[NSUserDefaults standardUserDefaults] setBool:item.tag!=0 forKey:@"Autumn"];
    [self changed:@"Autumn"];
}
- (void)toggle:(NSMenuItem *)item {
    self.paused = !self.paused;
    item.title = self.paused ? @"Продолжить" : @"Пауза";
    [self updateTimerState];
    self.lastScan = self.lastFrame = 0;
    for (NSWindow *p in self.panels) {
        if (self.paused) [p orderOut:nil]; else [p orderFrontRegardless];
    }
}
- (void)screensChanged:(NSNotification *)note { [self rebuild]; }
- (void)rebuild {
    for (NSWindow *p in self.panels) [p orderOut:nil];
    self.panels = [NSMutableArray array];
    for (NSScreen *s in NSScreen.screens) {
        NSWindow *p = [[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,s.frame.size.width,s.frame.size.height)
            styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:NO];
        [p setFrame:s.frame display:NO];
        p.opaque = NO;
        p.backgroundColor = NSColor.clearColor;
        p.hasShadow = NO;
        p.ignoresMouseEvents = YES;
        p.level = NSFloatingWindowLevel;
        p.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
            NSWindowCollectionBehaviorStationary | NSWindowCollectionBehaviorIgnoresCycle |
            NSWindowCollectionBehaviorFullScreenAuxiliary;
        p.contentView = [[SnowView alloc] initWithFrame:NSMakeRect(0,0,s.frame.size.width,s.frame.size.height)];
        if (!self.paused) [p orderFrontRegardless];
        [self.panels addObject:p];
    }
    self.lastScan = 0;
}
- (void)scan {
    CFArrayRef raw = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly |
        kCGWindowListExcludeDesktopElements,kCGNullWindowID);
    if (!raw) return;
    if (NSScreen.screens.count==0) { CFRelease(raw); return; }
    NSArray *rows = CFBridgingRelease(raw);
    CGFloat top = NSMaxY(NSScreen.screens.firstObject.frame);
    for (NSWindow *p in self.panels) {
        CGRect screen = CGRectMake(NSMinX(p.frame),top-NSMaxY(p.frame),NSWidth(p.frame),NSHeight(p.frame));
        std::vector<snow::Window> windows;
        for (NSDictionary *row in rows) {
            if ([row[(__bridge NSString *)kCGWindowLayer] intValue]!=0 || [row[(__bridge NSString *)kCGWindowOwnerPID] intValue]==getpid() ||
                [row[(__bridge NSString *)kCGWindowAlpha] floatValue]<.05) continue;
            CGRect r=CGRectZero;
            if (!CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)row[(__bridge NSString *)kCGWindowBounds],&r) ||
                r.size.width<30 || r.size.height<20 || !CGRectIntersectsRect(r,screen)) continue;
            windows.push_back({[row[(__bridge NSString *)kCGWindowNumber] unsignedIntValue],
                float(r.origin.x-screen.origin.x),float(r.origin.y-screen.origin.y),
                float(r.size.width),float(r.size.height)});
        }
        SnowView *v = (SnowView *)p.contentView;
        v->scene->sync(windows);
    }
}
- (void)tick:(NSTimer *)timer {
    if(self.displaySleeping || self.sessionInactive) return;
    double now = NSProcessInfo.processInfo.systemUptime;

    float dt = self.lastFrame ? now-self.lastFrame : 1.f/30;
    self.lastFrame = now;
    if(self.controls.window.isVisible) {
        SnowView *v=self.controls.preview;
        v->scene->advance(dt); [v setNeedsDisplay:YES];
    }
    if(self.paused) return;
    if(now-self.lastScan>=(self.eco?.25:.10)) { [self scan]; self.lastScan=now; }
    for (NSWindow *p in self.panels) {
        SnowView *v = (SnowView *)p.contentView;
        v->scene->advance(dt);
        [v setNeedsDisplay:YES];
    }
}
@end

int main(void) {
    @autoreleasepool {
        for (NSRunningApplication *running in [NSRunningApplication runningApplicationsWithBundleIdentifier:@"local.windowsnow.desktop"]) {
            if (running.processIdentifier!=getpid()) {
                if (running.bundleURL) [NSWorkspace.sharedWorkspace openURL:running.bundleURL];
                return 0;
            }
        }
        NSApplication *app = NSApplication.sharedApplication;
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        __attribute__((objc_precise_lifetime)) App *delegate = [App new];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
