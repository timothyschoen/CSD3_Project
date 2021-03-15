/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <random>

const float ANTI_DENORMAL_FLOAT = 1e-15f;

// hilbert transformer: generates two orthogonal output signals
// max. phase error for f = 0.00015..0.49985fs is 0.017 degrees
class Hilbert
{
public:
    Hilbert() {
        FloatVectorOperations::fill(s, 0, 33);
        adidx = 0;
        GetAntiDenormalTable(adtab,16);
    }
    
    void GetAntiDenormalTable(float* d, int size)
    {
        // First create an instance of an engine.
        std::random_device rnd_device;
        // Specify the engine and distribution.
        std::mt19937 mersenne_engine {rnd_device()};
        std::uniform_real_distribution<float> dist {-1.0, 1.0};

        auto gen = [&dist, &mersenne_engine](){
                       return dist(mersenne_engine);
                   };

        std::generate(d, d + size, gen);
        
        for(int i = 0; i < size; i++) {
            d[i] = abs(d[i]) + 0.9f;
            for (int j = 1; j < size; j += 2) d[j] *= -1.0f;
            d[i] += 1.1f;
            d[i] *= 0.5f*ANTI_DENORMAL_FLOAT;
        }

    }
    
    void process(float* in, float* out1, float* out2, int samples) {
        
        float adn[2];
        adn[0] = adtab[adidx]; adn[1] = adtab[adidx+1]; adidx = (adidx+2) & 0xe;
        float xa,xb,adin;
        for (int i=0; i<samples; i++) {
            adin = in[i] + adn[i&1];

            // out1 filter chain: 8 allpasses + 1 unit delay
            xa = s[1] - 0.999533593f*adin;        s[1] = s[0];
            s[0] = adin + 0.999533593f*xa;
            xb = s[3] - 0.997023120f*xa;        s[3] = s[2];
            s[2] = xa + 0.997023120f*xb;
            xa = s[5] - 0.991184054f*xb;        s[5] = s[4];
            s[4] = xb + 0.991184054f*xa;
            xb = s[7] - 0.975597057f*xa;        s[7] = s[6];
            s[6] = xa + 0.975597057f*xb;
            xa = s[9] - 0.933889435f*xb;        s[9] = s[8];
            s[8] = xb + 0.933889435f*xa;
            xb = s[11] - 0.827559364f*xa;        s[11] = s[10];
            s[10] = xa + 0.827559364f*xb;
            xa = s[13] - 0.590957946f*xb;        s[13] = s[12];
            s[12] = xb + 0.590957946f*xa;
            xb = s[15] - 0.219852059f*xa;        s[15] = s[14];
            s[14] = xa + 0.219852059f*xb;
            out1[i] = s[32]; s[32] = xb;

            // out2 filter chain: 8 allpasses
            xa = s[17] - 0.998478404f*adin;       s[17] = s[16];
            s[16] = adin + 0.998478404f*xa;
            xb = s[19] - 0.994786059f*xa;        s[19] = s[18];
            s[18] = xa + 0.994786059f*xb;
            xa = s[21] - 0.985287169f*xb;        s[21] = s[20];
            s[20] = xb + 0.985287169f*xa;
            xb = s[23] - 0.959716311f*xa;        s[23] = s[22];
            s[22] = xa + 0.959716311f*xb;
            xa = s[25] - 0.892466594f*xb;        s[25] = s[24];
            s[24] = xb + 0.892466594f*xa;
            xb = s[27] - 0.729672406f*xa;        s[27] = s[26];
            s[26] = xa + 0.729672406f*xb;
            xa = s[29] - 0.413200818f*xb;        s[29] = s[28];
            s[28] = xb + 0.413200818f*xa;
            out2[i] = s[31] - 0.061990080f*xa;    s[31] = s[30];
            s[30] = xa + 0.061990080f*out2[i];
        }
    }
    
private:
    float s[33];
    int adidx;
    float adtab[16];
};


/* SIMD Implementation
 
 
 using SIMD = dsp::SIMDRegister<float>;

 // hilbert transformer: generates two orthogonal output signals
 // max. phase error for f = 0.00015..0.49985fs is 0.017 degrees
 class Hilbert
 {
 public:
     Hilbert() {
         for(int i = 0; i < 33; i++)  s[i] = SIMD(0.0);

         adidx = 0;
         GetAntiDenormalTable(adtab,16);
     }
     
     void GetAntiDenormalTable(SIMD* d, int size)
     {
         // First create an instance of an engine.
         std::random_device rnd_device;
         // Specify the engine and distribution.
         std::mt19937 mersenne_engine {rnd_device()};
         std::uniform_real_distribution<float> dist {-1.0, 1.0};

         auto gen = [&dist, &mersenne_engine](){
                        return dist(mersenne_engine);
                    };

         std::generate(d, d + size, gen);
         
         for(int i = 0; i < size; i++) {
             d[i] = SIMD::abs(d[i]) + SIMD(0.9f);
             for (int j = 1; j < size; j += 2) d[j] *= SIMD(-1.0f);
             d[i] += SIMD(1.1f);
             d[i] *= SIMD(0.5f*ANTI_DENORMAL_FLOAT);
         }

     }
     
     
     void process(dsp::AudioBlock<SIMD> in, dsp::AudioBlock<SIMD> out1, dsp::AudioBlock<SIMD> out2, int samples) {
         
         SIMD adn[2];
         adn[0] = adtab[adidx]; adn[1] = adtab[adidx+1]; adidx = (adidx+2) & 0xe;
         
         SIMD xa,xb,adin;
         for (int i=0; i<samples; i++) {
             adin = in.getSample(0, i) + adn[i&1];

             // out1 filter chain: 8 allpasses + 1 unit delay
             xa = s[1] -  SIMD(0.999533593f)*adin;        s[1] = s[0];
             s[0] = adin + SIMD(0.999533593f)*xa;
             xb = s[3] - SIMD(0.997023120f)*xa;        s[3] = s[2];
             s[2] = xa + SIMD(0.997023120f)*xb;
             xa = s[5] - SIMD(0.991184054f)*xb;        s[5] = s[4];
             s[4] = xb + SIMD(0.991184054f)*xa;
             xb = s[7] - SIMD(0.975597057f)*xa;        s[7] = s[6];
             s[6] = xa + SIMD(0.975597057f)*xb;
             xa = s[9] - SIMD(0.933889435f)*xb;        s[9] = s[8];
             s[8] = xb + SIMD(0.933889435f)*xa;
             xb = s[11] - SIMD(0.827559364f)*xa;        s[11] = s[10];
             s[10] = xa + SIMD(0.827559364f)*xb;
             xa = s[13] - SIMD(0.590957946f)*xb;        s[13] = s[12];
             s[12] = xb + SIMD(0.590957946f)*xa;
             xb = s[15] - SIMD(0.219852059f)*xa;        s[15] = s[14];
             s[14] = xa + SIMD(0.219852059f)*xb;
             out1.setSample(0, i, s[32]); s[32] = xb;

             // out2 filter chain: 8 allpasses
             xa = s[17] - SIMD(0.998478404f)*adin;       s[17] = s[16];
             s[16] = adin + SIMD(0.998478404f)*xa;
             xb = s[19] - SIMD(0.994786059f)*xa;        s[19] = s[18];
             s[18] = xa + SIMD(0.994786059f)*xb;
             xa = s[21] - SIMD(0.985287169f)*xb;        s[21] = s[20];
             s[20] = xb + SIMD(0.985287169f)*xa;
             xb = s[23] - SIMD(0.959716311f)*xa;        s[23] = s[22];
             s[22] = xa + SIMD(0.959716311f)*xb;
             xa = s[25] - SIMD(0.892466594f)*xb;        s[25] = s[24];
             s[24] = xb + SIMD(0.892466594f)*xa;
             xb = s[27] - SIMD(0.729672406f)*xa;        s[27] = s[26];
             s[26] = xa + SIMD(0.729672406f)*xb;
             xa = s[29] - SIMD(0.413200818f)*xb;        s[29] = s[28];
             s[28] = xb + SIMD(0.413200818f)*xa;
             out2.setSample(0, i, s[31] - SIMD(0.061990080f)*xa);    s[31] = s[30];
             s[30] = xa + SIMD(0.061990080f)*out2.getSample(0, i);
         }
     }
     
 private:
     SIMD s[33];
     int adidx;
     SIMD adtab[16];
 };


 
 
 */
