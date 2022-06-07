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
    //printf("entering objecthistory get...\n");
    if(history.size() ==0){
        //printf("requested object has never existed...\n");
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
void ObjectHistory::deleteAfter(double time){
    
    int delete_from = history.size();
    for(int k=history.size()-1;k>=0; k--){
        if(history[k]->write_time < time){ // finding first one from end not being deleted
            delete_from = k;
            break;
        }
    }

    for(int k=delete_from;k<history.size(); k++){
        for(auto& [reader, read_time] : history[k]->readers){
            if(read_time > time && !reader->deleted && !reader->run_pending){
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
        //printf("retroactive write detected!\n");
        deleteAfter(time);
    }

    // A non retroactive write might still rollback a read to the most recent value if it happened betweeen the last write and the read
    for(auto& [reader, read_time] : history[history.size()-1]->readers){
        if(read_time > time && !reader->deleted && !reader->run_pending){
            timeline->events.rerunEvent(reader);
        }
    }

    std::unique_ptr<TObject> new_instant = history[history.size()-1]->deepCopy() ;
    new_instant->write_time = time ;
    history.push_back(std::move(new_instant));

    return history[history.size()-1].get() ;
}

// Objects may have a single instant before the clear time, so their value at that time can still be fetched
void ObjectHistory::clearHistoryBefore(double clear_time){
    int delete_to = 0;
    for(int k=0;k<history.size(); k++){
        if(history[k]->write_time > clear_time){ // finding first one from end not being deleted
            delete_to = k -1;
            break;
        }
    }
    history.erase(history.cbegin(), history.cbegin() + delete_to);
}