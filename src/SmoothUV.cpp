#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <emmintrin.h>

#include <VapourSynth.h>
#include <VSHelper.h>


typedef struct SmoothUVData {
    VSNodeRef *clip;
    const VSVideoInfo *vi;

    int radius;
    int threshold;
    bool interlaced;
    bool interlaced_exists;

    uint16_t divin[256];
} SmoothUVData;


static inline void sum_pixels_SSE2(const uint8_t *srcp, uint8_t *dstp, const int stride,
                    const int diff, const int width, const int height,
                    const __m128i &thres,
                    const uint16_t *divinp) {

    __m128i zeroes = _mm_setzero_si128();

    __m128i sum = zeroes;
    __m128i count = zeroes;

    __m128i center_pixel = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *)srcp),
                                             zeroes);

    srcp = srcp - diff;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x <= width; x++) {
            __m128i neighbour_pixel = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *)(srcp + x)),
                                                        zeroes);

            __m128i abs_diff = _mm_or_si128(_mm_subs_epu16(center_pixel, neighbour_pixel),
                                            _mm_subs_epu16(neighbour_pixel, center_pixel));

             // Absolute difference less than thres
            __m128i mask = _mm_cmpgt_epi16(thres, abs_diff);

            // Sum up the pixels that meet the criteria
            sum = _mm_adds_epu16(sum,
                                 _mm_and_si128(neighbour_pixel, mask));

            // Keep track of how many pixels are in the sum
            count = _mm_sub_epi16(count, mask);
        }

        srcp += stride;
    }

    sum = _mm_adds_epu16(sum,
                         _mm_srli_epi16(count, 1));

    __m128i divres = zeroes;

    for (int i = 0; i < 8; i++) {
        int e = _mm_extract_epi16(count, i);
        divres = _mm_insert_epi16(divres, divinp[e], i);
    }

    // Now multiply (divres/65536)
    sum = _mm_mulhi_epu16(sum, divres);
    sum = _mm_packus_epi16(sum, sum);
    _mm_storel_epi64((__m128i *)dstp, sum);
}


template <bool interlaced>
static void smoothN_SSE2(int radius,
                         const uint8_t* origsrc, uint8_t* origdst,
                         int stride, int w, int h,
                         const int threshold,
                         const uint16_t* divin) {
    const uint8_t *srcp = origsrc;
    const uint8_t *srcp2 = origsrc + stride;
    uint8_t *dstp = origdst;
    uint8_t *dstp2 = origdst + stride;

    const int SqrtTsquared = (int)sqrt((threshold * threshold) / 3);

    const __m128i thres = _mm_set1_epi16(SqrtTsquared);

    int h2 = h;

    if (interlaced) {
        stride *= 2;
        h2 >>= 1;
    }

    for (int y = 0; y < h2; y++) {
        int y0 = (y < radius) ? y : radius;

        int yn = (y < h2 - radius) ? y0 + radius + 1
                                   : y0 + (h2 - y);

        if (interlaced)
            yn--;

        int offset = y0 * stride;

        for (int x = 0; x < w; x += 8) {
            int x0 = (x < radius) ? x : radius;

            int xn = (x + 7 + radius < w - 1) ? x0 + radius + 1
                                              : x0 + w - x - 7;

            sum_pixels_SSE2(srcp + x, dstp + x,
                            stride,
                            offset + x0,
                            xn, yn,
                            thres,
                            divin);

            if (interlaced) {
                sum_pixels_SSE2(srcp2 + x, dstp2 + x,
                                stride,
                                offset + x0,
                                xn, yn,
                                thres,
                                divin);
            }
        }

        dstp += stride;
        srcp += stride;
        dstp2 += stride;
        srcp2 += stride;
    }

    if (interlaced && h % 1) {
        int yn = radius;

        int offset = radius * stride;

        for (int x = 0; x < w; x += 8) {
            int x0 = (x < radius) ? x : radius;

            int xn = (x + 7 + radius < w - 1) ? x0 + radius + 1
                                              : x0 + w - x - 7;

            sum_pixels_SSE2(srcp + x, dstp + x,
                            stride,
                            offset + x0,
                            xn, yn,
                            thres,
                            divin);
        }
    }
}


static void VS_CC smoothUVInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi) {
    (void)in;
    (void)out;
    (void)core;

    SmoothUVData *d = (SmoothUVData *) *instanceData;

    vsapi->setVideoInfo(d->vi, 1, node);
}


static const VSFrameRef *VS_CC smoothUVGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi) {
    (void)frameData;

    const SmoothUVData *d = (const SmoothUVData *) *instanceData;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->clip, frameCtx);
    } else if (activationReason == arAllFramesReady) {
        const VSFrameRef *src = vsapi->getFrameFilter(n, d->clip, frameCtx);

        int width = vsapi->getFrameWidth(src, 0);
        int height = vsapi->getFrameHeight(src, 0);

        // Reuse the luma plane, allocate memory for the chroma planes.
        const VSFrameRef *plane_src[3] = { src, nullptr, nullptr };
        int planes[3] = { 0 };

        VSFrameRef *dst = vsapi->newVideoFrame2(d->vi->format, width, height, plane_src, planes, src, core);

        const VSMap *props = vsapi->getFramePropsRO(src);
        int err;
        int64_t field_based = vsapi->propGetInt(props, "_FieldBased", 0, &err);

        bool interlaced = field_based == 1 || field_based == 2;
        if (d->interlaced_exists)
            interlaced = d->interlaced;

        for (int plane = 1; plane < 3; plane++) {
            const uint8_t *srcp = vsapi->getReadPtr(src, plane);
            uint8_t *dstp = vsapi->getWritePtr(dst, plane);

            int stride = vsapi->getStride(src, plane);
            width = vsapi->getFrameWidth(src, plane);
            height = vsapi->getFrameHeight(src, plane);

            (interlaced ? smoothN_SSE2<true>
                        : smoothN_SSE2<false>)(d->radius, srcp, dstp, stride, width, height, d->threshold, d->divin);
        }

        vsapi->freeFrame(src);

        return dst;
    }

    return nullptr;
}


static void VS_CC smoothUVFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
    (void)core;

    SmoothUVData *d = (SmoothUVData *)instanceData;

    vsapi->freeNode(d->clip);

    free(d);
}


static void VS_CC smoothUVCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
    (void)userData;

    SmoothUVData d;
    memset(&d, 0, sizeof(d));

    int err;

    d.radius = int64ToIntS(vsapi->propGetInt(in, "radius", 0, &err));
    if (err)
        d.radius = 3;

    d.threshold = int64ToIntS(vsapi->propGetInt(in, "threshold", 0, &err));
    if (err)
        d.threshold = 270;

    d.interlaced = !!vsapi->propGetInt(in, "interlaced", 0, &err);
    d.interlaced_exists = !err;


    if (d.radius < 1 || d.radius > 7) {
        vsapi->setError(out, "SmoothUV: radius must be between 1 and 7 (inclusive).");
        return;
    }

    if (d.threshold < 0 || d.threshold > 450) {
        vsapi->setError(out, "SmoothUV: threshold must be between 0 and 450 (inclusive).");
        return;
    }


    d.clip = vsapi->propGetNode(in, "clip", 0, nullptr);
    d.vi = vsapi->getVideoInfo(d.clip);

    if (!d.vi->format ||
        d.vi->format->bitsPerSample != 8 ||
        d.vi->format->colorFamily != cmYUV) {
        vsapi->setError(out, "SmoothUV: only 8 bit YUV with constant format supported.");
        vsapi->freeNode(d.clip);
        return;
    }


    for (int i = 1; i < 256; i++)
        d.divin[i] = (uint16_t)std::min((int)(65536.0 / i + 0.5), 65535);


    SmoothUVData *data = (SmoothUVData *)malloc(sizeof(SmoothUVData));
    *data = d;

    vsapi->createFilter(in, out, "SmoothUV", smoothUVInit, smoothUVGetFrame, smoothUVFree, fmParallel, 0, data, core);
}


VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin) {
    configFunc("com.nodame.smoothuv", "smoothuv", "SmoothUV is a spatial derainbow filter", VAPOURSYNTH_API_VERSION, 1, plugin);
    registerFunc("SmoothUV",
                 "clip:clip;"
                 "radius:int:opt;"
                 "threshold:int:opt;"
                 "interlaced:int:opt;"
                 , smoothUVCreate, 0, plugin);
}
