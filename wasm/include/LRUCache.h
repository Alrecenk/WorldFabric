#ifndef _LRUCACHE_H_
#define _LRUCACHE_H_ 1

#include <memory>
#include <unordered_map>
#include <set>
#include <vector>

template<class Key, class Value>
class CacheQueueNode {
    public:
        Key entry_key;
        std::shared_ptr<Value> entry;
        CacheQueueNode<Key,Value> *next = nullptr;
        CacheQueueNode<Key,Value> *prev = nullptr;
        
        CacheQueueNode(Key key, std::shared_ptr<Value> value);
        void removeFromQueue();
};

template<class Key, class Value>
class LRUCache {
    public:
        int size;
        int filled;
        std::unordered_map<Key,std::unique_ptr<CacheQueueNode<Key,Value>>> map;
        CacheQueueNode<Key,Value> *most_recent = nullptr;
        CacheQueueNode<Key,Value> *least_recent = nullptr;
        std::set<Key> deleted_keys ;
        
        LRUCache(int total_size);
        bool contains(Key key) const;
        void removeFromQueue(CacheQueueNode<Key,Value>* node);
        void accessed(CacheQueueNode<Key,Value>* node);
        void insert(Key key, std::shared_ptr<Value> value);
        std::shared_ptr<Value> get(Key key);
        void remove(Key key);
        std::set<Key> popDeletedKeys();
        std::vector<Key> getAllKeys();
};


template<class Key, class Value> CacheQueueNode<Key, Value>::CacheQueueNode(Key key, std::shared_ptr<Value> value){
    entry = value;
    entry_key = key;
}

template<class Key, class Value>
void CacheQueueNode<Key, Value>::removeFromQueue(){
    if(next !=nullptr){
        next->prev = prev;
    }
    if(prev !=nullptr){
        prev->next = next;
    }
    next = nullptr;
    prev= nullptr;
}

template<class Key, class Value>
LRUCache<Key, Value>::LRUCache(int total_size){
    size = total_size;
    filled = 0 ;
}

template<class Key, class Value>
bool LRUCache<Key, Value>::contains(Key key) const{
    return map.find(key) != map.end() ;
}

template<class Key, class Value> 
void LRUCache<Key, Value>::insert(Key key, std::shared_ptr<Value> value){
    remove(key);
    CacheQueueNode<Key, Value> *node = new CacheQueueNode<Key, Value>(key,value) ;
    std::unique_ptr<CacheQueueNode<Key, Value>> ptr = std::unique_ptr<CacheQueueNode<Key, Value>>(node);
    map[key] = std::move(ptr);
    accessed(node);
    filled++;
    if(filled > size){
        deleted_keys.insert(least_recent->entry_key);
        map.erase(least_recent->entry_key);
        removeFromQueue(least_recent);
        filled--;
    }

}

template<class Key, class Value>
std::shared_ptr<Value> LRUCache<Key, Value>::get(Key key){
    auto iter = map.find(key);
    if(iter != map.end()){
        CacheQueueNode<Key, Value> *node = (*iter).second.get();
        accessed(node);
        return node->entry;
    }else{
        return std::shared_ptr<Value>(nullptr);
    }
}

template<class Key, class Value>
void LRUCache<Key, Value>::remove(Key key){
    auto iter = map.find(key);
    if(iter != map.end()){
        deleted_keys.insert(key);
        removeFromQueue((*iter).second.get());
        map.erase(iter);
        filled--;
    }
}

template<class Key, class Value>
void LRUCache<Key, Value>::removeFromQueue(CacheQueueNode<Key,Value>* node){
    if(most_recent == node){
        most_recent = node->next;
    }
    if(least_recent == node){
        least_recent = node->prev;
    }
    node->removeFromQueue();
}

template<class Key, class Value>
void LRUCache<Key, Value>::accessed(CacheQueueNode<Key,Value>* node){
    removeFromQueue(node);
    if(most_recent != nullptr){
        most_recent->prev = node;
        node->next = most_recent;
    }else{ // empty queue
        least_recent = node;
    }
    most_recent = node;
}

template<class Key, class Value>
std::set<Key> LRUCache<Key, Value>::popDeletedKeys(){
    std::set<Key> ret = deleted_keys ;
    deleted_keys.clear();
    return ret ;
}

template<class Key, class Value>
std::vector<Key> LRUCache<Key, Value>::getAllKeys(){
    std::vector<Key> keys;
    keys.reserve(map.size());
    for(auto& kv : map) {
        keys.push_back(kv.first);
    }
    return keys ;
}

#endif // #ifndef _LRUCACHE_H_