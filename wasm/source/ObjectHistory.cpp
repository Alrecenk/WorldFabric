#include "ObjectHistory.h"

#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"
#include "EventQueue.h"

using glm::vec3;

ObjectHistory::ObjectHistory(){

}

ObjectHistory::ObjectHistory(std::unique_ptr<TObject> value, double make_time){
    value->write_time = make_time ;
    history.push_back(std::move(value));
}

//Returns the latest version of this oject that is observable from the given vantage point and time
TObject* ObjectHistory::get(const glm::vec3& vantage, double time, double info_speed){
    //printf("entering objecthistory get...\n");


    if(history.size() ==0){
        //printf("requested object has never existed...\n");
        return nullptr ;
    }
    int latest_index = history.size()-1 ;
    TObject* latest = history[latest_index].get() ;
    while(latest->deleted){
        latest_index--;
        latest = history[latest_index].get() ;
    }

    vec3 diff = vantage-latest->position ;
    double d2 = glm::dot(diff,diff) ;
    //double warp = sqrt(d2)/info_speed ;
    /*
    if(time > deleted_time + warp){// if it's past time you could read deletion
        return nullptr ; // read deletion
    }else if(time > latest->write_time + warp){ //if could read latest value
        return latest ;
    }*/
    
    if(time > deleted_time){ // if reading past deleted time
        double cdt = (time-deleted_time) * info_speed ; // check time warp to see if we can read deletion yet
        if(cdt*cdt > d2){
            return nullptr;
        }
    }
    
    if(time>latest->write_time){ // if after latest value (the most common case)
        double cdt = (time-latest->write_time) * info_speed ;
        if(cdt*cdt > d2){// check time warp to see if we can read latest value yet
            return latest ;
        }
    }
    //printf("not reading latest\n");
    // Walk backwards from latest until we find something we're allowed to read
    for(int k=history.size()-2;k>=0; k--){
        if(!history[k]->deleted){
            double dist = glm::length(vantage-history[k]->position);
            double time_to_read = history[k]->write_time + dist/info_speed; // time this vantage point would be able ot read thatdata
            if(time_to_read < time){
                return history[k].get();
            }
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
        if(history[k]->write_time <= time && !history[k]->deleted){
            return history[k].get();
        }
    }
    return nullptr; // request is before creation
}

// Deletes the object at the given time
void ObjectHistory::deleteAfter(double time){
    
    int delete_from = history.size();
    for(int k=history.size()-1;k>=0; k--){
        if(!history[k]->deleted){
            if(history[k]->write_time < time){ // finding first one from end not being deleted
                delete_from = k;
                break;
            }
        }
    }


    for(int k=delete_from;k<history.size(); k++){
        if(!history[k]->deleted){
            for(auto& [reader, read_time] : history[k]->readers){
                if(reader != nullptr && read_time > time && !reader->deleted && !reader->run_pending){
                    timeline->events.rerunEvent(reader);
                }
            }
            if(k != delete_from){ // check for reruns in last surviving instant but don't delete it
                history[k]->deleted = true;
            }
        }
    }

    //printf("erasing from index %d\n", delete_from+1);
    /*
    if(delete_from < history.size()-1){
        history.erase(history.cbegin() + delete_from +1 , history.cend());// all after first should be completely removed
    }*/
}

//Creates a new instant at the given time by deep copying the previous instant
// and returns an editable version of it
// deletes all data beyond that point and marks events appropriately
TObject* ObjectHistory::getMutable(double time){

    int latest_index = history.size()-1 ;
    TObject* latest = history[latest_index].get() ;
    while(latest->deleted){
        latest_index--;
        latest = history[latest_index].get() ;
    }

    //printf("getting mutable at time %f\n", time);
    //printf("most recent write time %f\n", history[history.size()-1]->write_time);
    if(time < latest->write_time){
        //printf("retroactive write detected!\n");
        deleteAfter(time);
    }

    latest_index = history.size()-1 ;
    latest = history[latest_index].get() ;
    while(latest->deleted){
        latest_index--;
        latest = history[latest_index].get() ;
    }

    // A non retroactive write might still rollback a read to the most recent value if it happened betweeen the last write and the read
    for(auto& [reader, read_time] : latest->readers){
        if(reader != nullptr && read_time > time && !reader->deleted && !reader->run_pending){
            timeline->events.rerunEvent(reader);
        }
    }

    std::unique_ptr<TObject> new_instant = latest->deepCopy() ;
    new_instant->write_time = time ;
    history.push_back(std::move(new_instant));

    return history[history.size()-1].get() ;
}

// Objects may have a single instant before the clear time, so their value at that time can still be fetched
void ObjectHistory::clearHistoryBefore(double clear_time){
    // keep one instant before the clear time so we're defined at clear time
    int one_before = -1;
    for(int k=0;k<history.size(); k++){
        if(!history[k]->deleted){
            if(history[k]->write_time >= clear_time){
                break ;
            }else{
                one_before = k;
            }
        }
    }
    // Move undeleted elements after the start to the beginning of history
    int next = 0 ;
    for(int k=one_before;k<history.size(); k++){
        if(!history[k]->deleted){
            history[next] = std::move(history[k]) ;
            next++;
        }
    }

    // clear history we didn't move
    history.erase(history.cbegin() + next , history.cend());

    
}