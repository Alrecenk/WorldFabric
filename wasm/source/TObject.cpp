#include "TObject.h"

std::unique_ptr<TObject>(*TObject::generateTypedTObject)(const Variant& serialized) ; 

TObject::TObject(){
}


// Override this to provide an efficient deep copy of this object
// If not overridden serialize and set will be used to copy your object (which will be inefficent)
std::unique_ptr<TObject> TObject::deepCopy(){
    return TObject::generateTypedTObject(Variant(serialize()));
}

// Override this function to provide logic for interpolation after rollback or extrapolation for slowly updating objects
// If not overridden getObserved returns the raw value of the object
std::unique_ptr<TObject> TObject::getObserved(double time, const std::weak_ptr<TObject> last_observed, double last_time){
    return deepCopy();
}

void TObject::print() const{
    Variant(serialize()).printFormatted();
}