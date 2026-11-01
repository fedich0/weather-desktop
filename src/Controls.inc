// Included after SnowView so the preview uses the same simulation and renderer as the desktop.
@interface FlippedPane : NSView @end
@implementation FlippedPane
- (BOOL)isFlipped { return YES; }
@end

static NSTextField *caption(NSView *host, NSString *value, NSRect frame, CGFloat size, NSFontWeight weight) {
    NSTextField *label=[NSTextField labelWithString:value];
    label.frame=frame; label.font=[NSFont systemFontOfSize:size weight:weight];
    [host addSubview:label];
    return label;
}
static NSView *glassSurface(NSView *body) {
    if (NSWorkspace.sharedWorkspace.accessibilityDisplayShouldReduceTransparency) {
        body.wantsLayer=YES;
        body.layer.backgroundColor=NSColor.windowBackgroundColor.CGColor;
        return body;
    }
    Class glass=NSClassFromString(@"NSGlassEffectView");
    if(glass) {
        NSView *surface=[[glass alloc] initWithFrame:body.bounds];
        [surface setValue:@18 forKey:@"cornerRadius"];
        [surface setValue:body forKey:@"contentView"];
        return surface;
    }
    NSVisualEffectView *surface=[[NSVisualEffectView alloc] initWithFrame:body.bounds];
    surface.material=NSVisualEffectMaterialPopover;
    surface.blendingMode=NSVisualEffectBlendingModeBehindWindow;
    surface.state=NSVisualEffectStateActive;
    [surface addSubview:body];
    return surface;
}

static NSColor *champagne(CGFloat alpha) {
    return [NSColor colorWithSRGBRed:.92 green:.75 blue:.52 alpha:alpha];
}
static NSTextField *smallTitle(NSView *host,NSString *title,CGFloat x,CGFloat y) {
    NSTextField *field=caption(host,title,NSMakeRect(x,y,300,18),10,NSFontWeightSemibold);
    field.textColor=champagne(.95);
    return field;
}

// The NSButton stays above its material so keyboard focus, pointer tracking and accessibility are native.
@interface StudioKey : FlippedPane
@property(strong) NSButton *button;
@property(strong) NSView *material;
@property NSInteger installedMode;
@property BOOL selected;
- (void)updateMaterial;
@end
@implementation StudioKey
- (void)updateMaterial {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    NSInteger mode=[d integerForKey:@"GlassMode"];
    if(NSWorkspace.sharedWorkspace.accessibilityDisplayShouldReduceTransparency) mode=0;
    CGFloat tint=snow::bounded([d floatForKey:@"GlassTint"],0,1,.55f);
    if(!self.material || mode!=self.installedMode) {
        [self.material removeFromSuperview];
        Class glass=NSClassFromString(@"NSGlassEffectView");
        if(mode==2 && glass) {
            self.material=[[glass alloc] initWithFrame:self.bounds];
            [self.material setValue:@13 forKey:@"cornerRadius"];
        } else if(mode!=0) {
            NSVisualEffectView *blur=[[NSVisualEffectView alloc] initWithFrame:self.bounds];
            blur.material=mode==1?NSVisualEffectMaterialHUDWindow:NSVisualEffectMaterialPopover;
            blur.blendingMode=NSVisualEffectBlendingModeWithinWindow; blur.state=NSVisualEffectStateActive;
            self.material=blur;
        } else self.material=[[NSView alloc] initWithFrame:self.bounds];
        self.installedMode=mode;
        [self addSubview:self.material positioned:NSWindowBelow relativeTo:self.button];
    }
    self.wantsLayer=YES; self.layer.cornerRadius=13;self.layer.masksToBounds=YES;
    NSColor *wash=self.selected?champagne(.18+tint*.45):[NSColor colorWithWhite:.78 alpha:.035+tint*.13];
    if([self.material respondsToSelector:@selector(setTintColor:)]) [self.material setValue:wash forKey:@"tintColor"];
    self.material.wantsLayer=YES;
    self.material.layer.backgroundColor=(mode==0?(self.selected?champagne(.62):[NSColor colorWithWhite:.21 alpha:1]):wash).CGColor;
    self.layer.borderWidth=mode==0?.6:1;
    self.layer.borderColor=(self.selected?champagne(.74):[NSColor colorWithWhite:1 alpha:.12+.28*tint]).CGColor;
    self.button.attributedTitle=[[NSAttributedString alloc] initWithString:self.button.title attributes:@{
        NSForegroundColorAttributeName:NSColor.whiteColor,NSFontAttributeName:[NSFont systemFontOfSize:12 weight:self.selected?NSFontWeightBold:NSFontWeightMedium]}];
}
@end

@interface ControlPanel : NSObject
@property(strong) NSPanel *window;
@property(strong) FlippedPane *hero;
@property(strong) FlippedPane *weather;
@property(strong) FlippedPane *appearancePage;
@property(strong) NSView *previewLedge;
@property(strong) SnowView *preview;
@property(strong) NSImageView *scenery;
@property(strong) NSTextField *sceneName;
@property(strong) NSTextField *notice;
@property(strong) NSMutableArray<StudioKey *> *keys;
@property(strong) NSMutableDictionary<NSString *,NSControl *> *inputs;
@property(strong) NSMutableDictionary<NSString *,NSTextField *> *values;
@property(copy) void (^onChange)(NSString *);
- (void)open;
- (void)refreshPreview;
@end
@implementation ControlPanel
- (NSString *)valueString:(NSString *)key {
    float v=[NSUserDefaults.standardUserDefaults floatForKey:key];
    if([key isEqual:@"Speed"]) return [[NSString stringWithFormat:@"%.2g×",v] stringByReplacingOccurrencesOfString:@"." withString:@","];
    if([key isEqual:@"Wind"]) return fabs(v)<.5?@"Штиль":[NSString stringWithFormat:@"%@ %.0f",v<0?@"←":@"→",fabs(v)];
    return [NSString stringWithFormat:@"%.0f %%",v*100];
}
- (void)button:(NSView *)host title:(NSString *)title id:(NSString *)identifier rect:(NSRect)r {
    StudioKey *key=[[StudioKey alloc] initWithFrame:r];
    key.button=[NSButton buttonWithTitle:title target:self action:@selector(press:)];
    key.button.frame=key.bounds; key.button.identifier=identifier; key.button.bordered=NO;
    key.button.focusRingType=NSFocusRingTypeExterior;
    [key addSubview:key.button];[host addSubview:key];[self.keys addObject:key];[key updateMaterial];
}
- (void)slider:(NSView *)host title:(NSString *)title key:(NSString *)key y:(CGFloat)y low:(double)low high:(double)high {
    caption(host,title,NSMakeRect(0,y,230,19),13,NSFontWeightMedium);
    NSTextField *n=caption(host,@"",NSMakeRect(234,y,92,19),12,NSFontWeightMedium);
    n.alignment=NSTextAlignmentRight;n.textColor=champagne(1);self.values[key]=n;
    NSSlider *s=[NSSlider sliderWithValue:[NSUserDefaults.standardUserDefaults doubleForKey:key] minValue:low maxValue:high target:self action:@selector(adjust:)];
    s.identifier=key;s.continuous=YES;s.frame=NSMakeRect(-2,y+21,330,19);
    if (@available(macOS 12.0, *)) s.trackFillColor=champagne(1); [s setAccessibilityLabel:title];[host addSubview:s];self.inputs[key]=s;
}
- (void)toggle:(NSView *)host title:(NSString *)title key:(NSString *)key y:(CGFloat)y {
    caption(host,title,NSMakeRect(0,y+3,260,23),13,NSFontWeightMedium);
    NSSwitch *s=[[NSSwitch alloc] initWithFrame:NSMakeRect(278,y,48,25)];s.target=self;s.action=@selector(adjust:);
    s.identifier=key;[s setAccessibilityLabel:title];[host addSubview:s];self.inputs[key]=s;
}
- (void)syncUI {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    for(NSString *key in self.inputs) {
        NSControl *input=self.inputs[key];
        if([input isKindOfClass:NSSwitch.class]) ((NSSwitch *)input).state=[d boolForKey:key]?NSControlStateValueOn:NSControlStateValueOff;
        else {input.doubleValue=[d doubleForKey:key];self.values[key].stringValue=[self valueString:key];}
    }
    NSArray *material=@[@"Flat",@"Frost",@"Liquid"];
    for(StudioKey *k in self.keys) {
        NSString *i=k.button.identifier;
        k.selected=([i isEqual:@"Winter"] && ![d boolForKey:@"Autumn"]) || ([i isEqual:@"Fall"] && [d boolForKey:@"Autumn"])
            || ([i isEqual:@"Weather"] && !self.weather.hidden) || ([i isEqual:@"Design"] && self.weather.hidden)
            || [i isEqual:material[std::clamp<NSInteger>([d integerForKey:@"GlassMode"],0,2)]]
            || [i isEqual:[d stringForKey:@"ActivePreset"]];
        k.button.enabled=![i isEqual:@"Recall"] || [d dictionaryForKey:@"Favorite"]!=nil;
        [k updateMaterial];
        k.alphaValue=k.button.enabled?1:.45;
    }
}
- (void)adjust:(NSControl *)sender {
    NSString *key=sender.identifier;NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    if([sender isKindOfClass:NSSwitch.class]) [d setBool:((NSSwitch *)sender).state==NSControlStateValueOn forKey:key];
    else [d setDouble:sender.doubleValue forKey:key];
    self.values[key].stringValue=[self valueString:key];
    if([key isEqual:@"GlassTint"]) {for(StudioKey *k in self.keys) [k updateMaterial];return;}
    if(self.values[key]) {[d removeObjectForKey:@"ActivePreset"];[self syncUI];}
    self.onChange(key);
}
- (void)press:(NSButton *)sender {
    NSString *key=sender.identifier; NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    NSArray *styles=@[@"Flat",@"Frost",@"Liquid"];
    NSUInteger index=[styles indexOfObject:key];
    NSArray *fields=@[@"Speed",@"Density",@"Accumulation",@"Wind"];
    NSDictionary *presets=@{@"Quiet":@[@.68,@.45,@.45,@0],@"Breeze":@[@1.0,@1.0,@.8,@26],@"Storm":@[@1.7,@2.25,@1,@(-54)]};
    if([key isEqual:@"Weather"] || [key isEqual:@"Design"]) {
        self.weather.hidden=[key isEqual:@"Design"];self.appearancePage.hidden=!self.weather.hidden;
    } else if(index!=NSNotFound) [d setInteger:index forKey:@"GlassMode"];
    else if([key isEqual:@"Fall"] || [key isEqual:@"Winter"]) {
        BOOL autumn=[key isEqual:@"Fall"];
        if(autumn!=[d boolForKey:@"Autumn"]) {[d setBool:autumn forKey:@"Autumn"];self.onChange(@"Autumn");}
    } else if(presets[key]) {
        NSArray *vals=presets[key];
        for(NSUInteger j=0;j<fields.count;j++) [d setObject:vals[j] forKey:fields[j]];
        [d setObject:key forKey:@"ActivePreset"]; self.onChange(@"Weather");
    } else if([key isEqual:@"Shake"]) self.onChange(@"Shake");
    else if([key isEqual:@"Save"]) {
        NSMutableDictionary *saved=[NSMutableDictionary dictionary];
        for(NSString *field in [fields arrayByAddingObjectsFromArray:@[@"Ground",@"Autumn",@"Eco"]]) saved[field]=[d objectForKey:field];
        [d setObject:saved forKey:@"Favorite"];self.notice.stringValue=@"Ваш профиль сохранён";
    } else if([key isEqual:@"Recall"]) {
        NSDictionary *saved=[d dictionaryForKey:@"Favorite"];
        if(!saved)return;
        BOOL oldSeason=[d boolForKey:@"Autumn"];
        for(NSString *field in [fields arrayByAddingObjectsFromArray:@[@"Ground",@"Autumn",@"Eco"]]) {
            id value=saved[field];
            if ([value isKindOfClass:NSNumber.class]) [d setObject:value forKey:field];
        }
        BOOL seasonChanged=oldSeason!=[d boolForKey:@"Autumn"];
        [d removeObjectForKey:@"ActivePreset"]; self.onChange(seasonChanged?@"Autumn":@"Weather");self.onChange(@"Eco");
        self.notice.stringValue=@"Ваш профиль применён";
    }
    [self syncUI];
}
- (void)refreshPreview {
    BOOL autumn=[NSUserDefaults.standardUserDefaults boolForKey:@"Autumn"];
    self.sceneName.stringValue=autumn?@"Янтарный лес":@"Полярная ночь";
    self.scenery.image=[[NSImage alloc] initWithContentsOfURL:[NSBundle.mainBundle URLForResource:autumn?@"studio-autumn":@"studio-winter" withExtension:@"jpg"]];
    [self.preview removeFromSuperview];self.preview=[[SnowView alloc] initWithFrame:self.hero.bounds];
    self.preview.wantsLayer=YES;[self.hero addSubview:self.preview positioned:NSWindowAbove relativeTo:self.previewLedge];
    self.preview->scene->sync({{1,40,234,162,65}});
    for(int i=0;i<180;++i)self.preview->scene->advance(1.f/30);
    [self syncUI];
}
- (void)open {
    if(!self.window) {
        self.keys=[NSMutableArray array];self.values=[NSMutableDictionary dictionary];self.inputs=[NSMutableDictionary dictionary];
        NSRect r=NSMakeRect(0,0,680,520);
        self.window=[[NSPanel alloc] initWithContentRect:r styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];
        self.window.title=@"Weather Desktop";self.window.appearance=[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
        self.window.releasedWhenClosed=NO;self.window.level=NSFloatingWindowLevel+1;self.window.hidesOnDeactivate=NO;
        self.window.opaque=NO;self.window.backgroundColor=NSColor.clearColor;
        FlippedPane *body=[[FlippedPane alloc] initWithFrame:r];self.window.contentView=glassSurface(body);
        self.hero=[[FlippedPane alloc] initWithFrame:NSMakeRect(12,12,280,496)];
        self.hero.wantsLayer=YES;self.hero.layer.cornerRadius=19;self.hero.layer.masksToBounds=YES;[body addSubview:self.hero];
        self.scenery=[[NSImageView alloc] initWithFrame:self.hero.bounds];self.scenery.imageScaling=NSImageScaleAxesIndependently;[self.hero addSubview:self.scenery];
        smallTitle(self.hero,@"W E A T H E R   D E S K T O P",19,22);
        self.sceneName=caption(self.hero,@"",NSMakeRect(18,50,252,58),32,NSFontWeightMedium);
        self.sceneName.font=[NSFont fontWithName:@"Georgia-Italic" size:33]?:self.sceneName.font;
        self.sceneName.textColor=NSColor.whiteColor;
        NSView *ledge=[[NSView alloc] initWithFrame:NSMakeRect(40,234,162,65)];
        self.previewLedge=ledge;
        ledge.wantsLayer=YES;ledge.layer.cornerRadius=9;ledge.layer.backgroundColor=[NSColor colorWithWhite:.93 alpha:.20].CGColor;
        ledge.layer.borderWidth=1;ledge.layer.borderColor=[NSColor colorWithWhite:1 alpha:.32].CGColor;
        [self.hero addSubview:ledge positioned:NSWindowAbove relativeTo:self.scenery];
        for(int i=0;i<3;i++){NSView *dot=[[NSView alloc] initWithFrame:NSMakeRect(10+i*9,53,4,4)];dot.wantsLayer=YES;dot.layer.cornerRadius=2;dot.layer.backgroundColor=champagne(.9).CGColor;[ledge addSubview:dot];}
        smallTitle(self.hero,@"СЕЗОН",18,390);
        [self button:self.hero title:@"Снег" id:@"Winter" rect:NSMakeRect(16,413,119,35)];
        [self button:self.hero title:@"Листья" id:@"Fall" rect:NSMakeRect(143,413,119,35)];
        caption(self.hero,@"Живой предпросмотр",NSMakeRect(19,465,243,18),11,NSFontWeightRegular).textColor=[NSColor colorWithWhite:1 alpha:.75];
        smallTitle(body,@"АТЕЛЬЕ ПОГОДЫ",315,22);
        caption(body,@"Настройки",NSMakeRect(314,39,310,44),30,NSFontWeightSemibold);
        [self button:body title:@"Атмосфера" id:@"Weather" rect:NSMakeRect(315,91,157,34)];
        [self button:body title:@"Оформление" id:@"Design" rect:NSMakeRect(480,91,160,34)];
        self.weather=[[FlippedPane alloc] initWithFrame:NSMakeRect(315,145,330,362)];
        self.appearancePage=[[FlippedPane alloc] initWithFrame:self.weather.frame];[body addSubview:self.weather];[body addSubview:self.appearancePage];self.appearancePage.hidden=YES;
        FlippedPane *w=self.weather,*a=self.appearancePage;
        smallTitle(w,@"ХАРАКТЕР ПОГОДЫ",0,0);
        NSArray *names=@[@"Тишина",@"Бриз",@"Шквал"],*ids=@[@"Quiet",@"Breeze",@"Storm"];
        for(int i=0;i<3;i++) [self button:w title:names[i] id:ids[i] rect:NSMakeRect(i*111,23,104,32)];
        [self slider:w title:@"Скорость падения" key:@"Speed" y:70 low:.4 high:2.5];
        [self slider:w title:@"Густота потока" key:@"Density" y:118 low:.2 high:2.5];
        [self slider:w title:@"Накопление" key:@"Accumulation" y:166 low:0 high:1];
        [self slider:w title:@"Ветер" key:@"Wind" y:214 low:-80 high:80];
        [self toggle:w title:@"Собирать внизу экрана" key:@"Ground" y:269];
        [self button:w title:@"Стряхнуть с окон" id:@"Shake" rect:NSMakeRect(0,313,326,34)];
        smallTitle(a,@"МАТЕРИАЛ КНОПОК",0,0);
        NSArray *labels=@[@"Без стекла",@"Матовое",@"Liquid Glass"],*modes=@[@"Flat",@"Frost",@"Liquid"];
        for(int i=0;i<3;i++) [self button:a title:labels[i] id:modes[i] rect:NSMakeRect(i*111,23,104,32)];
        [self slider:a title:@"Тонировка кнопок" key:@"GlassTint" y:76 low:0 high:1];
        smallTitle(a,@"ЛИЧНЫЙ ПРОФИЛЬ ПОГОДЫ",0,151);
        [self button:a title:@"Запомнить" id:@"Save" rect:NSMakeRect(0,179,158,33)];
        [self button:a title:@"Применить" id:@"Recall" rect:NSMakeRect(166,179,160,33)];
        self.notice=caption(a,@"Сезон, ветер и параметры падения",NSMakeRect(0,222,326,19),11,NSFontWeightRegular);
        self.notice.textColor=NSColor.secondaryLabelColor;
        [self toggle:a title:@"Экономичный режим" key:@"Eco" y:276];
        caption(a,@"20 кадров/с · реже опрашивает окна",NSMakeRect(0,308,323,18),11,NSFontWeightRegular).textColor=NSColor.secondaryLabelColor;
        [self refreshPreview];[self.window center];
    }
    [self syncUI];[NSApp activateIgnoringOtherApps:YES];[self.window makeKeyAndOrderFront:nil];
}
@end
