#include "../src/Powder.hpp"
#include <cassert>
#include <numeric>
#include <iostream>
using namespace snow;
float mass(const Bank& b) {return std::accumulate(b.depth.begin(),b.depth.end(),0.f);}
int main(){
 Scene s(400,400,42); s.flakes.clear();
 Window a{1,0,50,200,150}, b{2,100,100,200,80};
 s.sync({a,b});
 assert(!s.exposed(2,150,100));assert(s.exposed(2,250,100));
 s.flakes.push_back({150,46,0,60,2,0,false});
 s.advance(.04); assert(mass(s.banks.at(1))>0 && mass(s.banks.at(2))==0);
 a.x+=5; s.sync({a,b});
 assert(mass(s.banks.at(1))==0 && std::any_of(s.flakes.begin(),s.flakes.end(),[](Flake f){return f.loose;}));
 for(int i=0;i<100;++i)s.advance(1.f/30);
 assert(mass(s.floor)>0);
 s.flakes.clear(); s.flakes.push_back({250,95,0,80,2,0,false}); s.advance(.05);
 assert(mass(s.banks.at(2))>0); s.sync({a}); assert(!s.banks.count(2));
 for(int i=0;i<7200;++i){s.advance(1.f/30);assert(s.flakes.size()<=2000);}
 for(float d:s.floor.depth)assert(d>=0&&d<=36);
 for(float d:s.banks.at(1).depth)assert(d>=0&&d<=32);
 Bank full{{99,0,100,80,55},std::vector<float>(10,12),0,{},0};
 std::vector<uint8_t> visible(10,1);visible[5]=0;
 auto painted=paintPowder(full,0,80,visible);auto again=paintPowder(full,0,80,visible);
 assert(painted.rgba==again.rgba);
 for(int y=0;y<stripHeight;++y)assert(!painted.rgba[(y*160+83)*4+3]);
 assert(painted.rgba[((sky-3)*2*160+61)*4+3]==255);
 assert(painted.rgba[((sky+2)*2*160+61)*4+3]>0);
 std::cout<<"Physics checks passed\n";
}
