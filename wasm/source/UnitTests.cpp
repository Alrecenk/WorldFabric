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
    string success = UnitTests::createAndMoveCircle() ? "passed" : "failed" ;
    printf("createAndMoveCircle : %s\n", success.c_str());
    success = UnitTests::checkSimpleTimeWarp() ? "passed" : "failed" ;
    printf("checkSimpleTimeWarp : %s\n", success.c_str());
    success = UnitTests::checkCollisionRollback() ? "passed" : "failed" ;
    printf("checkCollisionRollback : %s\n", success.c_str());
    success = UnitTests::checkClearHistory() ? "passed" : "failed" ;
    printf("checkClearHistory : %s\n", success.c_str());
    return true; 
}

void UnitTests::expect(bool& success, bool condition, std::string error_message){
    if(!condition){
        printf("%s\n",error_message.c_str());
    }
    success &= condition;
}

void UnitTests::expectNear(bool& success, glm::vec3 a, glm::vec3 b, float dist, std::string error_message){
    bool condition = glm::length(a-b) <= dist ;
    if(!condition){
        printf("%s\n",error_message.c_str());
        printf("(%f,%f,%f) != (%f,%f,%f)\n", a.x,a.y,a.z,b.x,b.y,b.z);
    }
    success &= condition;
}

// TODO make these proper unit tests that check correctness and don't spam the console if they're passing
bool UnitTests::createAndMoveCircle(){
    bool s = true ;
    //printf("---createAndMoveCircle---\n");
    //printf("Initializing timeline...\n");
    Timeline t = Timeline();
    //printf("setting generators...\n");
    t.setGenerators(&UnitTests::createEvent, &UnitTests::createObject);
    //printf("init circle...\n");
    std::unique_ptr<MovingObject> o = std::make_unique<MovingObject>(vec3(1,0,0),vec3(0,2,0), 3.0f) ;
    //printf("create object event...\n");
    t.createObject(std::move(o), std::unique_ptr<TEvent>(nullptr), 1.0);
    //printf("running...\n");
    t.run(2.0);
    //printf("updating observables...\n");
    vector<int> ob = t.updateObservables();
    UnitTests::expect(s, ob.size() == 1, "Object not observable after creation!");


    //printf("Adding first move event...\n");
    t.addEvent(std::make_unique<MoveObject>(ob[0], 1.0), 2.0) ;
    t.run(5.0);

    ob = t.updateObservables();

    UnitTests::expectNear(s, ((MovingObject*)t.getLastObserved(ob[0]))->position, vec3(1,6,0), 0.01, "Circle did not move correctly!");


    //printf("Adding change direction event...\n");
    t.addEvent(std::make_unique<ChangeVelocity>(ob[0], vec3(0,0,1.0)), 5.5) ;
    t.run(10.0);

    ob = t.updateObservables();

    UnitTests::expectNear(s, ((MovingObject*)t.getLastObserved(ob[0]))->position, vec3(1,8,4), 0.01, "Circle did not change velocity correctly!");

    //printf("Adding retroactive change direction event...\n");
    t.addEvent(std::make_unique<ChangeVelocity>(ob[0], vec3(0,0,-1.0)), 5.5) ;
    t.run(10.0);


    ob = t.updateObservables();

    UnitTests::expectNear(s, ((MovingObject*)t.getLastObserved(ob[0]))->position, vec3(1,8,-4), 0.01, "Circle did not retroactively change velocity correctly!");


    return s ;

}


bool UnitTests::checkSimpleTimeWarp(){
    //printf("---checkSimpleTimeWarp---\n");
    bool s = true ;
    
    Timeline t = Timeline();
    t.info_speed = 10;

    //printf("Initialized timeline with %f info speed.\n", t.info_speed);

    //printf("setting generators...\n");
    t.setGenerators(&UnitTests::createEvent, &UnitTests::createObject);
    //printf("init vantage at (0,0,0) at time 0.0...\n");
    std::unique_ptr<MovingObject> v = std::make_unique<MovingObject>(vec3(0,0,0),vec3(0,0,0), 1.0f) ;
    t.createObject(std::move(v), std::unique_ptr<TEvent>(nullptr), 0.0);
    //printf("running...\n");
    t.run(1);
    
    vector<int> ob = t.updateObservables();

    
    t.vantage_id = ob[0]; // set the cantage point
    UnitTests::expectNear(s, ((MovingObject*)t.getLastObserved(t.vantage_id ))->position, vec3(0,0,0), 0.01,
     "Vantage point did not initialize correctly!");


    // Create an object at the distance of the info speed so it should take 2 time to appear in observables
    //printf("Attempting to create an object at (10,0,0) from vantage at time 1.0\n");
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
    //printf("Second object observed at time %f \n", time );
    
    double found_time = time ;

    //Variant(t.getLastObserved(ob[0])->serialize()).printFormatted(); 
    //Variant(t.getLastObserved(ob[1])->serialize()).printFormatted(); 

    UnitTests::expect(s,(found_time - 3.0f)<0.0001, "Observation of created object with time warp is at wrong time!");

    

    bool reached = false ;
    while(!reached){
        time+=0.1;
        t.run(time);
        ob = t.updateObservables();
        reached = fabs(t.getLastObserved(ob[0])->position.x - t.getLastObserved(ob[1])->position.x)<0.06 ;
    }
    UnitTests::expect(s, (float)time == 12.0f, "Collision time of object in time warp is incorrect!");
    UnitTests::expect(s, (10.0f/(time-found_time) - 10.0f/9.0f)<0.0001, "Perceived speed change under time warp is incorrect!");
    //printf("Objects met at time %f , perceived speed was %f f\n", time, 10.0/(time-found_time) );
    //Variant(t.getLastObserved(ob[0])->serialize()).printFormatted(); 
    //Variant(t.getLastObserved(ob[1])->serialize()).printFormatted(); 

    return s ;
}


bool UnitTests::checkCollisionRollback(){
    bool s = true ;
    Timeline t = Timeline();

    std::unique_ptr<MovingObject> a = std::make_unique<MovingObject>(vec3(0,0,0),vec3(1,0,0), 1.0f) ;
    std::unique_ptr<MoveObject> a_move = std::make_unique<MoveObject>(0.5) ;
    a_move->stop_on_hit = true;
    t.createObject(std::move(a), std::move(a_move), 1.0);
    //printf("A created!\n");
    t.run(1.1);
    int a_id = -1 ;
    vector<int> ob = t.updateObservables();
    a_id = ob[0]; 
    //printf("A_id : %d!\n", a_id);
    std::unique_ptr<MovingObject> b = std::make_unique<MovingObject>(vec3(5,5,0),vec3(0,-1,0), 1.0f) ;
    std::unique_ptr<MoveObject> b_move = std::make_unique<MoveObject>(0.5) ;
    b_move->stop_on_hit = false;
    t.createObject(std::move(b), std::move(b_move) , 1.0);

    
    double time = 1.1 ;
    t.run(time);
   
    ob = t.updateObservables();
    int b_id = ob[0] == a_id ? ob[1] : ob[0];
    //printf("B created! %d \n", b_id);

    UnitTests::expectNear(s, ((MovingObject*)t.getLastObserved(a_id ))->position, vec3(0.5,0,0), 0.01,
        "A did not initialize correctly!");

    UnitTests::expectNear(s, ((MovingObject*)t.getLastObserved(b_id ))->position, vec3(5,4.5,0), 0.01,
        "B did not initialize correctly!");

    //printf("Searching for collisions!\n");
    bool a_stopped = false;
    while(!a_stopped && time < 10.0){
        time+=0.1;
        t.run(time);
        ob = t.updateObservables();
        MovingObject* ao = (MovingObject*)t.getLastObserved(a_id) ;
        a_stopped =  glm::length(ao->velocity) < 0.01;
    }
    //printf("A stopped at time : %f \n", time);
    UnitTests::expect(s, (time - 5.1)<0.01, "A did not collide and stop at correct time!");
    t.run(10.0);
    ob = t.updateObservables();
    /*
    printf("A at time 10 : \n");
    t.getLastObserved(a_id)->print() ;
    printf("B at time 10 : \n");
    t.getLastObserved(b_id)->print() ;
    */
    UnitTests::expectNear(s, ((MovingObject*)t.getLastObserved(a_id ))->position, vec3(4.5,0,0), 0.01,
        "A at incorrect position after collision");

    //printf("Adding retroactive change direction event that prevent collisions and running to time 10...\n");
    t.addEvent(std::make_unique<ChangeVelocity>(b_id, vec3(0,0,0)), 1.01) ;
    t.run(10.0);
    ob = t.updateObservables();
    /*
    printf("A at time 10 : \n");
    t.getLastObserved(a_id)->print() ;
    printf("B at time 10 : \n");
    t.getLastObserved(b_id)->print() ;
    */
    UnitTests::expectNear(s, ((MovingObject*)t.getLastObserved(a_id ))->position, vec3(9,0,0), 0.01,
        "A at incorrect position  after rolled back non collision");

    //printf("Adding retroactive change direction event that makes collision happen again and running to time 10...\n");
    t.addEvent(std::make_unique<ChangeVelocity>(b_id, vec3(0,-1,0)), 1.02) ;
    t.run(10.0);
    ob = t.updateObservables();
    /*
    printf("A at time 10 : \n");
    t.getLastObserved(a_id)->print() ;
    printf("B at time 10 : \n");
    t.getLastObserved(b_id)->print() ;
    */

    UnitTests::expectNear(s, ((MovingObject*)t.getLastObserved(a_id ))->position, vec3(4.5,0,0), 0.01,
        "A at incorrect position after rolled back recollision");

    return s ;
}

bool UnitTests::checkClearHistory(){
    //printf("---checkSimpleTimeWarp---\n");
    bool s = true ;
    
    Timeline t = Timeline();
    
    for(int k=0;k<10;k++){
        std::unique_ptr<MovingObject> o = std::make_unique<MovingObject>(vec3(k*100,k*1000%77,0),vec3((k*2)%3 - 1,k%3 - 1,0), 1.0f) ;
        std::unique_ptr<MoveObject> o_move = std::make_unique<MoveObject>(0.5) ;
        t.createObject(std::move(o), std::move(o_move) , 1.0);
    }
    t.run(1.0);
    expect(s, t.events.events.size() == 20, "Wrong number of events after first step!");
    //printf("%d\n",(int)t.events.events.size());

    double time = 10;
    double clear_time = 5;

    
    t.run(time);
    expect(s, t.events.events.size() == 200, "Wrong number of events after running!");
    //printf("%d\n",(int)t.events.events.size());

    expect(s, t.objects[1].history.size() == 19, "Wrong number of object instances after running!");
    //printf("%d\n",(int)t.objects[1].history.size());

    TObject* o1 = t.objects[1].get(clear_time) ;

    t.clearHistoryBefore(clear_time);
    expect(s, t.events.events.size() == 200, "Wrong number of events after first clear!");
    //printf("%d\n",(int)t.events.events.size());

    TObject* o1c = t.objects[1].get(clear_time) ;
    expect(s, o1c == o1, "Object at clear time changed after clear!");

    expect(s, t.objects[1].history.size() == 11, "Wrong number of object instances after first clear!");
    //printf("%d\n",(int)t.objects[1].history.size());

    t.run(time+1);
    while(time < 100){
        time+=10;
        clear_time+=10;
        t.run(time);
        t.clearHistoryBefore(clear_time);

        expect(s, t.events.events.size() == 310, "Wrong number of events after clear!");
        //printf("%d\n",(int)t.events.events.size());

        expect(s, t.objects[2].history.size() == 11, "Wrong number of object instances after clear!");
        //printf("%d\n",(int)t.objects[1].history.size());
    }


    t.vantage_id = 1 ;
    t.info_speed = 100;

    // Make sure time warped objects won't be deleted when they may be accessed
    while(time < 200){
        time+=10;
        clear_time+=10;
        t.run(time);

        vector<int> ob = t.updateObservables();
        vector<glm::vec3> positions ;
        for(int k=0;k<ob.size();k++){
            positions.push_back(t.getLastObserved(ob[k])->position);
        }

        t.clearHistoryBefore(clear_time);

        ob = t.updateObservables();
        for(int k=0;k<ob.size();k++){
            vec3 c_pos = t.getLastObserved(ob[k])->position ;
            //printf("pos %d: %f, %f, %f\n", ob[k], c_pos.x, c_pos.y, c_pos.z);
            expectNear(s,positions[k], c_pos, 0.001,"Object moved after clear history!");
        }

        expect(s, t.events.events.size() == 310, "Wrong number of events after time warp clear!");
        //printf("%d\n",(int)t.events.events.size());

        expect(s, t.objects[2].history.size() <= 11, "Too many object instances after time warp clear!");
        //printf("%d\n",(int)t.objects[2].history.size());
    }


    return s ;
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