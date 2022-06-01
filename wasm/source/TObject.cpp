#include "TObject.h"

// Override this to provide an efficient deep copy of this object
// If not overridden serialize and set will be used to copy your object (which will be inefficent)
TObject TObject::deepCopy(){
    return TObject::generateTypedTObject(serialize());
}

// Override this function to provide logic for interpolation after rollback or extrapolation for slowly updating objects
// If not overridden getObserved returns the raw value of the object
TObject TObject::getObserved(const TObject& last_observed){
    return deepCopy();
}