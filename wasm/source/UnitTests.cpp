#include "UnitTests.h"
#include "Variant.h"
#include "glm/vec3.hpp"
#include "TObject.h"
#include "TEvent.h"
#include "EventQueue.h"
#include "ObjectHistory.h"

#include "MovingObject.h"
#include "ChangeVelocity.h"
#include "MoveObject.h"

#include <unordered_map>
#include <vector>
#include <map>
#include <string>
#include <memory>


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
    t.time_kept = 10000; // Domn't delete things for this unit test
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
        printf("Object 0: %d\n", ob[0]);
        Variant(t.getLastObserved(ob[0])->serialize()).printFormatted(); 
    }else{
        printf("Error no object found after creation!\n");
        return false;
    }

    printf("Adding first move event...\n");
    t.addEvent(std::make_unique<MoveObject>(ob[0], 1.0), 2.0) ;
    t.run(5.0);

    ob = t.updateObservables();
    if(ob.size() == 1){
        printf("Object 0: %d\n", ob[0]);
        Variant(t.getLastObserved(ob[0])->serialize()).printFormatted(); 
    }else{
        printf("Error no object found!");
        return false;
    }

    printf("Adding change direction event...\n");
    t.addEvent(std::make_unique<ChangeVelocity>(ob[0], vec3(0,0,1.0)), 5.5) ;
    t.run(10.0);

    ob = t.updateObservables();
    if(ob.size() == 1){
        printf("Object 0: %d\n", ob[0]);
        Variant(t.getLastObserved(ob[0])->serialize()).printFormatted(); 
    }else{
        printf("Error no object found!");
        return false;
    }

    printf("Adding retroactive change direction event...\n");
    t.addEvent(std::make_unique<ChangeVelocity>(ob[0], vec3(0,0,-1.0)), 5.5) ;
    t.run(10.0);


    ob = t.updateObservables();
    if(ob.size() == 1){
        printf("Object 0: %d\n", ob[0]);
        Variant(t.getLastObserved(ob[0])->serialize()).printFormatted(); 
    }else{
        printf("Error no object found!");
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