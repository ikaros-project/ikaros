#include "ikaros.h"

using namespace ikaros;

class Delta: public Module
{
    parameter alpha;
    parameter inverse;

    matrix cs;
    matrix us;
    matrix cr;
    matrix weights;

    void Init()
    {
        Bind(alpha, "alpha");
        Bind(inverse, "inverse");

        Bind(cs, "CS");
        Bind(us, "US");
        Bind(cr, "CR");
    }

    void Tick()
    {
        if (!cs.connected() || !us.connected())
            return;

        if (weights.rank() == 0)
            weights = matrix(cs.shape());

        float us_sum = us.sum();
        float response = cs.dot(weights);
        float delta = alpha.as_float() * (us_sum - response);

        if (delta > 0)
            weights.apply(cs, [delta](float w, float stimulus) { return w + stimulus * delta; });

        if (inverse.as_bool())
            cr = std::max(0.0f, response - us_sum);
        else
            cr = response;
    }
};

INSTALL_CLASS(Delta)
