import vapoursynth as vs


def RainbowSmooth(clip, radius=3, lthresh=0, hthresh=220):
    core = vs.get_core()

    lderain = clip

    if lthresh > 0:
        lderain = clip.smoothuv.SmoothUV(radius=radius, threshold=lthresh, interlaced=False)

    hderain = clip.smoothuv.SmoothUV(radius=radius, threshold=hthresh, interlaced=False)

    mask = core.std.Expr(clips=[clip.std.Maximum(planes=0), clip.std.Minimum(planes=0)], expr=["x y - 90 > 255 x y - 255 90 / * ?", "", ""])

    if hthresh > lthresh:
        return core.std.MaskedMerge(clipa=lderain, clipb=hderain, mask=mask, planes=[1, 2], first_plane=True)
    else:
        return lderain
