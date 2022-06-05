#ifndef _UNIT_TESTS_H_
#define _UNIT_TESTS_H_ 1

#include "Variant.h"
#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"
#include <memory>

class UnitTests{
public:
    static bool runAll();

    static bool createAndMoveCircle();

    static bool checkSimpleTimeWarp();

    static bool checkCollisionRollback();

    static std::unique_ptr<TObject> createObject(const Variant& serialized);

    static std::unique_ptr<TEvent> createEvent(const Variant& serialized);
};
#endif // #ifndef _UNIT_TESTS_H_