#include "ObjectHistory.h"

#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"
#include "EventQueue.h"

ObjectHistory::ObjectHistory(){

}

ObjectHistory::ObjectHistory(std::unique_ptr<TObject> value, double make_time){
    value->write_time = make_time ;
    history.push_back(std::move(value));
}

//Returns the latest version of this oject that is observable from the given vantage point and time
TObject* ObjectHistory::get(glm::vec3 vantage, double time, double info_speed){
    printf("entering objecthistory get...\n");
    if(history.size() ==0){
        printf("requested object has never existed...\n");
        return nullptr ;
    }
    double time_to_deleted = deleted_time + glm::length(vantage-history[history.size()-1]->position)/info_speed;
    if(time > deleted_time){// if it's past time you could read deletion
        return nullptr ; // read deletion
    }
    for(int k=history.size()-1;k>=0; k--){
        double dist = glm::length(vantage-history[k]->position);
        double time_to_read = history[k]->write_time + dist/info_speed; // time this vantage point would be able ot read thatdata
        if(time_to_read < time){
            return history[k].get();
        }
    }
    return nullptr; 
}


//Returns the version of this oject at the given time, ignoring time warp
// For internal use to get the position of events from their anchor or when no vantage object exists
TObject* ObjectHistory::get(double time){
    if(time > deleted_time){
        return nullptr;
    }
    for(int k=history.size()-1;k>=0; k--){
        if(history[k]->write_time < time){
            return history[k].get();
        }
    }
    return nullptr; // request is before creation
}

// Deletes the object at the given time
void ObjectHistory::deleteAt(double time){
    if(time > deleted_time){
        return ; //don't need to redelete something deleted earlier
    }
    printf("deleting from time %f\n", time);
    deleted_time = time ;
    int delete_from = history.size();
    for(int k=history.size()-1;k>=0; k--){
        if(history[k]->write_time < deleted_time){
            delete_from = k;
            break;
        }
    }

    //printf("deleting from index %d\n", delete_from);
    
    for(int k=delete_from;k<history.size(); k++){
        for(auto& [reader, read_time] : history[k]->readers){
            if(read_time > deleted_time && !reader->deleted && !reader->run_pending){
                timeline->events.rerunEvent(reader);
            }
        }
    }

    //printf("erasing from index %d\n", delete_from+1);
    history.erase(history.cbegin() + delete_from +1 , history.cend());// all after first should be completely removed
}

//Creates a new instant at the given time by deep copying the previous instant
// and returns an editable version of it
// deletes all data beyond that point and marks events appropriately
TObject* ObjectHistory::getMutable(double time){
    //printf("getting mutable at time %f\n", time);
    //printf("most recent write time %f\n", history[history.size()-1]->write_time);
    if(time < history[history.size()-1]->write_time){
        printf("retroactive write detected!\n");
        deleteAt(time);
        deleted_time = 9999999.0 ;
    }
    std::unique_ptr<TObject> new_instant = history[history.size()-1]->deepCopy() ;
    new_instant->write_time = time ;
    history.push_back(std::move(new_instant));

    return history[history.size()-1].get() ;
}