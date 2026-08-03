#pragma once

#include "ikaros.h"

namespace ikaros
{
    class ProjectiveGeometry
    {
    public:
        explicit ProjectiveGeometry(int maxPoints);

        bool Homography(const matrix & source, const matrix & target, matrix & result);
        bool Similarity(const matrix & source, const matrix & target, matrix & result) const;
        static bool Project(const matrix & transform, float x, float y,
                            float & targetX, float & targetY);

    private:
        int maxPoints_;
        matrix normalizedSource_;
        matrix normalizedTarget_;
        matrix design_;
        matrix designTranspose_;
        matrix normal_;
        matrix u_;
        matrix singular_;
        matrix vt_;
        matrix normalizedTransform_;
        matrix sourceNormalization_;
        matrix targetNormalization_;
        matrix targetNormalizationInverse_;
        matrix temporary_;
    };
}
