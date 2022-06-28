#ifndef _MOVE_BOUNCING_BALL_H_
#define _MOVE_BOUNCING_BALL_H_ 1

#include "TEvent.h"
#include "TObject.h"
#include "Variant.h"

#include <string>
#include <map>


class MoveBouncingBall : public TEvent{

    public:

        static float friction ; // amount of velocity lost per second

        MoveBouncingBall();

        MoveBouncingBall(int moving_object, double time_step);

        MoveBouncingBall(double time_step);

        ~MoveBouncingBall() override;

        // Serialize this event's data, so it can be efficiently moved between timelines
        std::map<std::string,Variant> serialize() const override;

        // Set this event to data generated by its serialize method
        void set(std::map<std::string,Variant>& serialized) override;

        // Runs the event
        // This is what you need to override to implement your application
        // To maintain causality run should only interact with dynamic data by using the privided methods:
        // get(id), getMutable(), addEvent, createObject, deleteObject, and getCollisions
        void run() override;

        double interval ;

        static glm::vec3 getClosestPoint(std::pair<glm::vec3,glm::vec3> lines, glm::vec3 point);
        
};
#endif // #ifndef _MOVE_BOUNCING_BALL_H_