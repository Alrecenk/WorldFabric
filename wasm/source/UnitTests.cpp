#include "UnitTests.h"
#include "Variant.h"
#include "glm/vec3.hpp"
#include "TObject.h"
#include "TEvent.h"
#include "CreateObject.h"

#include "BouncingBall.h"
#include "MoveBouncingBall.h"
#include "ChangeBallVelocity.h"

#include <unordered_map>
#include <vector>
#include <map>
#include <string>
#include <memory>


using std::vector;
using glm::vec3;
using std::string;
using std::map;
using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;

bool UnitTests::runAll(){
    string success = UnitTests::createAndMoveCircle() ? "passed" : "failed" ;
    printf("createAndMoveCircle : %s\n", success.c_str());
    
    success = UnitTests::checkSimpleTimeWarp() ? "passed" : "failed" ;
    printf("checkSimpleTimeWarp : %s\n", success.c_str());

    success = UnitTests::checkClearHistory() ? "passed" : "failed" ;
    printf("checkClearHistory : %s\n", success.c_str());

    success = UnitTests::checkCollisionRollback() ? "passed" : "failed" ;
    printf("checkCollisionRollback : %s\n", success.c_str());
   
    success = UnitTests::checksimpleTimelineSync() ? "passed" : "failed" ;
    printf("checksimpleTimelineSync : %s\n", success.c_str());
    
    success = UnitTests::checkSyncExistingObject() ? "passed" : "failed" ;
    printf("checkSyncExistingObject : %s\n", success.c_str());

    success = UnitTests::checkCreateObjectNotification() ? "passed" : "failed" ;
    printf("checkCreateObjectNotification : %s\n", success.c_str());
    
    return true; // TODO return whether all succeeded
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

int UnitTests::countHistory(shared_ptr<TObject> recent){
    shared_ptr<TObject> instant = recent;
    if(!instant){
        return 0 ;
    }
    double count = 1 ;
    while(instant->prev){
        instant = instant->prev ;
        count++;
    }
    return count ;
}

// TODO make these proper unit tests that check correctness and don't spam the console if they're passing
bool UnitTests::createAndMoveCircle(){
    bool s = true ;
    //printf("---createAndMoveCircle---\n");
    //printf("Initializing timeline...\n");
    Timeline t = Timeline(&BouncingBall::createEvent, &BouncingBall::createObject);
    t.history_kept = 1000;
    t.base_age = 100 ;
    //printf("setting generators...\n");
    //printf("init circle...\n");
    std::unique_ptr<BouncingBall> o = std::make_unique<BouncingBall>(vec3(1,0,0),vec3(0,2,0), 3.0f) ;
    //printf("create object event...\n");
    t.createObject(std::move(o), std::unique_ptr<TEvent>(nullptr), 1.0);
    //printf("running...\n");
    t.run(2.0);
    //printf("updating observables...\n");
    vector<int> ob = t.updateObservables();
    UnitTests::expect(s, ob.size() == 1, "Object not observable after creation!");


    //printf("Adding first move event...\n");
    t.addEvent(std::make_unique<MoveBouncingBall>(ob[0], 1.0), 2.0) ;
    t.run(5.0);

    ob = t.updateObservables();

    UnitTests::expectNear(s, t.getLastObserved(ob[0])->position, vec3(1,8,0), 0.01, "Circle did not move correctly!");


    //printf("Adding change direction event...\n");
    t.addEvent(std::make_unique<ChangeBallVelocity>(ob[0], vec3(0,0,1.0)), 5.5) ;
    t.run(10.0);

    ob = t.updateObservables();

    UnitTests::expectNear(s, t.getLastObserved(ob[0])->position, vec3(1,8,5), 0.01, "Circle did not change velocity correctly!");

    //printf("Adding retroactive change direction event...\n");
    t.addEvent(std::make_unique<ChangeBallVelocity>(ob[0], vec3(0,0,-1.0)), 5.5) ;
    t.run(10.0);


    ob = t.updateObservables();

    UnitTests::expectNear(s, t.getLastObserved(ob[0])->position, vec3(1,8,-5), 0.01, "Circle did not retroactively change velocity correctly!");


    return s ;

}


bool UnitTests::checkSimpleTimeWarp(){
    //printf("---checkSimpleTimeWarp---\n");
    bool s = true ;
    
    Timeline t = Timeline(&BouncingBall::createEvent, &BouncingBall::createObject);
    t.info_speed = 10;
    t.history_kept = 1000;
    t.base_age = 100 ;
    t.max_time_warp = 100;

    //printf("Initialized timeline with %f info speed.\n", t.info_speed);

    //printf("setting generators...\n");
    //printf("init vantage at (0,0,0) at time 0.0...\n");
    std::unique_ptr<BouncingBall> v = std::make_unique<BouncingBall>(vec3(0,0,0),vec3(0,0,0), 0.0001f) ;
    t.createObject(std::move(v), std::unique_ptr<TEvent>(nullptr), 0.0);
    //printf("running...\n");
    t.run(1);
    
    vector<int> ob = t.updateObservables();

    
    t.vantage_id = ob[0]; // set the cantage point
    UnitTests::expectNear(s, t.getLastObserved(t.vantage_id)->position, vec3(0,0,0), 0.01,
     "Vantage point did not initialize correctly!");


    // Create an object at the distance of the info speed so it should take 2 time to appear in observables
    //printf("Attempting to create an object at (10,0,0) from vantage at time 1.0\n");
    std::unique_ptr<BouncingBall> a = std::make_unique<BouncingBall>(vec3(10,0,0),vec3(-1.0,0,0), 0.0001f) ;
    t.createObject(std::move(a), std::make_unique<MoveBouncingBall>(0.1), 1.0);
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
    Timeline t = Timeline(&BouncingBall::createEvent, &BouncingBall::createObject);
    t.history_kept = 1000;
    std::unique_ptr<BouncingBall> a = std::make_unique<BouncingBall>(vec3(0,0,0),vec3(1,0,0), 0.4f) ;
    std::unique_ptr<MoveBouncingBall> a_move = std::make_unique<MoveBouncingBall>(0.1) ;
    t.createObject(std::move(a), std::move(a_move), 1.0);
    //printf("A created!\n");
    t.run(1.1);
    int a_id = -1 ;
    vector<int> ob = t.updateObservables();
    a_id = ob[0]; 
    //printf("A_id : %d!\n", a_id);
    std::unique_ptr<BouncingBall> b = std::make_unique<BouncingBall>(vec3(5,5,0),vec3(0,-1,0), 0.4f) ;
    std::unique_ptr<MoveBouncingBall> b_move = std::make_unique<MoveBouncingBall>(0.1) ;
    t.createObject(std::move(b), std::move(b_move) , 1.001);

    double time = 1.1 ;
    t.run(time);
   
    ob = t.updateObservables();
    int b_id = ob[0] == a_id ? ob[1] : ob[0];
    //printf("B created! %d \n", b_id);

    UnitTests::expectNear(s, t.getLastObserved(a_id )->position, vec3(0.1,0,0), 0.01,
        "A did not initialize correctly!");

    UnitTests::expectNear(s, t.getLastObserved(b_id )->position, vec3(5,4.9,0), 0.01,
        "B did not initialize correctly!");

    //printf("Searching for collisions!\n");
    bool a_hit = false;
    while(!a_hit && time < 10.0){
        time+=0.1;
        t.run(time);
        ob = t.updateObservables();

        weak_ptr<TObject> ow = t.getLastObserved(a_id) ;
        if(auto og = ow.lock()){
            shared_ptr<BouncingBall> o = std::static_pointer_cast<BouncingBall>(og);
            a_hit =  o->velocity.x <= 0.5 ; // bounced and now has less v.x
        }
    }
    //printf("A hit at time : %f  %f\n", time , fabs(time - 5.0));
    UnitTests::expect(s, fabs(time - 5.5)<0.2, "A did not collide at correct time!");
    t.run(10.1);
    ob = t.updateObservables();
    /*
    printf("A at time 10 : \n");
    t.getLastObserved(a_id)->print() ;
    printf("B at time 10 : \n");
    t.getLastObserved(b_id)->print() ;
    */
    UnitTests::expect(s, t.getLastObserved(a_id)->position.x < 7, "A at incorrect position after collision");
    //printf("Adding retroactive change direction event that prevent collisions and running to time 10...\n");
    t.addEvent(std::make_unique<ChangeBallVelocity>(b_id, vec3(0,0,0)), 1.01) ;
    t.run(10.1);
    ob = t.updateObservables();
    /*
    printf("A at time 10 : \n");
    t.getLastObserved(a_id)->print() ;
    printf("B at time 10 : \n");
    t.getLastObserved(b_id)->print() ;
    */
    UnitTests::expect(s, t.getLastObserved(a_id)->position.x > 7, "A at incorrect position  after rolled back non collision");
    //printf("Adding retroactive change direction event that makes collision happen again and running to time 10...\n");
    t.addEvent(std::make_unique<ChangeBallVelocity>(b_id, vec3(0,-1,0)), 1.02) ;
    t.run(10.1);
    ob = t.updateObservables();
    /*
    printf("A at time 10 : \n");
    t.getLastObserved(a_id)->print() ;
    printf("B at time 10 : \n");
    t.getLastObserved(b_id)->print() ;
    */
    UnitTests::expect(s, t.getLastObserved(a_id)->position.x < 7, "A at incorrect position after rolled back recollision");
    
    return s ;
}

bool UnitTests::checkClearHistory(){
    //printf("---checkSimpleTimeWarp---\n");
    bool s = true ;
    
    Timeline t = Timeline(&BouncingBall::createEvent, &BouncingBall::createObject);
    t.history_kept = 1;
    t.base_age = 0.5 ;
    
    for(int k=0;k<10;k++){
        std::unique_ptr<BouncingBall> o = std::make_unique<BouncingBall>(vec3(k*100,k*1000%77,0),vec3((k*2)%3 - 1,k%3 - 1,0), 1.0f) ;
        std::unique_ptr<MoveBouncingBall> o_move = std::make_unique<MoveBouncingBall>(0.5) ;
        t.createObject(std::move(o), std::move(o_move) , 1.001 + 0.001*k);
    }
    t.run(1.1);
    expect(s, t.events.size() == 30, "Wrong number of events after first step!");
    //printf("%d\n",(int)t.events.events.size());

    double time = 10;
    double clear_time = 1.9;

    
    t.run(time);
    expect(s, t.events.size() == 200, "Wrong number of events after running!");
    //printf("%d\n",(int)t.events.events.size());

    expect(s, countHistory(t.objects[1]) == 19, "Wrong number of object instances after running!");
    //printf("%d\n",(int)t.objects[1].history.size());

    shared_ptr<TObject> o1 = t.getObjectInstant(1, clear_time).lock() ;

    t.clearHistoryBefore(clear_time);
    expect(s, t.events.size() == 200, "Wrong number of events after first clear!");
    //printf("%d\n",(int)t.events.events.size());

    shared_ptr<TObject> o1c = t.getObjectInstant(1, clear_time).lock() ;
    expect(s, o1c == o1, "Object at clear time changed after clear!");

    expect(s, countHistory(t.objects[1]) == 17, "Wrong number of object instances after first clear!");
    //printf("%d\n",(int)t.objects[1].history.size());

    t.run(time+1);
    while(time < 100){
        time+=10;
        clear_time+=10;
        t.run(time);
        t.clearHistoryBefore(clear_time);

        expect(s, t.events.size() == 370, "Wrong number of events after clear!");
        //printf("%d\n",(int)t.events.events.size());

        expect(s, countHistory(t.objects[2]) == 17, "Wrong number of object instances after clear!");
        //printf("%d\n",(int)t.objects[1].history.size());
    }


    t.vantage_id = 1 ;
    t.info_speed = 150;

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
            expectNear(s,positions[k], c_pos, 0.001,"Object moved after warped clear history!");
        }

        expect(s, t.events.size() == 370, "Wrong number of events after time warp clear!");
        //printf("%d\n",(int)t.events.events.size());

        expect(s, countHistory(t.objects[2])<= 17, "Too many object instances after time warp clear!");
        //printf("%d\n",(int)t.objects[2].history.size());

    }

    
    return s ;
}

bool UnitTests::checksimpleTimelineSync(){
    bool s = true ;
    
    Timeline t1 = Timeline(&BouncingBall::createEvent, &BouncingBall::createObject);
    t1.history_kept = 1;
    t1.base_age = 0.5 ;
    Timeline t2 = Timeline(&BouncingBall::createEvent, &BouncingBall::createObject);
    t2.history_kept = 1;
    t2.base_age = 0.5 ;
    std::unique_ptr<BouncingBall> o1 = std::make_unique<BouncingBall>(vec3(0,0,0),vec3(1,0,0), 1.0f) ;
    std::unique_ptr<MoveBouncingBall> o_move1 = std::make_unique<MoveBouncingBall>(0.5) ;
    t1.createObject(std::move(o1), std::move(o_move1) , 1.0);

    Variant d1 = t1.getDescriptor(0,false);
    //printf("d1 after queued creation event:\n");
    //d1.printFormatted();
    Variant d2 = t2.getDescriptor(0,false);

    //printf("d2 before first sync:\n");
    //d2.printFormatted();

    Variant u = t1.getUpdateFor(d2, true);
    //printf("first update:\n");
    //u.printFormatted();
    t2.applyUpdate(u, false);    
    d2 = t2.getDescriptor(0,false);

    //printf("d2 after first sync:\n");
    //d2.printFormatted();
    expect(s, d1.hash() == d2.hash(), "Timelines don't match after synchronizing object creation event!");
    
    t1.run(3.0);
    //t2.events.events[0]->print();
    t2.run(3.0);
    d1 = t1.getDescriptor(2.0, false);
    d2 = t2.getDescriptor(2.0, false);
    expect(s, d1.hash() == d2.hash(), "Timelines don't match after creating object!");
    vector<int> ob1 = t1.updateObservables();
    vector<int> ob2 = t2.updateObservables();
    expect(s, ob1.size() == 1, "Wrong number of objects after synchronization! 1");
    expect(s, ob2.size() == 1, "Wrong number of objects after synchronization! 2");
    for(int k=0;k<ob1.size();k++){
        const std::shared_ptr<TObject> a = t1.getLastObserved(ob1[k]);
        const std::shared_ptr<TObject> b = t2.getLastObserved(ob1[k]); // Note order of observables ids returned is not guarsnteed
        expectNear(s, a->position, b->position,0.001, "New object differs after syncrhonization!");
    }

    Timeline t3 = Timeline(&BouncingBall::createEvent, &BouncingBall::createObject);
    t3.history_kept = 1;
    t3.base_age = 0.5 ;
    t3.run(3.0);
    Variant d3 = t3.getDescriptor(2.0,false);
    u = t2.getUpdateFor(d3, true);

    //printf("d2 to d3 update:\n");
    //u.printFormatted();
    t3.applyUpdate(u, false);
    t3.run(3.0);
    d3 = t3.getDescriptor(2.0,false);
    expect(s, d1.hash() == d3.hash(), "Timelines don't match after syncing object!");

    t1.run(10);
    t2.run(10);
    t3.run(10);
    d1 = t1.getDescriptor(8.0,false);
    d2 = t2.getDescriptor(8.0,false);
    d3 = t3.getDescriptor(8.0,false);
    expect(s, d1.hash() == d2.hash(), "Synced timelines diverged!");
    expect(s, d1.hash() == d3.hash(), "Synced timelines diverged!");
    
    return s ;
}

bool UnitTests::checkSyncExistingObject(){

    bool s = true ;
    
    Timeline server = Timeline(&BouncingBall::createEvent, &BouncingBall::createObject);
    server.history_kept = 1;
    server.base_age = 0.5 ;

    Timeline client = Timeline(&BouncingBall::createEvent, &BouncingBall::createObject);
    client.history_kept = 1;
    client.base_age = 0.5 ;

    std::unique_ptr<BouncingBall> o1 = std::make_unique<BouncingBall>(vec3(0,0,0),vec3(1,0,0), 1.0f) ;
    std::unique_ptr<MoveBouncingBall> o_move1 = std::make_unique<MoveBouncingBall>(0.05) ;
    server.createObject(std::move(o1), std::move(o_move1) , 1.0);


    map<string, Variant> client_packet ;
    client_packet["descriptor"] = client.getDescriptor(0.0, false);
    map<string, Variant> server_packet = server.synchronize(client_packet, true);
    client_packet = client.synchronize(server_packet, false);

    expect(s, server.getDescriptor(0,false).hash() == client.getDescriptor(0,false).hash(), 
        "Timelines don't match after synchronizing object creation event!");
    
    server.run(3.0);
    //t2.events.events[0]->print();
    client.run(3.0);
    expect(s, server.getDescriptor(0,false).hash() == client.getDescriptor(0,false).hash(), 
        "Timelines don't match after creating object!");

    vector<int> ob1 = server.updateObservables();
    vector<int> ob2 = client.updateObservables();
    expect(s, ob1.size() == 1, "Wrong number of objects after synchronization! 1");
    expect(s, ob2.size() == 1, "Wrong number of objects after synchronization! 2");
    for(int k=0;k<ob1.size();k++){
        const std::shared_ptr<TObject> a = server.getLastObserved(ob1[k]);
        const std::shared_ptr<TObject> b = client.getLastObserved(ob1[k]); // Note order of observables ids returned is not guarsnteed
        expectNear(s, a->position, b->position,0.001, "New object differs after first synchronization!");
    }

    server.addEvent(std::make_unique<ChangeBallVelocity>(ob1[0], vec3(0,0,0)), 3.01) ;
    server.run(4.0);
    client.run(4.0);

    client_packet["descriptor"] = client.getDescriptor(4.0, false);
    server_packet = server.synchronize(client_packet, true);
    client_packet = client.synchronize(server_packet, false);
    //Variant(server_packet).printFormatted();
    //Variant(client_packet).printFormatted();
    server.run(5.0);
    client.run(5.0);

    ob1 = server.updateObservables();
    ob2 = client.updateObservables();
    for(int k=0;k<ob1.size();k++){
        const std::shared_ptr<TObject> a = server.getLastObserved(ob1[k]);
        const std::shared_ptr<TObject> b = client.getLastObserved(ob1[k]); // Note order of observables ids returned is not guarsnteed
        expectNear(s, a->position, b->position,0.001, "New object differs after object synchronization!");
    }

    
    return s ;

}


bool UnitTests::checkCreateObjectNotification(){
    bool s = true ;
    Timeline t = Timeline(&BouncingBall::createEvent, &BouncingBall::createObject);
    t.history_kept = 1000;
    t.base_age = 100 ;
    //printf("setting generators...\n");
    //printf("init circle...\n");
    std::unique_ptr<BouncingBall> o = std::make_unique<BouncingBall>(vec3(1,0,0),vec3(0,2,0), 3.0f) ;
    //printf("create object event...\n");
    t.createObject(std::move(o), std::unique_ptr<TEvent>(nullptr), "ball_id_trigger", 1.0);
    shared_ptr<int> ball_id = std::make_shared<int>(-1);
    t.subscribe("ball_id_catcher", "ball_id_trigger",
    [ball_id](const string& subscriber, const string& trigger, const Variant& data){
        //printf("notified of ball ID: %d\n", data.getInt());
        //printf("Subscriber: %s, Trigger: %s\n", subscriber.c_str(), trigger.c_str());
        *ball_id = data.getInt();
    });
    t.run(2.0);
    expect(s, *ball_id == 1, "Notification did not set ball ID correctly!");
    t.unsubscribe("ball_id_catcher", "ball_id_trigger");
    o = std::make_unique<BouncingBall>(vec3(10,0,0),vec3(0,2,0), 3.0f) ;
    t.createObject(std::move(o), std::unique_ptr<TEvent>(nullptr), "ball_id_trigger", 3.0);
    t.run(4.0);
    expect(s, *ball_id == 1, "ID not correct after notifying after unsubscribing!");

    return s ;
}