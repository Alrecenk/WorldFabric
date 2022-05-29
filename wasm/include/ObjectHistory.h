#ifndef _OBJECTHISTORY_H_
#define _OBJECTHISTORY_H_ 1

#include "Variant.h"
class ObjectHistory{

    public:

        //Returns the latest version of this oject that is observable from the given vantage point and time
        const TObject& getObserved(vec3 vantage, double time);
        
};
#endif // #ifndef _OBJECTHISTORY_H_