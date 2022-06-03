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
    printf("createAndMoveCircle\n");
    printf("Initializing timeline...\n");
    Timeline t = Timeline();
    printf("setting generators...\n");
    t.setGenerators(&UnitTests::createEvent, &UnitTests::createObject);
    printf("init circle...\n");
    std::unique_ptr<MovingObject> o = std::make_unique<MovingObject>(vec3(1,0,0),vec3(0,2,0), 3.0f) ;
    printf("create object event...\n");
    t.createObject(std::move(o), std::unique_ptr<TEvent>(nullptr), 1.0);
    printf("running...\n");
    t.run(2.0);
    printf("updating observables...\n");
    vector<int> ob = t.updateObservables();
    if(ob.size() == 1){
        printf("Object 0:\n");
        Variant(t.getLastObserved(ob[0])->serialize()).printFormatted(); 
    }else{
        printf("Error no objects!\n");
        return false;
    }

    return true ;

}

std::unique_ptr<TObject> UnitTests::createObject(const Variant& serialized){
    auto map = serialized.getObject() ;
    auto o = std::make_unique<MovingObject>();
    o->set(map);
    return std::move(o);
}

std::unique_ptr<TEvent> UnitTests::createEvent(const Variant& serialized){
    auto map = serialized.getObject() ;
    auto e = std::make_unique<ChangeVelocity>();
    e->set(map);
    return std::move(e);
}