#pragma once
#include "Snow.hpp"

namespace snow {
inline int flakeKind(const Flake &f) {
    if (f.radius<1.48f) return 0;
    return !f.loose && f.radius>2.74f && f.phase>2.8f ? 2 : 1;
}
inline float flakeSide(const Flake &f) { return f.radius*(flakeKind(f)==2?4.15f:3.1f); }
// Two pixels per point for Retina. Rows run from the sky down to the window.
constexpr int powderScale = 2;
constexpr int sky = 42;
constexpr int stripHeight = (sky+7)*powderScale;
struct PowderStrip {
    int width;
    std::vector<uint8_t> rgba;
};
inline PowderStrip paintPowder(const Bank &b, int start, int end, const std::vector<uint8_t> &visible) {
    start=std::max(0,start);
    end=std::min(end,32768);
    if (end<=start || b.depth.empty() || visible.size()<b.depth.size()) return {0,{}};
    int width = (end-start)*powderScale;
    PowderStrip out{width,std::vector<uint8_t>(size_t(width)*stripHeight*4)};
    uint32_t seed = b.frame.id * 739;
    int last = int(b.depth.size())-1;
    for (int px=0; px<width; ++px) {
        float x = start+(px+.5f)/powderScale;
        int bin = int(x/8);
        if (bin>last || !visible[bin] || x>=b.frame.w) continue;
        float pos = x/8-.5f;
        int j = int(std::floor(pos));
        float t = pos-j; t=t*t*(3-2*t);
        float h = b.depth[std::clamp(j,0,last)]*(1-t)+b.depth[std::clamp(j+1,0,last)]*t;
        if (b.frame.id) h *= std::clamp((std::min(x,b.frame.w-x)-3.f)/6.f,0.f,1.f);
        if (h<.16f) continue;
        float soft = std::min(1.f,h/4);
        float wind = drift(x/11,seed+32), puff = drift(x/4,seed+87);
        float top = -h-soft*(2.4f*wind+1.6f*puff+.48f*drift(x/1.6f,seed+112));
        float lip = b.frame.id ? soft*(.15f+1.7f*wind*wind+.72f*puff) : 0;
        float facing = drift(x/18,seed+56);
        for (int y=std::max(0,int((sky+top)*2)-1); y<std::min(stripHeight,int((sky+lip+3)*2)+1); ++y) {
            float v = (y+.5f)/2-sky;
            uint8_t *p = &out.rgba[(size_t(y)*width+px)*4];
            if (v>lip) {
                float a = soft*std::max(0.f,1-(v-lip)/3)*.36f;
                p[0]=39;p[1]=58;p[2]=77;p[3]=uint8_t(255*a);continue;
            }
            float coverage = std::clamp((v-top)*2+.5f,0.f,1.f);
            float level = std::clamp((v-top)/(lip-top),0.f,1.f);
            float shade = (.17f+.65f*level*level)*(1-.34f*facing);
            // Shallow overlapping pockets shade the forward face, with a bright powder crest.
            float crease = level-(.42f+.24f*wind+.14f*puff);
            shade += .18f*std::exp(-100*crease*crease)*(1-facing);
            float noise = grain(seed+uint32_t(start*2+px)*5639u+uint32_t(y)*103u);
            float sparkle = (noise-.5f)*10+(noise>.964f?12:0);
            for(int k=0;k<3;++k) {
                constexpr float light[3]={255,255,255}, blue[3]={140,174,202};
                p[k]=uint8_t(std::clamp(light[k]+(blue[k]-light[k])*shade+sparkle,0.f,255.f));
            }
            p[3]=uint8_t(255*coverage);
        }
    }
    return out;
}
}
