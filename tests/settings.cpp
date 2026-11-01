#include "../src/Snow.hpp"
#include <cassert>
#include <numeric>
float mass(const snow::Bank &b){return std::accumulate(b.depth.begin(),b.depth.end(),0.f);}
int main(){
 for(bool autumn:{false,true}){
  snow::Scene s(320,200,21,autumn);
  snow::Window w{1,100,80,120,60};
  s.sync({w});
  s.flakes.clear();
  s.flakes.push_back({35,20,0,60,2,0,false});s.configure({.5f,1,0,true});
  s.advance(.03);float slow=s.flakes[0].y-20;
  s.flakes[0]={35,20,0,60,2,0,false};s.configure({2,1,80,true});s.advance(.03);
  assert(s.flakes[0].y-20>3.5f*slow && s.flakes[0].x>37);
  s.configure({1,1,0,true});
  for(int i=0;i<1200;++i)s.advance(.04);
  assert(mass(s.floor)>0 && mass(s.banks.at(1))>0);
  auto retained=mass(s.banks.at(1)); s.configure({1,1,0,false});
  assert(mass(s.floor)==0 && s.floor.leaves.empty() && mass(s.banks.at(1))==retained);
  for(int i=0;i<240;i++) s.advance(.05);
  assert(mass(s.floor)==0);
  s.configure({1,0,0,true});
  for(int i=0;i<240;i++) s.advance(.05);
  assert(mass(s.floor)==0 && mass(s.banks.at(1))==0 && s.banks.at(1).leaves.empty());
  s.configure({1,1,0,true});for(int i=0;i<600;i++)s.advance(1.f/30);
  assert(mass(s.floor)>0 && mass(s.banks.at(1))>0);
  if(autumn){
   size_t rest=0,air=0,total=0;
   s.drawPile(s.floor,0,[&](auto){++rest;});
   s.drawPile(s.banks.at(1),0,[&](auto){++rest;});
   s.drawAirborne([&](auto){++air;});
   s.drawLeaves([&](auto){++total;}); assert(total==rest+air && rest>0);
   auto count=s.banks.at(1).leaves.size(); w.x+=12;s.sync({w});assert(count && s.banks.at(1).leaves.empty());
  }
 }
}
