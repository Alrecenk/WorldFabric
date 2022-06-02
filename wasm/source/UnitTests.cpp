#include "UnitTests.h"
#include "Variant.h"
#include "glm/vec3.hpp"
#include "TObject.h"
#include "TEvent.h"
#include "EventQueue.h"
#include "ObjectHistory.h"

#include "MovingObject.h"
#include "ChangeVelocity.h"
#include <unordered_map>
#include <vector>
#include <map>
#include <string>


using std::vector;
using glm::vec3;
using std::string;
using std::map;

bool UnitTests::runAll(){
    return UnitTests::createAndMoveCircle();
}

bool UnitTests::createAndMoveCircle(){
    Timeline t = Timeline();
    t.setGenerators(&UnitTests::createEvent, &UnitTests::createObject);
    t.createObject(MovingObject(vec3(1,0,0),vec3(0,2,0), 3.0f), 1.0);
    t.run(2.0);
    vector<int> ob = t.updateObservables();
    if(ob.size() == 1){
        printf("Object 0:\n");
        Variant(t.getLastObserved(ob[0]).serialize()).printFormatted(); 
    }else{
        return false;
    }

    return true ;

}

TObject* UnitTests::createObject(Variant& serialized){
    auto map = serialized.getObject() ;
    MovingObject o = MovingObject();
    o.set(map);
    return &o;
}

TEvent* UnitTests::createEvent(Variant& serialized){
    auto map = serialized.getObject() ;
    ChangeVelocity e = ChangeVelocity();
    e.set(map);
    return &e;
}