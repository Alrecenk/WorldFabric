#include "Variant.h"

using std::string;

Variant::Variant() {
    type_ = NULL_VARIANT;
    ptr = nullptr;
}

Variant::Variant(const std::map<string, Variant> &obj) {
    type_ = OBJECT;
    int total_size = Variant::getSize(obj);
    //printf("total size: %i\n", total_size);
    ptr = (byte *) malloc(total_size);
    byte *input_ptr = ptr;
    ((int *) ptr)[0] = obj.size();
    input_ptr += 4;
    std::map<string, int *> linker;
    // Write the list of keys and types
    for (auto const&[key, data]:obj) {
        int key_size = Variant::getSize(key);
        Variant keyvar(key);
        memcpy(input_ptr, keyvar.ptr, key_size);
        input_ptr += key_size;
        input_ptr[0] = data.type_;
        input_ptr += 1;
        ((int *) input_ptr)[0] = 0; // place holder for relative position
        linker[key] = (int *) input_ptr;
        input_ptr += 4;
    }
    //write the actual objects
    for (auto const&[key, data]:obj) {
        int size = data.getSize();
        if(data.ptr != nullptr){
            memcpy(input_ptr, data.ptr, size);
        }
        // Link relative pointer into first array
        *linker[key] = (int) (input_ptr -
                              (ptr)); // Might throw a warning but should still work in 64 bit environment because it's relative
        input_ptr += size;
    }
}


Variant::Variant(const std::map<int, Variant> &obj) {
    type_ = INT_OBJECT;
    int total_size = Variant::getSize(obj);
    //printf("total size: %i\n", total_size);
    ptr = (byte *) malloc(total_size);
    byte *input_ptr = ptr;
    ((int *) ptr)[0] = obj.size();
    input_ptr += 4;
    std::map<int, int *> linker;
    // Write the list of keys and types
    for (auto const&[key, data]:obj) {
        Variant keyvar(key);
        int key_size = keyvar.getSize();
        memcpy(input_ptr, keyvar.ptr, key_size);
        input_ptr += key_size;
        input_ptr[0] = data.type_;
        input_ptr += 1;
        ((int *) input_ptr)[0] = 0; // place holder for relative position
        linker[key] = (int *) input_ptr;
        input_ptr += 4;
    }
    //write the actual objects
    for (auto const&[key, data]:obj) {
        int size = data.getSize();
        if(data.ptr != nullptr){
            memcpy(input_ptr, data.ptr, size);
        }
        // Link relative pointer into first array
        *linker[key] = (int) (input_ptr -
                              (ptr)); // Might throw a warning but should still work in 64 bit environment because it's relative
        input_ptr += size;
    }
}


Variant::Variant(const std::map<byte, Variant> &obj) {
    type_ = BYTE_OBJECT;
    int total_size = Variant::getSize(obj);
    //printf("total size: %i\n", total_size);
    ptr = (byte *) malloc(total_size);
    byte *input_ptr = ptr;
    ((int *) ptr)[0] = obj.size();
    input_ptr += 4;
    std::map<byte, int *> linker;
    // Write the list of keys and types
    for (auto const&[key, data]:obj) {
        Variant keyvar(key);
        int key_size = keyvar.getSize();
        memcpy(input_ptr, keyvar.ptr, key_size);
        input_ptr += key_size;
        input_ptr[0] = data.type_;
        input_ptr += 1;
        ((int *) input_ptr)[0] = 0; // place holder for relative position
        linker[key] = (int *) input_ptr;
        input_ptr += 4;
    }
    //write the actual objects
    for (auto const&[key, data]:obj) {
        int size = data.getSize();
        if(data.ptr != nullptr){
            memcpy(input_ptr, data.ptr, size);
        }
        // Link relative pointer into first array
        *linker[key] = (int) (input_ptr -
                              (ptr)); // Might throw a warning but should still work in 64 bit environment because it's relative
        input_ptr += size;
    }
}


Variant::Variant(const string& data) {
    //printf("Making variant from string: %s\n", data.c_str());
    int length = data.length();
    type_ = STRING;
    ptr = (byte *) malloc(4 + length);
    ((int *) ptr)[0] = length;
    memcpy(ptr + 4, data.c_str(), length);
}

Variant::Variant(int data) {
    type_ = INT;
    ptr = (byte *) malloc(4);
    ((int *) ptr)[0] = data;
}

Variant::Variant(short data) {
    type_ = SHORT;
    ptr = (byte *) malloc(2);
    ((short *) ptr)[0] = data;
}

Variant::Variant(byte data) {
    type_ = BYTE;
    ptr = (byte *) malloc(1);
    (ptr)[0] = data;
}

Variant::Variant(float data) {
    type_ = FLOAT;
    ptr = (byte *) malloc(4);
    ((float *) ptr)[0] = data;
}

Variant::Variant(double data) {
    type_ = DOUBLE;
    ptr = (byte *) malloc(8);
    ((double *) ptr)[0] = data;
}

Variant::Variant(const int *data, int array_length) {
    type_ = INT_ARRAY;
    ptr = (byte *) malloc(4 + 4 * array_length);
    ((int *) ptr)[0] = array_length;
    memcpy(ptr + 4, data, 4 * array_length);
}

Variant::Variant(const short *data, int array_length) {
    type_ = SHORT_ARRAY;
    ptr = (byte *) malloc(4 + 2 * array_length);
    ((int *) ptr)[0] = array_length;
    memcpy(ptr + 4, data, 2 * array_length);
}

Variant::Variant(const byte *data, int array_length) {
    type_ = BYTE_ARRAY;
    ptr = (byte *) malloc(4 + array_length);
    ((int *) ptr)[0] = array_length;
    memcpy(ptr + 4, data, array_length);
}

Variant::Variant(const float *data, int array_length) {
    type_ = FLOAT_ARRAY;
    ptr = (byte *) malloc(4 + 4 * array_length);
    ((int *) ptr)[0] = array_length;
    memcpy(ptr + 4, data, 4 * array_length);
}

Variant::Variant(const glm::vec3& data){
    type_ = FLOAT_ARRAY;
    int array_length = 3 ;
    ptr = (byte *) malloc(4 + 4 * array_length);
    ((int *) ptr)[0] = array_length;
    memcpy(ptr + 4, &data, 4 * array_length);
}

Variant::Variant(const double *data, int array_length) {
    type_ = DOUBLE_ARRAY;
    ptr = (byte *) malloc(4 + 8 * array_length);
    ((int *) ptr)[0] = array_length;
    memcpy(ptr + 4, data, 8 * array_length);
}

Variant::Variant(const std::vector<Variant> &array) {
    type_ = VARIANT_ARRAY;
    int total_size = Variant::getSize(array);
    ptr = (byte *) malloc(total_size);
    byte *input_ptr = ptr;
    ((int *) ptr)[0] = array.size();
    input_ptr += 4;
    std::vector<byte *> linker(array.size());
    //printf("total size: %i\n", total_size);
    // Write the list of types and allocate pointers
    for (int k = 0; k < array.size(); k++) {
        input_ptr[0] = array[k].type_;
        input_ptr += 1;
        //((int*)input_ptr)[0] = 0; // place holder for relative position
        linker[k] = input_ptr;
        input_ptr += 4;
    }
    //write the actual objects
    for (int k = 0; k < array.size(); k++) {
        int size = array[k].getSize();
        memcpy(input_ptr, array[k].ptr, size);
        // Link relative pointer into first array
        ((int *) linker[k])[0] = (int) (input_ptr -
                                        (ptr)); // Might throw a warning but should still work in 64 bit environment because it's relative
        input_ptr += size;
    }
}

Variant::Variant(const Variant *source) {
    type_ = source->type_;
    if (source->type_ != NULL_VARIANT) {
        int size = source->getSize();
        ptr = (byte *) malloc(size);
        memcpy(ptr, source->ptr, size);
    }else{
        ptr = nullptr;
    }
}

Variant::Variant(const Variant &source) {
    type_ = source.type_;
    if (source.type_ != NULL_VARIANT) {
        int size = source.getSize();
        ptr = (byte *) malloc(size);
        memcpy(ptr, source.ptr, size);
    }else{
        ptr = nullptr;
    }
}

Variant::Variant(const Type type, const byte *input_ptr) {
    type_ = type;
    if(type != NULL_VARIANT){
        int size = Variant::getSize(type, input_ptr);
        ptr = (byte *) malloc(size);
        memcpy(ptr, input_ptr, size);
    }else{
        ptr = nullptr;
    }
}

Variant::Variant(Variant &&source) noexcept {
    type_ = source.type_;
    ptr = source.ptr;
    source.type_ = NULL_VARIANT;
    source.ptr = nullptr;
}

// Variants own their pointers and free them when deallocated
Variant::~Variant() {
    if(ptr != nullptr && type_ == NULL_VARIANT){
        printf ("freeing null variant with pointer!?\n");
    }
    free(ptr);
}


Variant &Variant::operator=(Variant &&source) {
    free(ptr);
    type_ = source.type_;
    ptr = source.ptr;
    source.type_ = NULL_VARIANT;
    source.ptr = nullptr;
    return *this;
}

int Variant::getSize() const {
    return Variant::getSize(type_, ptr);
}

// returns the size of the data pointed to by a variant's pointer (does not include type)
int Variant::getSize(const Type type, const byte *input_ptr) {
    if (type == OBJECT) {
        int num_keys = *((int *) input_ptr);
        const byte *sinput_ptr = input_ptr + 4;
        for (int k = 0; k < num_keys; k++) {
            sinput_ptr = sinput_ptr + getSize(STRING, sinput_ptr);
            Type type = (Type) (*((char *) sinput_ptr));
            sinput_ptr = sinput_ptr + 1;
            int rel_input_ptr = *((int *) sinput_ptr);
            sinput_ptr = sinput_ptr + 4;
            if (k == num_keys - 1) { // Last item is at end
                return rel_input_ptr + getSize(
                        type, input_ptr +
                              rel_input_ptr); // so size is relative pointer to end of last item
            }
        }
        return 4; // empty object takes 4 bytes for its size
    }
    if (type == INT_OBJECT) {
        int num_keys = *((int *) input_ptr);
        const byte *sinput_ptr = input_ptr + 4;
        for (int k = 0; k < num_keys; k++) {
            sinput_ptr = sinput_ptr + 4; // one int for key
            Type type = (Type) (*((char *) sinput_ptr));
            sinput_ptr = sinput_ptr + 1;
            int rel_input_ptr = *((int *) sinput_ptr);
            sinput_ptr = sinput_ptr + 4;
            if (k == num_keys - 1) { // Last item is at end
                return rel_input_ptr + getSize(
                        type, input_ptr +
                              rel_input_ptr); // so size is relative pointer to end of last item
            }
        }
        return 4; // empty object takes 4 bytes for its size
    }
    if (type == BYTE_OBJECT) {
        int num_keys = *((int *) input_ptr);
        const byte *sinput_ptr = input_ptr + 4;
        for (int k = 0; k < num_keys; k++) {
            sinput_ptr = sinput_ptr + 1; // 1 byte for key
            Type type = (Type) (*((char *) sinput_ptr));
            sinput_ptr = sinput_ptr + 1;
            int rel_input_ptr = *((int *) sinput_ptr);
            sinput_ptr = sinput_ptr + 4;
            if (k == num_keys - 1) { // Last item is at end
                return rel_input_ptr + getSize(
                        type, input_ptr +
                              rel_input_ptr); // so size is relative pointer to end of last item
            }
        }
        return 4; // empty object takes 4 bytes for its size
    } else if (type == STRING) {
        return 4 + (*((int *) input_ptr));// Actually just a byte array serialized
    } else if (type == INT) {
        return 4;
    } else if (type == SHORT) {
        return 2;
    } else if (type == BYTE) {
        return 1;
    } else if (type == FLOAT) {
        return 4;
    } else if (type == DOUBLE) {
        return 8;
    } else if (type == INT_ARRAY) {
        return 4 + (*((int *) input_ptr)) * 4;
    } else if (type == SHORT_ARRAY) {
        return 4 + (*((int *) input_ptr)) * 2;
    } else if (type == BYTE_ARRAY) {
        return 4 + (*((int *) input_ptr));
    } else if (type == FLOAT_ARRAY) {
        return 4 + (*((int *) input_ptr)) * 4;
    } else if (type == DOUBLE_ARRAY) {
        return 4 + (*((int *) input_ptr)) * 8;
    } else if (type == VARIANT_ARRAY) {
        int length = *((int *) input_ptr);
        const byte *sinput_ptr = input_ptr + 4;
        for (int k = 0; k < length; k++) {
            Type type = (Type) (*((byte *) sinput_ptr));
            sinput_ptr = sinput_ptr + 1;
            int rel_input_ptr = *((int *) sinput_ptr);
            sinput_ptr = sinput_ptr + 4;
            if (k == length - 1) { // Last item is at end
                return rel_input_ptr + getSize(
                        type, input_ptr +
                              rel_input_ptr); // so size is relative pointer to end of last item
            }
        }
        return 4;// if no elements
    } else if (type == NULL_VARIANT) {
        return 0;
    }
    printf("Unrecognized type in C++ serializer! Data corruption likely.\n");
    return 0;
}

// returns the size that would need to be allocated to serialize the given object (does not include type)
int Variant::getSize(const std::map<string, Variant> &obj) {
    int size = 4; // num_keys
    for (auto const&[key, data]:obj) {
        size += getSize(key);
        size += 5; // type and pointer
        size += data.getSize();
    }
    return size;
}

// returns the size that would need to be allocated to serialize the given object (does not include type)
int Variant::getSize(const std::map<int, Variant> &obj) {
    int size = 4; // num_keys
    for (auto const&[key, data]:obj) {
        size += 4; // int key
        size += 5; // type and pointer
        size += data.getSize();
    }
    return size;
}

// returns the size that would need to be allocated to serialize the given object (does not include type)
int Variant::getSize(const std::map<byte, Variant> &obj) {
    int size = 4; // num_keys
    for (auto const&[key, data]:obj) {
        size += 1; // byte key
        size += 5; // type and pointer
        size += data.getSize();
    }
    return size;
}

// returns the size that would need to be allocated to serialize the given string (does not include type)
int Variant::getSize(const string &data) {
    return 4 + data.length();
}

// returns the size that would need to be allocated to serialize the given variant array (does not include type)
int Variant::getSize(const std::vector<Variant> &array) {
    int size = 4; // length
    for (const auto & var : array) {
        size += 5; // type and pointer
        size += var.getSize();
    }
    return size;
}

// Returns a string serialized with this library or its Javascript counterpart (might be UTF-8!)
string Variant::deserializeString(const byte *input_ptr) {
    int length = *((int *) input_ptr);// in bytes, not necessarily characters
    return string((char *) (input_ptr + 4), length);
}

// Returns the string on a Variant holding a string
string Variant::getString() const {
    return Variant::deserializeString(ptr);
}

// Given a pointer to a JS style object returns a map with Variants to the internal elements
// Copies all data so use wisely
std::map<string, Variant> Variant::deserializeObject(byte *input_ptr) {
    byte *start_input_ptr = input_ptr;
    std::map<string, Variant> obj;
    int num_keys = *((int *) input_ptr);
    input_ptr = input_ptr + 4;
    for (int k = 0; k < num_keys; k++) {
        string key = deserializeString(input_ptr);
        input_ptr = input_ptr + Variant::getSize(key);
        Type type = (Type) (*((char *) input_ptr));
        input_ptr = input_ptr + 1;
        int rel_input_ptr = *((int *) input_ptr);
        input_ptr = input_ptr + 4;
        Variant v(type, start_input_ptr + rel_input_ptr);
        obj.emplace(key, v);
    }
    return obj;
}


// Given a pointer to an object  with int keys returns a map with Variants to the internal elements
// Copies all data so use wisely
std::map<int, Variant> Variant::deserializeIntObject(byte *input_ptr) {
    byte *start_input_ptr = input_ptr;
    std::map<int, Variant> obj;
    int num_keys = *((int *) input_ptr);
    input_ptr = input_ptr + 4;
    for (int k = 0; k < num_keys; k++) {
        int key = *((int *) input_ptr);
        input_ptr = input_ptr + 4;
        Type type = (Type) (*((char *) input_ptr));
        input_ptr = input_ptr + 1;
        int rel_input_ptr = *((int *) input_ptr);
        input_ptr = input_ptr + 4;
        Variant v(type, start_input_ptr + rel_input_ptr);
        obj.emplace(key, v);
    }
    return obj;
}

// Given a pointer to an object  with int keys returns a map with Variants to the internal elements
// Copies all data so use wisely
std::map<byte, Variant> Variant::deserializeByteObject(byte *input_ptr) {
    byte *start_input_ptr = input_ptr;
    std::map<byte, Variant> obj;
    int num_keys = *((int *) input_ptr);
    input_ptr = input_ptr + 4;
    for (int k = 0; k < num_keys; k++) {
        byte key = *((byte *) input_ptr);
        input_ptr = input_ptr + 1;
        Type type = (Type) (*((char *) input_ptr));
        input_ptr = input_ptr + 1;
        int rel_input_ptr = *((int *) input_ptr);
        input_ptr = input_ptr + 4;
        Variant v(type, start_input_ptr + rel_input_ptr);
        obj.emplace(key, v);
    }
    return obj;
}

// Returns the object on a Variant holding an Object
std::map<string, Variant> Variant::getObject() const {
    if (type_ != OBJECT) {
        printf("string object is not a string object! %i\n", type_);
    }
    return Variant::deserializeObject(ptr);
}

// Returns the object on a Variant holding an Object
std::map<int, Variant> Variant::getIntObject() const {
    if (type_ != INT_OBJECT) {
        printf("int object is not an int object! %i\n", type_);
    }
    return Variant::deserializeIntObject(ptr);
}

// Returns the object on a Variant holding an Object
std::map<byte, Variant> Variant::getByteObject() const {
    if (type_ != BYTE_OBJECT) {
        printf("byte object is not a byte object! %i\n", type_);
    }
    return Variant::deserializeByteObject(ptr);
}

// Given a pointer to a JS array returns a map with pointers to the internal elements
// Copies all data so use wisely
std::vector<Variant> Variant::deserializeVariantArray(byte *input_ptr) {
    byte *start_input_ptr = input_ptr;
    int length = *((int *) input_ptr);
    std::vector<Variant> array;
    input_ptr = input_ptr + 4;
    for (int k = 0; k < length; k++) {
        Type type = (Type) (*((byte *) input_ptr));
        input_ptr = input_ptr + 1;
        int rel_input_ptr = *((int *) input_ptr);
        input_ptr = input_ptr + 4;
        Variant v(
                type, start_input_ptr +
                      rel_input_ptr);// Not commutative, use input_ptr first to get input_ptr
        array.push_back(v);
    }
    return array;
}

// Returns the array on a Variant holding a Variant array
std::vector<Variant> Variant::getVariantArray() const {
    return Variant::deserializeVariantArray(ptr);
}

// Convenience functions for extracting raw types
int Variant::getInt() const {
    return *(int *) ptr;
}

short Variant::getShort() const {
    return *(short *) ptr;
}

float Variant::getFloat() const {
    return *(float *) ptr;
}

double Variant::getDouble() const {
    return *(double *) ptr;
}

byte Variant::getByte() const {
    return *(ptr);
}

// Returns the length of any array type variant (including variant array)
int Variant::getArrayLength() const {
    return getInt(); // Array length is just a preceding int
}

//Convenience methods for returning array pointers
//These do not copy and aren't malloc locations so will crash if you try to free them.
int *Variant::getIntArray() const {
    return (int *) (ptr + 4); // skip length
}

byte *Variant::getByteArray() const {
    return (ptr + 4); // skip length
}

float *Variant::getFloatArray() const {
    return (float *) (ptr + 4); // skip length
}

glm::vec3 Variant::getVec3() const{
    return *((glm::vec3*)getFloatArray()) ;
}

double *Variant::getDoubleArray() const {
    return (double *) (ptr + 4); // skip length
}

short *Variant::getShortArray() const {
    return (short *) (ptr + 4); // skip length
}

/* Numbers passed from javascript could be int, double, or float depending on how they're writtten or stored
This function detects what type came in and converts it to a 32 bit float */
float Variant::getNumberAsFloat() const{
    if(type_ == Variant::INT){
        return (float)*((int*)(ptr)) ;
    }else if(type_ == Variant::DOUBLE){
        return (float)*((double*)(ptr)) ;
    }else if(type_ == Variant::FLOAT){
        return *((float*)(ptr)) ;
    }else{
        return 0;
    }
}

// Prints this variant to the console using printf (no newline)
void Variant::print() const {
    printf("%s", Variant::deserializeToString(type_, ptr).c_str());
}

// Prints a serialized object to a string
string Variant::deserializeToString(char type, byte *input_ptr) {
    string res;
    if (type == INT) {
        res = std::to_string(*((int *) input_ptr));
    } else if (type == SHORT) {
        res = std::to_string(*((short *) input_ptr));
    } else if (type == BYTE) {
        res = std::to_string(*((char *) input_ptr));
    } else if (type == FLOAT) {
        res = std::to_string(*((float *) input_ptr));
    } else if (type == DOUBLE) {
        res = std::to_string(*((double *) input_ptr));
    } else if (type == OBJECT) {
        std::map<string, Variant> obj = deserializeObject(input_ptr);
        res = "{";
        byte first = true;
        for (auto const&[key, data]:obj) {
            if (!first) {
                res += ",";
            }
            first = false;
            res += key + ":";
            res += deserializeToString(data.type_, data.ptr);
        }
        res += "}";
    } else if (type == INT_OBJECT) {
        std::map<int, Variant> obj = deserializeIntObject(input_ptr);
        res = "{";
        byte first = true;
        for (auto const&[key, data]:obj) {
            if (!first) {
                res += ",";
            }
            first = false;
            res += std::to_string(key) + ":";
            res += deserializeToString(data.type_, data.ptr);
        }
        res += "}";
    } else if (type == BYTE_OBJECT) {
        std::map<byte, Variant> obj = deserializeByteObject(input_ptr);
        res = "{";
        byte first = true;
        for (auto const&[key, data]:obj) {
            if (!first) {
                res += ",";
            }
            first = false;
            res += std::to_string(key) + ":";
            res += deserializeToString(data.type_, data.ptr);

        }
        res += "}";
    } else if (type == STRING) {
        res = "\"" + deserializeString(input_ptr) + "\"";
    } else if (type == VARIANT_ARRAY) {
        std::vector<Variant> array = deserializeVariantArray(input_ptr);
        res = "[";
        byte first = true;
        for (int k = 0; k < array.size(); k++) {
            if (!first) {
                res += ",";
            }
            first = false;
            Variant data = array[k];
            res += deserializeToString(data.type_, data.ptr);
        }
        res += "]";
    } else if (type == INT_ARRAY) {
        int length = *((int *) input_ptr);
        input_ptr += 4;
        res = "[";
        byte first = true;
        for (int k = 0; k < length; k++) {
            if (!first) {
                res += ",";
            }
            first = false;
            res += std::to_string(*((int *) input_ptr));
            input_ptr += 4;
        }
        res += "]";
    } else if (type == SHORT_ARRAY) {
        int length = *((int *) input_ptr);
        input_ptr += 4;
        res = "[";
        byte first = true;
        for (int k = 0; k < length; k++) {
            if (!first) {
                res += ",";
            }
            first = false;
            res += std::to_string(*((short *) input_ptr));
            input_ptr += 2;
        }
        res += "]";
    } else if (type == BYTE_ARRAY) {
        int length = *((int *) input_ptr);
        input_ptr += 4;
        res = "[";
        byte first = true;
        for (int k = 0; k < length; k++) {
            if (!first) {
                res += ",";
            }
            first = false;
            res += std::to_string(*((byte *) input_ptr));
            input_ptr += 1;
        }
        res += "]";
    } else if (type == FLOAT_ARRAY) {
        int length = *((int *) input_ptr);
        input_ptr += 4;
        res = "[";
        byte first = true;
        for (int k = 0; k < length; k++) {
            if (!first) {
                res += ",";
            }
            first = false;
            res += std::to_string(*((float *) input_ptr));
            input_ptr += 4;
        }
        res += "]";
    } else if (type == DOUBLE_ARRAY) {
        int length = *((int *) input_ptr);
        input_ptr += 4;
        res = "[";
        byte first = true;
        for (int k = 0; k < length; k++) {
            if (!first) {
                res += ",";
            }
            first = false;
            res += std::to_string(*((double *) input_ptr));
            input_ptr += 8;
        }
        res += "]";
    } else if (type == NULL_VARIANT) {
        res = "NULL";
    } else {
        res = "Unknown type:" + std::string(1, type);
    }
    return res;
}

string Variant::toString() const {
    return Variant::deserializeToString(type_, ptr);
}


// Returns a deep copy of the given variant
Variant Variant::clone() const {
    return Variant(this);
}

// returns the hash of a variant
int Variant::hash() {
    return Variant::murmur((uint8_t*) ptr, getSize(), 17);
}

int Variant::hash(string s) {
    return Variant(s).hash();
}


uint32_t Variant::murmurscramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

// I grabbed a hash function that seemed pretty good from wikipedia
// It's in the public domain
uint32_t Variant::murmur(const uint8_t* key, size_t len, uint32_t seed) {
    uint32_t h = seed;
    uint32_t k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
        memcpy(&k, key, sizeof(uint32_t));
        key += sizeof(uint32_t);
        h ^= murmurscramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }
    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmurscramble(k);
    /* Finalize. */
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}