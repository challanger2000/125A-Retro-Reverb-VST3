// Offline raw-f32 reference renderer for controlled Virtual Vocaid A/B measurements.
// INPUT/OUTPUT: little-endian stereo interleaved float32, no WAV header.
// This CLI never runs inside the VST3 audio callback.
#include "revmodel.hpp"
#include "freeverb_mix.hpp"
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
  if(sr!=44100.0) {
    std::cerr<<"The historical Jezar Freeverb tunings are fixed for 44100 Hz; A/B comparison is restricted to this rate.\n";
    return 2;
  }
  revmodel rv;
  rv.setroomsize(static_cast<float>(std::atof(argv[2])));
  rv.setdamp(static_cast<float>(std::atof(argv[3])));
  rv.setwidth(static_cast<float>(std::atof(argv[4])));
  a125::applyMix(rv,static_cast<float>(std::atof(argv[5])));
  rv.mute();
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
    rv.processreplace(l.data(),r.data(),ol.data(),orr.data(),static_cast<long>(take),1);
    for(std::size_t i=0;i<take;++i){data[2*i]=ol[i];data[2*i+1]=orr[i];}
    out.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(take*2*sizeof(float)));
    if(!out)return 3;
    remain-=take;
  }
  std::cout<<"Rendered "<<(static_cast<std::size_t>(size)/(2*sizeof(float)))
           <<" stereo frames @ "<<sr<<" Hz (original Jezar Freeverb DSP; not Vocaid capture)\n";
  return 0;
}
