#ifndef _IMAGE_FIELD_H_
#define _IMAGE_FIELD_H_ 1

#include <vector>
#include <memory>
#include <utility>
#include "glm/glm.hpp"


class ImageField{
    public:

        float expadjust[256];

        struct Node{
            bool leaf = true;

            //if leaf, color
            glm::dvec3 color ;

            //if branch, plane (children are determined by array position)
            glm::dvec2 N;
            double d ; 

            double step = 1.0f; // used for momentum on training
        };

        struct TrainingPixel{
            glm::dvec2 x = glm::dvec2(0,0) ; // pixel position
            glm::dvec3 y = glm::dvec3(0,0,0) ; // expected output value
        };

        struct TrainingChance{
            glm::dvec2 x ; // pixel position
            double p; // target probability
            double w; // weight
        };

        /*
        struct Cluster{
            TrainingPixel total;
            std::vector<TrainingPixel> data;
        }
        */

        // element 0 is root
        // parent of a node at index n is floor((n-1)/2)
        // children are 2n+1 and 2n+2
        std::vector<Node> field;

        std::vector<TrainingPixel> training_set;

        ImageField();


        ImageField(int depth);

        void initializeNode(int k);

        void initializeNode(int k, const std::vector<TrainingPixel>& data) ;

        // functions for navigating the tree
        int firstChild(int n);
        int secondChild(int n);
        int parent(int n);

        // random number between 0 and 1
        float random();

        double sigmoid(double x);

        double sigmoidprime(double x);

        // Returns the proibilistic value for a given ray starting from the root
        glm::dvec3 value(const glm::dvec2 &x);

        // Returns the proibilistic value for a ray starting from the given node
        glm::dvec3 value(int n, const glm::dvec2 &x);

        // Returns the most likely color for a given ray starting from the root
        glm::dvec3 color(const glm::dvec2 &x);

        // Returns the most likely color  for a ray starting from the given node
        glm::dvec3 color(int n, const glm::dvec2 &x);

        // Adds a training ray to the data set tha twill be used for computing the OptimizationProblem error and gradient
        void addTrainingPixel(const glm::dvec2 &x, const glm::dvec3 &y);

        // Run a training step on the given node after running it on its children
        void train(int n, const std::vector<TrainingPixel>& data) ;

        // Run a training cycle on all nodes from leaves to root
        void train(int samples, int sample_size);

        // Run a training cycle on all nodes from leaves to root
        void train();

        void addRow();
        int numRows();

        glm::dvec2 normalizeImagePosition(const int x, const int y, const int size);

        // error for moving planes with probability points
        double error(std::vector<TrainingChance>& data, glm::dvec3 Nd);

        // N is first two components d is third component
        glm::dvec3 gradient(std::vector<TrainingChance>& data, glm::dvec3 Nd);

/*
        void addToCluster(Cluster &c, TrainingPixel &p);

        float clusterDistance(Cluster &c, TrainingPixel &p);

        clusterTrain(int n, const std::vector<TrainingPixel>& data);
*/

float fastexp(const float x) const;

//build correction table to improve result in region of interest
//if region of interest is large enough then improves result everywhere
void buildexptable(double min, double max, double step);


};
#endif // #ifndef _IMAGE_FIELD_H_