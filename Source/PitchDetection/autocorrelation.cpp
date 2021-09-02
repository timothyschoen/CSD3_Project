#include "pitch_detection.h"
#include <algorithm>
#include <complex>
//#include <ffts/ffts.h>

#include "tools/kiss_fftr.h"

#include <numeric>
#include <vector>

template <typename T>
void
util::acorr_r(const std::vector<T> &audio_buffer, pitch_alloc::BaseAlloc<T> *ba)
{
    if (audio_buffer.size() == 0)
        throw std::invalid_argument("audio_buffer shouldn't be empty");

    /*std::transform(audio_buffer.begin(), audio_buffer.begin() + ba->N,
        ba->out_im.begin(), [](T x) -> std::complex<T> {
            return std::complex(x, static_cast<T>(0.0));
        }); */
    
    kiss_fftr(ba->fft_forward, audio_buffer.data(), (kiss_fft_cpx*)ba->out_im.data());
    
    //ffts_execute(ba->fft_forward, ba->out_im.data(), ba->out_im.data());

    std::complex<float> scale = {
        1.0f / (float)(ba->N * 2), static_cast<T>(0.0)};
    
    for (int i = 0; i < ba->N; ++i)
        ba->out_im[i] *= std::conj(ba->out_im[i]) * scale;

    kiss_fftri(ba->fft_backward, (kiss_fft_cpx*)ba->out_im.data(), ba->out_real.data());
}

template void
util::acorr_r<float>(const std::vector<float> &audio_buffer,
    pitch_alloc::BaseAlloc<float> *ba);
