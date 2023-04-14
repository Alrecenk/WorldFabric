#ifndef _PARTITIONING_RADIANCE_FIELD_H_
#define _PARTITIONING_RADIANCE_FIELD_H_ 1

#include <vector>
#include <memory>
#include <utility>
#include "glm/glm.hpp"
#include "OptimizationProblem.h"


class PartitioningRadianceField  : public OptimizationProblem{
    public:

        float offset = 0.0f ;

        struct Partition{
            // if locked partition and value will not change with optimization steps
            bool locked = false;
            bool sharp = false;

            // two planes define partition
            //goes to first child if ray hits N1*x + d1 = 0 first
            // or second child if it hits N2*x +d2 = 0 first  (sort of, not really cause there's a +1)
            glm::vec3 N1, N2 ;
            float d1, d2;
            
            bool leaf = false; // whether this is a leaf
            float value = 0.5 ; // if a leaf the value it outputs

            float h = 1.0 ; // scale factor on activation function for homotopy
            // s(x) = e^(hx) / (e^(hx) + 1)
        };

        struct TrainingRay{
            glm::vec3 p, v ; // ray p +v*t
            float y ; // expected output value
        };

        // element 0 is root
        // parent of a node at index n is floor((n-1)/2)
        // children are 2n+1 and 2n+2
        std::vector<Partition> field;

        std::vector<TrainingRay> training_set;

        PartitioningRadianceField();


        PartitioningRadianceField(int depth);

        void initializeNode(int k);

        // functions for navigating the tree
        int firstChild(int n);
        int secondChild(int n);
        int parent(int n);

        float sigmoid(float x, float h);

        float sigmoidprime(float x, float h);

        // Returns the value for a given ray starting from the root
        double computeValue(glm::vec3 &p, glm::vec3 &v);

        // Returns the value for a ray starting from the given node
        double computeValue(int n, glm::vec3 &p, glm::vec3 &v);

        // gets the squared error of a given data point
        float getError(glm::vec3 &p, glm::vec3 &v, float y);

        // adds the gradient from a given data point to an accumulating gradient
        // p,v is the ray mapping to value y
        void accumulateGradient(std::vector<float> &gradient, glm::vec3 &p, glm::vec3 &v, float y);

        // accumulates the gradient for a given node and training point
        // p,v is the ray mapping to value y, mult is the multiplier passed from the parent node
        //returns the value at the given node
        float accumulateGradient(int n, std::vector<float> &gradient, glm::vec3 &p, glm::vec3 &v, float mult);


         // Return the current x for this object
        std::vector<float> getX() override;

        // Set this object to a given x
        void setX(std::vector<float> x) override;

        // Adds a training ray to the data set tha twill be used for computing the OptimizationProblem error and gradient
        void addTrainingRay(glm::vec3 &p, glm::vec3 &v, float y);

        // Returns the error to be minimized for the given input
        double error(std::vector<float> x) override;

        // Returns the gradient of error about a given input
        std::vector<float> gradient(std::vector<float> x) override;
        

        void lockRow(int row);
        void unlockRow(int row);
        void setHomotopy(int row, float h);
        void addRow();
        int numRows();

void setLeavestoTrainingAverage(double strength) ;

};
#endif // #ifndef _PARTITIONING_RADIANCE_FIELD_H_