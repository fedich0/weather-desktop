#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <random>
#include <vector>

namespace snow {
inline float bounded(float value, float low, float high, float fallback) {
    return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
}
struct Settings {
    float speed=1, accumulation=1, wind=0;
    bool ground=true;
    float density=1;
};
struct Window {
    uint32_t id;
    float x, y, w, h;
};
struct Flake {
    float x, y, vx, vy, radius, phase;
    bool loose;
};
struct RestingLeaf { Flake particle; float offset; };
struct LeafPose { float x, y, size, angle, squash; int sprite; };
struct Bank {
    Window frame;
    std::vector<float> depth;
    float cooldown = 0;
    std::vector<RestingLeaf> leaves;
    uint64_t revision=0;
};
inline float grain(uint32_t n) {
    n ^= n >> 16; n *= 0x7feb352d; n ^= n >> 15; n *= 0x846ca68b; n ^= n >> 16;
    return (n >> 8) / 16777216.f;
}
// Smooth seeded noise keeps every drift anchored to its own window.
inline float drift(float x, uint32_t seed) {
    int i = int(std::floor(x));
    float t = x-i;
    t = t*t*(3-2*t);
    return grain(seed+uint32_t(i))*(1-t) + grain(seed+uint32_t(i+1))*t;
}
inline float capacity(const Bank &bank, int i, float limit) {
    float x = i*8.f + 4;
    float pockets = .56f*drift(x/41,bank.frame.id+91) + .44f*drift(x/17,bank.frame.id+33);
    return limit*(.25f+.75f*pockets);
}

class Scene {
    static constexpr int step = 8;
    static constexpr size_t particleLimit = 2000;
    static constexpr size_t windowLimit = 64;
    static constexpr size_t leafLimitPerBank = 1600;
    std::mt19937 rng;
    float width, height, clock = 0, spawnCredit = 0;
    float randf(float a, float b) {
        return std::uniform_real_distribution<float>(a, b)(rng);
    }
    static Bank bank(Window w) {
        return {w, std::vector<float>((size_t)std::ceil(w.w / step)), 0,{},0};
    }
    void deposit(Bank &b, int i, float amount, float limit) {
        for (int j = std::max(0, i-1); j <= std::min(i+1, int(b.depth.size())-1); ++j)
            b.depth[j] = std::min(capacity(b,j,limit), b.depth[j] + amount * (i == j ? 1.f : .3f));
    }
    void spill(const Bank &b, float dx) {
        if (autumn) {
            for (const auto &leaf : b.leaves) {
                if (flakes.size() >= particleLimit) break;
                Flake f=leaf.particle;
                f.x+=b.frame.x;
                f.y=b.frame.y-leaf.offset;
                f.vx=std::clamp(dx*1.5f,-75.f,75.f)+randf(-28,28);
                f.vy=randf(-26,30);
                f.loose=true;
                flakes.push_back(f);
            }
            return;
        }
        for (size_t i = 0; i < b.depth.size(); ++i) {
            float d = b.depth[i];
            if (d < .2f) continue;
            for (int j = 0; j < std::ceil(d / 2); ++j) {
                if (flakes.size() >= particleLimit) return;
                float x = b.frame.x + i*step + randf(0, step);
                if (x < 0 || x >= width || x >= b.frame.x+b.frame.w) continue;
                flakes.push_back({x, b.frame.y-randf(0,d), std::clamp(dx*2.f,-65.f,65.f)+randf(-24,24),
                                  randf(-48,60),randf(1.0,3.3),randf(0,6),true});
            }
        }
    }
public:
    std::vector<Window> frontToBack;
    std::map<uint32_t,Bank> banks;
    std::vector<Flake> flakes;
    Bank floor;
    const bool autumn;
    Settings settings;
    uint64_t geometryVersion=0;
    void shakeWindows() {
        for(auto &entry:banks) {
            Bank &b=entry.second;
            spill(b,0);
            std::fill(b.depth.begin(),b.depth.end(),0.f);
            b.leaves.clear();
            b.cooldown=.24f;
        }
        ++geometryVersion;
    }
    void configure(Settings next) {
        next.speed=bounded(next.speed,.4f,2.5f,1);
        next.accumulation=bounded(next.accumulation,0,1,1);
        next.wind=bounded(next.wind,-80,80,0);
        next.density=bounded(next.density,.2f,2.5f,1);
        auto clear=[](Bank &b){std::fill(b.depth.begin(),b.depth.end(),0.f);b.leaves.clear();++b.revision;};
        if ((!next.ground && settings.ground) || (!next.accumulation && settings.accumulation)) clear(floor);
        if (!next.accumulation && settings.accumulation)
            for (auto &entry:banks) clear(entry.second);
        settings=next;
    }
    template<class Draw> void drawPile(const Bank &bank, size_t first, Draw draw) const {
        for (size_t i=first;i<bank.leaves.size();++i) {
            const auto &leaf=bank.leaves[i];
            const auto &f=leaf.particle;
            float x=bank.frame.x+f.x;
            if (bank.frame.id && !exposed(bank.frame.id,x,bank.frame.y)) continue;
            draw(LeafPose{x,bank.frame.y-leaf.offset,12+f.radius*3.3f,
                1.57f+.33f*std::sin(f.phase*4)+(f.phase>3?3.14f:0),.60f,int(f.phase)%6});
        }
    }
    template<class Draw> void drawAirborne(Draw draw) const {
        for (const auto &f:flakes) {
            float wave=clock*(1.6f+.12f*f.phase)+f.phase;
            draw(LeafPose{f.x,f.y,12+f.radius*3.3f,.76f*std::sin(wave)+f.phase,
                .20f+.80f*std::abs(std::cos(wave*.92f)),int(f.phase)%6});
        }
    }
    template<class Draw> void drawLeaves(Draw draw) const {
        drawPile(floor,0,draw);
        for (const auto &entry:banks) drawPile(entry.second,0,draw);
        drawAirborne(draw);
    }
    Scene(float w, float h, uint32_t seed, bool leaves=false) : rng(seed), width(bounded(w,1,32768,1)), height(bounded(h,1,32768,1)), floor(bank({0,0,height,width,0})), autumn(leaves) {
        for (int i=0; i<std::min<int>(particleLimit,int(width*height/(autumn?30000:8500))); ++i)
            flakes.push_back({randf(0,width),randf(0,height),randf(-8,8),randf(45,95),randf(.75,3.3),randf(0,6),false});
    }
    bool exposed(uint32_t id, float x, float y) const {
        for (const auto &w : frontToBack) {
            if (w.id == id) return true;
            if (x >= w.x && x < w.x+w.w && y >= w.y && y < w.y+w.h) return false;
        }
        return false;
    }
    void sync(const std::vector<Window> &input) {
        // Window-server metadata may change during a display reconfiguration.
        std::vector<Window> windows;
        for (const auto &w : input) {
            if (windows.size() >= windowLimit) break;
            if (!w.id || !std::isfinite(w.x) || !std::isfinite(w.y) || !std::isfinite(w.w) || !std::isfinite(w.h) ||
                w.w<=0 || w.h<=0 || w.w>32768 || w.h>32768 ||
                w.x+w.w<=0 || w.x>=width || w.y>=height || w.y+w.h<=0) continue;
            if (std::none_of(windows.begin(),windows.end(),[&](const Window &v){return v.id==w.id;})) windows.push_back(w);
        }
        auto same=[](const Window &a,const Window &b){
            return a.id==b.id && a.x==b.x && a.y==b.y && a.w==b.w && a.h==b.h;
        };
        if (!std::equal(windows.begin(),windows.end(),frontToBack.begin(),frontToBack.end(),same)) ++geometryVersion;
        frontToBack = windows;
        for (const Window &w : windows) {
            auto it = banks.find(w.id);
            if (it == banks.end()) { banks.emplace(w.id, bank(w)); continue; }
            auto &b = it->second;
            float dx = w.x-b.frame.x;
            if (std::abs(dx)>=1 || std::abs(w.y-b.frame.y)>=1 || w.w!=b.frame.w) {
                spill(b,dx);
                b = bank(w);
                b.cooldown = .24f;
            } else b.frame = w;
        }
        for (auto it = banks.begin(); it != banks.end();) {
            if (std::none_of(windows.begin(),windows.end(),[&](const Window &w){return w.id==it->first;})) {
                spill(it->second,0);
                it = banks.erase(it);
            } else ++it;
        }
    }
    void advance(float dt) {
        if (!std::isfinite(dt) || dt<=0) return;
        dt = std::min(dt,.06f);
        clock += dt;
        for (auto &entry : banks) entry.second.cooldown = std::max(0.f,entry.second.cooldown-dt);
        spawnCredit += dt * width * settings.density / (autumn?115.f:38.f);
        while (spawnCredit >= 1) {
            --spawnCredit;
            if (flakes.size()<particleLimit) flakes.push_back({randf(0,width),-3,randf(-7,7),randf(45,95),randf(.75,3.3),randf(0,6),false});
        }
        flakes.erase(std::remove_if(flakes.begin(),flakes.end(),[&](Flake &f) {
            float oldBottom = f.y+f.radius;
            if (autumn) {
                float wave=clock*(1.6f+.12f*f.phase)+f.phase;
                f.x+=std::sin(wave)*55*dt;
                if(f.loose) f.vy=std::min(118.f,f.vy+132.f*dt);
                f.y+=std::cos(wave)*8*dt*settings.speed;
            } else if (f.loose) f.vy = std::min(410.f,f.vy+380.f*dt);
            f.x += (f.vx+std::sin(clock*1.2f+f.phase)*(f.loose?3:15))*dt;
            f.x += settings.wind*dt;
            f.y += f.vy*dt*settings.speed;
            if (f.x<0 || f.x>=width) return true;
            int floorIndex = int(f.x/step);
            float hit = height-floor.depth[floorIndex];
            Bank *target = &floor;
            int index = floorIndex;
            if (!f.loose) for (const auto &w : frontToBack) {
                if (f.x<w.x+3 || f.x>=w.x+w.w-3) continue;
                auto &b = banks.at(w.id);
                if (b.cooldown>0) continue;
                int i = int((f.x-w.x)/step);
                float surface = w.y-b.depth[i];
                if (oldBottom<=surface+.4f && f.y+f.radius>=surface && surface<hit && exposed(w.id,f.x,w.y)) {
                    hit = surface; target = &b; index = i;
                }
            }
            if (f.y+f.radius<hit) return false;
            if (target==&floor && !settings.ground) return f.y-f.radius>=height;
            if (settings.accumulation<1 && (settings.accumulation<=0 || randf(0,1)>=settings.accumulation)) {
                if(target==&floor) return true;
                f.loose=true;
                return false;
            }
            if (autumn) {
                float limit=target==&floor?24:17;
                if(target->depth[index]+2.5f>limit || target->leaves.size()>=leafLimitPerBank) {
                    if(target==&floor) return true;
                    f.loose=true;
                    return false;
                }
                Flake resting=f;
                resting.x-=target->frame.x;
                target->leaves.push_back({resting,target->depth[index]+2.5f});
                target->depth[index]+=2.5f;
                return true;
            }
            deposit(*target,index,f.radius*.8f,target==&floor?36:32);
            return true;
        }),flakes.end());
    }
};
}
