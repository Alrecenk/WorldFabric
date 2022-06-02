#ifndef _UNIT_TESTS_H_
#define _UNIT_TESTS_H_ 1

#include "Variant.h"
#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"

class UnitTests{
public:
    static bool runAll();

    static bool createAndMoveCircle();

    static TObject createObject(Variant& serialized);

    static TEvent createEvent(Variant& serialized);
};
#endif // #ifndef _UNIT_TESTS_H_