#include "ObjectHistory.h"

//Returns the latest version of this oject that is observable from the given vantage point and time
TObject* ObjectHistory::get(vec3 vantage, double time, double info_speed){

    double time_to_deleted = deleted_time + glm::length(vantage-history[history.size()-1].position)/info_speed;
    if(time > deleted_time){// if it's past time you could read deletion
        return nullptr ; // read deletion
    }
    for(int k=history.size()-1;k>=0; k--){
        double dist = glm::length(vantage-history[k].position);
        double time_to_read = history[k].write_time + dist/info_speed; // time this vantage point would be able ot read thatdata
        if(time_to_read < time){
            return &history[k];
        }
    }
    return nullptr; 
}


//Returns the version of this oject at the given time, ignoring time warp
// For internal use to get the position of events from their anchor
TObject* ObjectHistory::get(double time){
    if(time > deleted_time){
        return null_ptr;
    }
    for(int k=history.size()-1;k>=0; k--){
        if(history[k].time < time){
            return &history[k];
        }
    }
    return nullptr; 
}

// Deletes the object at the given time
void ObjectHistory::deleteAt(double time){
    deleted_time = time ;
    int delete_from = history.size();
    for(int k=history.size()-1;k>=0; k--){
        if(history[k].time < deleted_time){
            delete_from = k;
            break;
        }
    }
    
    for(int k=delete_from;k<history.size(); k++){
        for(auto& readers : history[k].readers){
            if(readers.second >= deleted_time && !readers.first->deleted && !readers.first.run_pending){
                timeline.events.rerunEvent(readers.first);
            }
        }
    }

    v.erase(history.cbegin() + k +1 , v.cend());
}

//Creates a new instant at the given time by deep copying the previous instant
// and returns an editable version of it
// deletes all data beyond that point and marks events appropriately
TObject* ObjectHistory::getMutable(double time){
    if(time < history[history.size()-1].time){
        deleteAt(time);
        deleted_time = 9999999.0 ;
    }
    history.push_back(history[history.size()-1].deepCopy());
    return &history[history.size()-1] ;
}