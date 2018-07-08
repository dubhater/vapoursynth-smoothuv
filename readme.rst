Description
===========

SmoothUV is a spatial derainbow filter.

The luma is returned unchanged.

Currently only x86 systems are supported.

This is a port of the SmoothUV filter from the SmoothUV Avisynth
plugin. The Avisynth plugin also includes the SSHiQ filter, which was
not ported.


RainbowSmooth is a script which adds edge detection to SmoothUV. It is
a port of the Avisynth function rainbow_smooth().


Usage
=====
::

    smoothuv.SmoothUV(clip clip, [int radius=3, int threshold=270, bint interlaced])


Parameters:
    *clip*
        A clip to process. It must have constant format and it must be
        8 bit YUV.

    *radius*
        Radius. Must be between 1 and 7.

        Larger values smooth more.

        Default: 3.

    *threshold*
        Threshold. Must be between 0 and 450.

        Larger values smooth more.

        Default: 270.

    *interlaced*
        Each frame's "_FieldBased" property is examined to determine if
        the frame should be considered interlaced. If the "_FieldBased"
        property is 0 or it doesn't exist, the frame is considered not
        interlaced.

        Set this parameter to override the automatic detection of
        interlaced frames.


::

    RainbowSmooth(clip, radius=3, lthresh=0, hthresh=220, mask="original")


Parameters:
    *clip*
        The clip to process.

    *radius*
        Radius passed to SmoothUV.

        Default: 3.

    *lthresh*, *hthresh*
        The low and the high smoothing thresholds. Use smaller values
        for safer processing. The masking is only used for hthresh,
        so if you set lthresh greater than hthresh lthresh will be the
        overall threshold and no masking will be used (fastest). But if
        you set lthresh=0 you disable the basic chroma smoothing and
        use only the chroma smoothing on edges.

        Default: lthresh=0, hthresh=220.

    *mask*
        Edge mask to use. It can be either a clip, or one of the
        following strings: "original", "prewitt", "sobel", "tcanny",
        "fast_sobel", "kirsch", "retinex_edgemask".

        "original" is the edge mask used by the original
        rainbow_smooth() Avisynth function.

        The latter three require kagefunc.py.

        Default: "original".

Compilation
===========

::

    mkdir build && cd build
    meson ../
    ninja

Or:

::

    ./autogen.sh
    ./configure
    make

Meson runs faster than autogen.sh and configure.


License
=======

GNU GPL, like the Avisynth plugin.
