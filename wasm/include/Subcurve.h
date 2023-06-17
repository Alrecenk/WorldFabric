#ifndef _SUBCURVE_H_
#define _SUBCURVE_H_ 1

#include "Variant.h"
#include "glm/vec3.hpp"

#include <map>
#include <vector>
#include "OptimizationProblem.h"


class Subcurve : public OptimizationProblem{

    public:

        struct Target{
            float x=0;
            float y=0;
            float time=0 ;
            float t=0 ;
            float weight=0 ;
        };

        std::vector<Target> targets ;

        int degree ;
        float t0=0,t1=1 ;// stores time of t=0 and t=1 to convert to/from normalized time

        std::vector<glm::vec2> poly ; /// poly[n]*x^n

        Subcurve(int poly_degree);

        glm::vec2 valueNormalized(float t);

        glm::vec2 value(float t);

        void addTarget(float x, float y, float time);

        // Return the current x for this object
        std::vector<float> getX() override;

        // Set this object to a given x
        void setX(std::vector<float> x) override;

        // Returns the error to be minimized for the given input
        double error(std::vector<float> x) override;

        // Returns the gradient of error about a given input
        std::vector<float> gradient(std::vector<float> x) override;

        float bestT(glm::vec2 target, float min_t, float max_t, int steps, int iter);

        // Uses targets to solve for the curve
        void solve();

        
};
#endif // #ifndef _SUBCURVE_H_