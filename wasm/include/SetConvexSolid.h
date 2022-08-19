#ifndef _SET_CONVEX_SOLID_H_
#define _SET_CONVEX_SOLID_H_ 1

#include "TEvent.h"
#include "TObject.h"
#include "Variant.h"

#include <string>
#include <map>

class SetConvexSolid : public TEvent{

    public:
        glm::vec3 new_position;
        glm::vec3 new_velocity;
        glm::quat new_orientation;
        glm::vec3 new_angular_velocity;

        SetConvexSolid();

        SetConvexSolid(int solid_id, glm::vec3 p, glm::vec3 v, glm::quat o, glm::vec3 av);

        ~SetConvexSolid() override;

        // Serialize this event's data, so it can be efficiently moved between timelines
        std::map<std::string,Variant> serialize() const override;

        // Set this event to data generated by its serialize method
        void set(std::map<std::string,Variant>& serialized) override;

        // Runs the event
        // This is what you need to override to implement your application
        // To maintain causality run should only interact with dynamic data by using the privided methods:
        // get(id), getMutable(), addEvent, createObject, deleteObject, and getCollisions
        void run() override;

};
#endif // #ifndef _SET_CONVEX_SOLID_H_