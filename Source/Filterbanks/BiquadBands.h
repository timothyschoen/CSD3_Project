// Efficient implementation of biquad bandpass filter bank.
// This is the core computation for musical physical models ("modal models").
//
// This code takes 1.5 instructions to compute each floating-point bandpass filter;
// for a SHARC running at 3ns instruction cycle this is 4.5ns / biquad.
//
// Written by Lippold Haken of Haken Audio, 2010, using ADSP-21364 and VisualDSP++ C.
//
//  dmXmod - pointer to DM data memory
//  pmXmod - pointer to PM data memory
//  bfbXComputePairs - optimized computation of a bank of bandpass filters
//  bfbXCoef - coefficient computation for the bandpass filters

// CFDSP hardware-specific definitions.
#include "cfdsp21364.h"

// === Start of definitions for .h file
// The following structure definitions are shared by this code and by the calling code,
// so they should be moved to a common .h file.

// Data in dm.
typedef struct
{
    #define bfb4FilterSections  64      // number of modes (number of bandpass filter sections)
    float bfb_Y[2][bfb4FilterSections];         // save state: y[n-1] and y[n-2]
} DmX_bfb;

// Data in pm.
typedef struct
{
    // The bfb_C[] contains 3 coefficients for each biquad, with a pair of biquad's coefficients
    // interleaved so that biquads can be processed in pairs by the optimized SIMD assembly loop.
    float bfb_C[ 3 * bfb4FilterSections ];      // bfb filter coefficients, see comment above
} PmX_bfb;

// Pointers to data, and a samples counter.
DmX_bfb *dmXmod;
PmX_bfb *pmXmod;
int sampInFrame;                    // sample counter

#define sr 48000                    // sample rate in Hz
#define Pi    (3.14159265)
#define ABS(x) ( ((x)>(0)) ? (x) : -(x) )
void sinCos( float fRadians, float *fSin, float *fCos ); // see Lippold Haken's sinCos code snippet

// === End of definitions for .h file

// Parameter arrays for Biquad Filter Bank (modal physical modelling) synthesis.
//
// If P is the biquad pair number, and R=0/R=1 distinguishes the two biquads within the pair:
//  x[n]   coefficient for biquad k is at pmXmod->bfb_C[ 6*P + R + 0 ]; this is also -x[n-2] coefficient.
//  y[n-1] coefficient for biquad k is at pmXmod->bfb_C[ 6*P + R + 2 ]; this is scaled by 0.5 to avoid overflow
//  y[n-2] coefficient for biquad k is at pmXmod->bfb_C[ 6*P + R + 4 ]
// The first index to dmXmod->bfb_Y[] is based on lsbs of sample counter.
// The second index to dmXmod->bfb_Y[] is 2P+R.
// Note: R=0 always for even voices, R=1 always for odd voices.
#define bfb_Ynm1(P) &(dmXmod->bfb_Y[sampInFrame & 1][2*(P)])        // start in y[n-1][] array
#define bfb_Ynm2(P) &(dmXmod->bfb_Y[1-(sampInFrame & 1)][2*(P)])    // start in y[n-2][] array

void bfbXComputePairs(int biquadP, int pairs,
            float inDiff0,          // x[n]-x[n-2] for even biquads
            float inDiff1,          // x[n]-x[n-2] for odd biquads
            float *fSum0,           // output for even biquad sum
            float *fSum1)           // output for odd biquad sum
// Sum the output of a sequence of at least 2 biquad pairs.
// For each of the sequence of pairs, the first (even) biquad in each pair has a common input "input0",
// and the second (odd) biquad of each pair has a (possibly different) common input "input1".
{
    // Assmebly-coded bfb filter loop using floating-point math.
    // The core loop takes 3 instructions for 2 biquads, or 4.5ns per biquad.
    // We use the even-numbered biquads for one voice, the odd-numbered biquads for a second voice.
    asm(
    "#include <def21364.h>                          \n"
    "BIT SET MODE1 SIMD;                            \n"

    // For each biquad k:  (k=6*biquadP for R=0)
    //   y[k](n) = C0[k] * (x[k](n) - x[k](n-2))
    //           + C1[k] * 2 * y[k](n-1)
    //           + C2[k] * y[k](n-2)
    //
    // Register usage for biquad k (R==0):
    //   F0       = C0[k], C1[k], and C2[k]
    //   F4       = input0-old_input0 aka x[k](n)-x[k](n-2)
    //   F6       = y[k](n-1), y[k](n-2)
    //   F8       = scratch for computing y[k](n)
    //   F10      = sum of even biquad's y[](n) in each biquad pair
    //   F12      = scratch for computing y[k](n)
    //   F13      = final value of y[k-2](n)
    //   DM(I4)   = C0[k], C1[k], and C2[k]
    //   PM(I10)  = read y[k](n-2) write to y[k-2](n)
    //   PM(I12)  = read y[k](n-1)
    // Register usage for biquad k+1 (R==1):
    //   SF0      = C0[k+1], C1[k+1], and C2[k+1]
    //   SF4      = input1-old_input1 aka x[k+1](n)-x[k+1](n-2)
    //   SF6      = y[k+1](n-1), y[k+1](n-2)
    //   SF8      = scratch for computing y[k+1](n)
    //   SF10     = sum of odd biquad's y[](n) in each biquad pair
    //   SF12     = scratch for computing y[k+1](n)
    //   SF13     = final value of y[k-1](n)
    //   DM(I4+1) = C0[k+1], C1[k+1], and C2[k+1]
    //   PM(I10+1)= read y[k+1](n-2) write y[k-1](n)
    //   PM(I12+1)= read y[k+1](n-1)
    // Register usage for even and odd biquads:
    //   M4=M12=2
    //   M10=+4
    //   M11=-2
    //
    // The three instruction loop below computes two biquads simultaneously, for k and k+1,
    // in each SIMD loop execution.  The comments are written just for biquad k;
    // the biquad k+1 code and comments are implied by SIMD mode.

    // Set pointer increments.
    "      M4=2;M10=4;M11=-2;M12=2;                 \n"

    // Loop priming, with k=0 (and SIMD k=1).

        // Initialize F10=SF10=0                    fetch C0[k]
    "      R10=R10-R10,                             F0=DM(I4,M4);                           \n"

        // C0[k]*(x(n)-x(n-2))                      fetch C1[k]         fetch y[k](n-1)
    "      F8=F0*F4,                                F0=DM(I4,M4),       F6=PM(I12,M12);     \n"
        // C1[k] * y[k](n-1)                        fetch C2[k]         fetch y[k](n-2)
    "      F12=F0*F6,                               F0=DM(I4,M4),       F6=PM(I10,M12);     \n"
        // C2[k] * y[k](n-2)    biquad k sum        fetch C0[k+2]
    "      F12=F0*F6,           F8=F8+F12,          F0=DM(I4,M4);                           \n"

    // Main loop, for k = 2..K-2 by 2 (and SIMD for k = 1..K-1 by 2).
    "DO (PC, endx) UNTIL LCE;   \n"

        // C0[k]*(x(n)-x(n-2))  F13 is y[k-2](n)    fetch C1[k]         fetch y[k](n-1)
    "      F8=F0*F4,            F13=F8+F12,         F0=DM(I4,M4),       F6=PM(I12,M12);     \n"
        // C1[k] * y[k](n-1)    output sum          fetch C2[k]         fetch y[k](n-2)
    "      F12=F0*F6,           F10=F10+F13,        F0=DM(I4,M4),       F6=PM(I10,M11);     \n"
        // C2[k] * y[k](n-2)    biquad k sum        fetch C0[k+2]       save y[k-2](n)
    "endx: F12=F0*F6,           F8=F8+F12,          F0=DM(I4,M4),       PM(I10,M10)=F13;    \n"

        //                      F13 is y[k-2](n)
    "                           F13=F8+F12,                             modify(I10,M11);    \n"
        //                      output sum                              save y[k-2](n)
    "                           F10=F10+F13,                            PM(I10,M10)=F13;    \n"

    "BIT CLR MODE1 SIMD;                            \n"     // disable SIMD
    "      NOP;                                     \n"     // wait for SIMD disable

    // Output register list, each element: "=rN" (varName)
            :   "=R10" (*fSum0),                    // F10 has floating sum of even & odd biquads
                "=S10" (*fSum1)                     // SF10 has floating sum of even & odd biquads
    // Input register list, each element: "rN" (varName)
            :   "lcntr" (pairs-1),
                "R4" (inDiff0),                     // x[n]-x[n-2] for even biquads
                "S4" (inDiff1),                     // x[n]-x[n-2] for odd biquads
                "I4" (&pmXmod->bfb_C[6 * biquadP]), // coefficients for all biquads
                "I10" (bfb_Ynm2(biquadP)),          // y[n-2] for all biquads, updated to y[n]
                "I12" (bfb_Ynm1(biquadP))           // y[n-1] for all biquads
    // Clobbered register list:
    // We must avoid "do not use" regs, Compiler and Library Manual p.1-245.
    // We try to use mostly scratch regs, Compiler and Library Manual p.1-248.
            :   "r0", "r4", "r6", "r8", "r10", "r12", "r13",
                "i4", "i10", "i12",
                "m4", "m10", "m11", "m12" );
}

void bfbXCoef(int biquadP, int biquadR, float freq, float amp, float bw)
// Compute coefficients for a biquad filter section of the floating-point Biquad Filter Bank routine.
{
    int startingIndex = 6 * biquadP + biquadR;          // 6 * biquad pair number (+1 if second in pair)
    float *pC = &pmXmod->bfb_C[ startingIndex ];        // index biquad filter coefficients array
    float *y0 = &dmXmod->bfb_Y[0][2*biquadP+biquadR];   // filter memory for biquad
    float *y1 = &dmXmod->bfb_Y[1][2*biquadP+biquadR];   // filter memory for biquad

    // Is center frequency is below aliasing-cutoff limit?
    if (freq != 0.0 && freq < 0.5 * sr)
    {
        // Compute sin and cos of the mode frequency.
        float phase = freq * 2. * Pi / sr;
        float fSin, fCos;
        sinCos(phase, &fSin, &fCos); // efficient sine and cosine; see Lippold Haken's DSPrelated code snippet.

        // These formulas are adapted from Robert Bristow-Johnson BLT biquad web posting.
        // I use the BPF with "constant skirt gain, peak gain = Q", with amplitude scaling of input coefficients.
        // Compute intermediate parameters, alpha and beta.
        //      alpha = sin(w0)/(2*Q)
        //      beta  = 1/(1+alpha)
        //      y[n] = (beta * sin(w0)/2   )  *  (x[n] - x[n-2])
        //           + (beta * 2*cos(w0)   )  *  y[n-1]
        //           + (beta * (alpha - 1) )  *  y[n-2]
        // In addition, the input x-coefficients are scaled by the amplitude of the biquad --
        // I use **twice** the amplitude value.
        float alpha = fSin * bw / 2.0;      // this is sin/(2*q)
        float beta = 1.0 / (1.0 + alpha);
        pC[0] = amp * beta * fSin;          // x[n] coefficient, and -x[n-2] coefficient
        pC[2] = beta * 2. * fCos;           // y[n-1] coefficient
        pC[4] = beta * (alpha - 1.0);       // y[n-2] coefficient
    }
    else
    {
        // Frequency of bfb filter is beyond aliasing frequency,
        // or if we are about to start a new note.
        // Zero bfb filter coefficients and zero out the filter memory.
        pC[0] = pC[2] = pC[4] = 0;
        *y0 = *y1 = 0;
    }
}
