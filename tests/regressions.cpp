#include "../src/Powder.hpp"
#include <cassert>
#include <limits>
#include <iostream>

int main() {
    const float nan=std::numeric_limits<float>::quiet_NaN();
    const float infinity=std::numeric_limits<float>::infinity();
    snow::Scene invalidScreen(nan,-3,1);
    invalidScreen.configure({nan,infinity,nan,false,nan});
    assert(invalidScreen.settings.speed==1);
    assert(invalidScreen.settings.density==1);
    invalidScreen.advance(nan);
    invalidScreen.advance(infinity);
    invalidScreen.sync({{0,0,0,100,100},{1,nan,0,100,100},{2,0,0,-100,100},{3,0,0,infinity,10}});
    assert(invalidScreen.banks.empty());

    for (bool autumn : {false,true}) {
        snow::Scene scene(1000,700,25,autumn);
        scene.sync({{1,20,120,380,100},{2,440,180,380,100}});
        const auto originalVersion=scene.geometryVersion;
        scene.sync(scene.frontToBack);
        assert(scene.geometryVersion==originalVersion);
        for (int frame=0;frame<1800;++frame) scene.advance(1.f/30);
        size_t before=scene.flakes.size();
        auto floorBefore=scene.floor.depth;
        scene.shakeWindows();
        assert(scene.flakes.size()>before);
        assert(scene.flakes.size()<=2000);
        assert(scene.floor.depth==floorBefore);
        for (const auto &entry:scene.banks) {
            assert(entry.second.leaves.empty());
            for (float depth:entry.second.depth) assert(depth==0);
        }
        before=scene.flakes.size();
        scene.shakeWindows();
        assert(scene.flakes.size()==before);
        const auto revision=scene.floor.revision;
        scene.configure({1,1,0,false});
        assert(scene.floor.revision>revision);
        assert(scene.floor.leaves.empty());
        for (float depth:scene.floor.depth) assert(depth==0);

        // Stress the airborne budget when many large piles spill together.
        if (autumn) {
            for(auto &entry:scene.banks) {
                for(int i=0;i<1500;++i)
                    entry.second.leaves.push_back({{float(i%300),0,0,70,2,1,false},10});
            }
            scene.shakeWindows();
            assert(scene.flakes.size()==2000);
        }
    }

    snow::Scene sparse(1000,10000,12,true),dense(1000,10000,12,true);
    sparse.flakes.clear();dense.flakes.clear();
    auto settings=sparse.settings;settings.density=.2f;sparse.configure(settings);
    settings.density=2.5f;dense.configure(settings);
    for(int frame=0;frame<90;++frame){sparse.advance(1.f/30);dense.advance(1.f/30);}
    assert(dense.flakes.size()>6*sparse.flakes.size());

    snow::Scene boundedScene(1000,700,42);
    // Fractional window resizing must resize the collision grid as well.
    boundedScene.sync({{1,0,100,80,100}});
    for(int i=1;i<=100;++i) {
        float width=80+i*.25f;
        boundedScene.sync({{1,0,100,width,100}});
        assert(boundedScene.banks.at(1).depth.size()==size_t(std::ceil(width/8)));
    }
    std::vector<snow::Window> windows;
    for(int i=1;i<=100;++i) windows.push_back({uint32_t(i),10,100,200,100});
    windows.push_back(windows.front());
    boundedScene.sync(windows);
    assert(boundedScene.banks.size()==64);
    assert(snow::paintPowder(boundedScene.floor,20,10,{}).rgba.empty());
    std::cout << "Bounds, invalid inputs, density and particle-budget regressions passed\n";
}
