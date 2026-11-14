// Offline raw-f32 reference renderer for controlled Virtual Vocaid A/B measurements.
// INPUT/OUTPUT: little-endian stereo interleaved float32, no WAV header.
// This CLI never runs inside the VST3 audio callback.
#include "reference_reverb.hpp"
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc,char** argv) {
  if(argc!=8) {
    std::cerr<<"usage: vocaid_ab_renderer sampleRate size damping width mix input.f32 output.f32\n";
    return 2;
  }
  const double sr=std::atof(argv[1]);
  a125::VocaidReferenceReverb rv;
  auto p=rv.controls();
  p.size=static_cast<float>(std::atof(argv[2]));
  p.damping=static_cast<float>(std::atof(argv[3]));
  p.width=static_cast<float>(std::atof(argv[4]));
  p.mix=static_cast<float>(std::atof(argv[5]));
  rv.setControls(p); // snap to settled controls before the first sample
  if(!rv.prepare(sr)) {std::cerr<<"invalid sample rate\n";return 2;}
  std::ifstream in(argv[6],std::ios::binary|std::ios::ate);
  if(!in){std::cerr<<"failed opening input\n";return 2;}
  const auto size=in.tellg();
  if(size<0 || (static_cast<std::size_t>(size)%(2*sizeof(float)))!=0){
    std::cerr<<"input must be stereo interleaved float32\n";return 2;
  }
  in.seekg(0);
  std::ofstream out(argv[7],std::ios::binary|std::ios::trunc);
  if(!out){std::cerr<<"failed opening output\n";return 2;}
  constexpr std::size_t n=256;
  std::array<float,2*n> data{};
  std::array<float,n> l{},r{},ol{},orr{};
  std::size_t remain=static_cast<std::size_t>(size)/(2*sizeof(float));
  while(remain) {
    const auto take=std::min(remain,n);
    if(!in.read(reinterpret_cast<char*>(data.data()),static_cast<std::streamsize>(take*2*sizeof(float))))
      return 3;
    for(std::size_t i=0;i<take;++i){l[i]=data[2*i];r[i]=data[2*i+1];}
    rv.processStereo(l.data(),r.data(),ol.data(),orr.data(),take);
    for(std::size_t i=0;i<take;++i){data[2*i]=ol[i];data[2*i+1]=orr[i];}
    out.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(take*2*sizeof(float)));
    if(!out)return 3;
    remain-=take;
  }
  std::cout<<"Rendered "<<(static_cast<std::size_t>(size)/(2*sizeof(float)))
           <<" stereo frames @ "<<sr<<" Hz (candidate reference DSP; not Vocaid capture)\n";
  return 0;
}
