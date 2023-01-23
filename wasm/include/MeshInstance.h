#ifndef _MESH_INSTANCE_H_
#define _MESH_INSTANCE_H_ 1

#include "TObject.h"

class MeshInstance : public TObject{

    public:
        std::string mesh_name ;
        std::string owner ;
        glm::mat4 pose;
        Variant bone_data;
        bool bones_compressed;

        //delay in seconds to target for mesh instance interpolation (mostly for remote avatar pose smoothing)
        static constexpr double mesh_interpolation_delay = 1.0/90.0;

        MeshInstance();

        MeshInstance(glm::vec3 p, float r, const std::string& own, const std::string& name, const glm::mat4& m, const Variant& bones, bool compressed);

        ~MeshInstance() override;

        // Serialize this object, so it can be efficiently moved between timelines
        std::map<std::string,Variant> serialize() const override;

        // Set this object to data generated by its serialize method
        void set(std::map<std::string,Variant>& serialized) override;

        // Override this to provide an efficient deep copy of this object
        // If not overridden serialize and set will be used to copy your object (which will be inefficent)
        std::unique_ptr<TObject> deepCopy() override;

        // Override this function to provide logic for interpolation after rollback or extrapolation for slowly updating objects
        // If not overridden getObserved returns the raw value of the object
        std::unique_ptr<TObject> getObserved(double time, const std::weak_ptr<TObject> last_observed, double last_time) override;


        static std::unique_ptr<TObject> createObject(const Variant& serialized);

        static std::unique_ptr<TEvent> createEvent(const Variant& serialized);
        
};
#endif // #ifndef _MESH_INSTANCE_H_