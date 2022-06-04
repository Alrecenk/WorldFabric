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
    //return UnitTests::createAndMoveCircle();
    return UnitTests::checkSimpleTimeWarp();
}

bool UnitTests::createAndMoveCircle(){
    printf("---createAndMoveCircle---\n");
    printf("Initializing timeline...\n");
    Timeline t = Timeline();
    t.time_kept = 10000; // Don't delete things for this unit test
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


bool UnitTests::checkSimpleTimeWarp(){
    printf("---checkSimpleTimeWarp---\n");
    
    Timeline t = Timeline();
    t.time_kept = 10000;
    t.info_speed = 10;

    printf("Initialized timeline with %f info speed.\n", t.info_speed);

    printf("setting generators...\n");
    t.setGenerators(&UnitTests::createEvent, &UnitTests::createObject);
    printf("init vantage at (0,0,0) at time 0.0...\n");
    std::unique_ptr<MovingObject> v = std::make_unique<MovingObject>(vec3(0,0,0),vec3(0,0,0), 1.0f) ;
    t.createObject(std::move(v), std::unique_ptr<TEvent>(nullptr), 0.0);
    printf("running...\n");
    t.run(1);
    
    vector<int> ob = t.updateObservables();
    if(ob.size() == 1){
        printf("Vantage id: %d\n", ob[0]);
        Variant(t.getLastObserved(ob[0])->serialize()).printFormatted(); 
        t.vantage_id = ob[0]; // set the cantage point
    }else{
        printf("Error no object found after creation!\n");
        return false;
    }

    // Create an object at the distance of the info speed so it should take 2 time to appear in observables
    printf("Attempting to create an object at (10,0,0) from vantage at time 1.0\n");
    std::unique_ptr<MovingObject> a = std::make_unique<MovingObject>(vec3(10,0,0),vec3(-1.0,0,0), 1.0f) ;
    t.createObject(std::move(a), std::make_unique<MoveObject>(0.1), 1.0);
    double time = 1.0; 
    bool found_new = false;
    while(!found_new){
        time+=0.1;
        t.run(time);
        ob = t.updateObservables();
        found_new = ob.size() > 1 ;
    }
    printf("Second object observed at time %f \n", time );
    double found_time = time ;
    Variant(t.getLastObserved(ob[0])->serialize()).printFormatted(); 
    Variant(t.getLastObserved(ob[1])->serialize()).printFormatted(); 

    bool reached = false ;
    while(!reached){
        time+=0.1;
        t.run(time);
        ob = t.updateObservables();
        reached = fabs(t.getLastObserved(ob[0])->position.x - t.getLastObserved(ob[1])->position.x)<0.06 ;
    }
    printf("Objects met at time %f , perceived speed was %f\n", time, 10.0/(time-found_time) );
    Variant(t.getLastObserved(ob[0])->serialize()).printFormatted(); 
    Variant(t.getLastObserved(ob[1])->serialize()).printFormatted(); 



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