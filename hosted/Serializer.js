/* This file provides serialization for javascript objects to byte arrays and back.
The webassembly interface and various binary socket components use these functions for their byte buffer conversions.
*/
class Serializer {

    static TYPES = {
        NULL_VARIANT: 0,
        OBJECT: 1,
        STRING: 2,
        INT: 3,
        SHORT: 4,
        BYTE: 5,
        FLOAT: 6,
        DOUBLE: 7,
        INT_ARRAY: 8,
        SHORT_ARRAY: 9,
        BYTE_ARRAY: 10,
        FLOAT_ARRAY: 11,
        DOUBLE_ARRAY: 12,
        VARIANT_ARRAY: 13,
        INT_OBJECT: 14,
        BYTE_OBJECT: 15
    }
    // TODO this side effect length is ugly and prevents Serializer from being static but not sure how to fix without a speed penalty.
    last_deserialized_length = 0; // The length of the last item deserialized with this Serializer

    // Returns the type of a piece of Javascript data for serialization
    // Numbers will be encoded as int if they are ints, otherwise they will be double
    // Note that serializable objects have restrictions (see serializeObject for details)
    static getType(data) {
        if (typeof data === "number") {
            if (Number.isInteger(data)) {
                return Serializer.TYPES.INT;
            } else {
                return Serializer.TYPES.DOUBLE;
            }
        } else if (typeof data === "string") {
            return Serializer.TYPES.STRING;
        } else if (data instanceof Int32Array) {
            return Serializer.TYPES.INT_ARRAY;
        } else if (data instanceof Int16Array) {
            return Serializer.TYPES.SHORT_ARRAY;
        } else if (data instanceof Int8Array) {
            return Serializer.TYPES.BYTE_ARRAY;
        } else if (data instanceof Float32Array) {
            return Serializer.TYPES.FLOAT_ARRAY;
        } else if (data instanceof Float64Array) {
            return Serializer.TYPES.DOUBLE_ARRAY;
        } else if (Array.isArray(data)) {
            return Serializer.TYPES.VARIANT_ARRAY;
        } else if (typeof data === 'object' && data !== null) {
            return Serializer.TYPES.OBJECT;
        } else {
            return Serializer.TYPES.NULL_VARIANT;
        }
    }

    // Returns the number of bytes it would take to serialize the given piece of data
    static getSize(data) {
        let type = Serializer.getType(data);
        let size = 0;// switch scoping is weird
        switch (type) {
            case Serializer.TYPES.OBJECT:
                size = 4; // num keys
                for (let key in data) {
                    size += 4 + (new TextEncoder().encode(key)).length;// extra int to store length
                    size += 1 + 4; // type + pointer
                    size += this.getSize(data[key]);
                }
                return size;
            case Serializer.TYPES.INT_OBJECT:
                size = 4; // num keys
                for (let key in data) {
                    size += 4;// int key
                    size += 1 + 4; // type + pointer
                    size += this.getSize(data[key]);
                }
                return size;
            case Serializer.TYPES.BYTE_OBJECT:
                size = 4; // num keys
                for (let key in data) {
                    size += 1;// byte key
                    size += 1 + 4; // type + pointer
                    size += this.getSize(data[key]);
                }
                return size;
            case Serializer.TYPES.STRING:
                return 4 + (new TextEncoder().encode(data)).length; // extra int to store length
            case Serializer.TYPES.INT:
                return 4
            case Serializer.TYPES.SHORT:
                return 2
            case Serializer.TYPES.BYTE:
                return 1
            case Serializer.TYPES.FLOAT:
                return 4
            case Serializer.TYPES.DOUBLE:
                return 8
            case Serializer.TYPES.INT_ARRAY:
                return 4 + 4 * data.length;
            case Serializer.TYPES.SHORT_ARRAY:
                return 4 + 2 * data.length;
            case Serializer.TYPES.BYTE_ARRAY:
                return 4 + data.length;
            case Serializer.TYPES.FLOAT_ARRAY:
                return 4 + 4 * data.length;
            case Serializer.TYPES.DOUBLE_ARRAY:
                return 4 + 8 * data.length;
            case Serializer.TYPES.VARIANT_ARRAY:
                size = 4; // num keys
                for (let o of data) {
                    size += 1 + 4; // type + pointer
                    size += this.getSize(o);
                }
                return size;
            case Serializer.NULL_VARIANT:
                return 0;
            default:
                console.error("Unsupported serialization type:" + type)
                return 0;
        }
    }

    // Serializes any javascript thing given the type 
    // thing will be written to array_buffer at ptr and the length will be returned
    serialize(thing, type, array_buffer, ptr) {
        switch (type) {
            case Serializer.TYPES.OBJECT:
                return this.serializeObject(thing, array_buffer, ptr);
            case Serializer.TYPES.STRING:
                return this.serializeString(thing, array_buffer, ptr);
            case Serializer.TYPES.INT:
                return this.serializeInt(thing, array_buffer, ptr);
            case Serializer.TYPES.SHORT:
                return this.serializeShort(thing, array_buffer, ptr);
            case Serializer.TYPES.BYTE:
                return this.serializeByte(thing, array_buffer, ptr);
            case Serializer.TYPES.FLOAT:
                return this.serializeFloat(thing, array_buffer, ptr);
            case Serializer.TYPES.DOUBLE:
                return this.serializeDouble(thing, array_buffer, ptr);
            case Serializer.TYPES.INT_ARRAY:
                return this.serializeIntArray(thing, array_buffer, ptr);
            case Serializer.TYPES.SHORT_ARRAY:
                return this.serializeShortArray(thing, array_buffer, ptr);
            case Serializer.TYPES.BYTE_ARRAY:
                return this.serializeByteArray(thing, array_buffer, ptr);
            case Serializer.TYPES.FLOAT_ARRAY:
                return this.serializeFloatArray(thing, array_buffer, ptr);
            case Serializer.TYPES.DOUBLE_ARRAY:
                return this.serializeDoubleArray(thing, array_buffer, ptr);
            case Serializer.TYPES.VARIANT_ARRAY:
                return this.serializeVariantArray(thing, array_buffer, ptr);
            default:
                console.error("Unsupported serialization type:" + type)
                return 0;
        }
    }

    // Serializes an object into the bytebuffer at the given location and returns the length in bytes
    // All keys must be strings, all data must be one of the listed types
    // Object with a few typed arrays are much more efficient than deeply nested objects
    serializeObject(obj, array_buffer, ptr) {
        let start_ptr = ptr;
        let linker = {}; // keep track of where pointers to data are so they can be set to where data gets written
        let num_keys = Object.keys(obj).length;
        //Write the list of keys and types
        ptr += this.serializeInt(num_keys, array_buffer, ptr);
        for (let key in obj) {
            ptr += this.serializeString(key, array_buffer, ptr);
            let data = obj[key];
            let type = Serializer.getType(data);
            ptr += this.serializeByte(type, array_buffer, ptr);
            linker[key] = ptr; // remember location where pointer for this object goes
            ptr += this.serializeInt(0, array_buffer, ptr);// a placeholder for the pointer
        }
        // write the actual objects
        for (let key in obj) {
            let data = obj[key];
            let type = Serializer.getType(data);
            this.serializeInt(ptr - start_ptr, array_buffer, linker[key]); // link into key list the relative location
            ptr += this.serialize(data, type, array_buffer, ptr);
        }
        return ptr - start_ptr; // length of everything
    }

    // Serializes a nontyped JS array into the bytebuffer at the given location and returns the length in bytes
    // Using typed arrays is far more efficient than this
    serializeVariantArray(array, array_buffer, ptr) {
        let start_ptr = ptr;
        let linker = {}; // keep track of where pointers to data are so they can be set to where data gets written
        let length = array.length;
        //Write the list of types and allocate space for pointers
        ptr += this.serializeInt(length, array_buffer, ptr);
        for (let k = 0; k < array.length; k++) {
            let data = array[k];
            let type = Serializer.getType(data);
            ptr += this.serializeByte(type, array_buffer, ptr);
            linker[k] = ptr; // remember location where pointer for this object goes
            ptr += this.serializeInt(0, array_buffer, ptr);// a placeholder for the pointer
        }
        // write the actual objects
        for (let k = 0; k < array.length; k++) {
            let data = array[k];
            let type = Serializer.getType(data);
            this.serializeInt(ptr - start_ptr, array_buffer, linker[k]); // link into key list the relative location
            ptr += this.serialize(data, type, array_buffer, ptr);
        }
        return ptr - start_ptr; // length of everything
    }

    // Serializes a string into the array_buffer at the given location and returns the length in bytes
    serializeString(string, array_buffer, ptr) {
        const encoder = new TextEncoder();
        let bytes = encoder.encode(string);
        return this.serializeByteArray(bytes, array_buffer, ptr);
    }

    // Serializes a 4 byte integer into the array_buffer at the given location and returns 4
    serializeInt(number, array_buffer, ptr) {
        // Array buffer doesn't allow misaligned view setting, so have to copy through byte array
        let arr = new Int32Array(1);
        arr[0] = number;
        let view = new Int8Array(array_buffer, ptr, 4);
        view.set(new Int8Array(arr.buffer));
        return 4;
    }

    // Serializes a 2 byte integer into the array_buffer at the given location and returns 2
    serializeShort(number, array_buffer, ptr) {
        // Array buffer doesn't allow misaligned view setting, so have to copy through byte array
        let arr = new Int16Array(1);
        arr[0] = number;
        let view = new Int8Array(array_buffer, ptr, 2);
        view.set(new Int8Array(arr.buffer));
        return 2;
    }

    // Serializes a byte into the array_buffer at the given location and returns 1
    serializeByte(number, array_buffer, ptr) {
        let view = new Int8Array(array_buffer, ptr, 1);
        view[0] = number;
        return 1;
    }

    // Serializes a 4 byte float into the array_buffer at the given location and returns 4
    serializeFloat(number, array_buffer, ptr) {
        // Array buffer doesn't allow misaligned view setting, so have to copy through byte array
        let arr = new Float32Array(1);
        arr[0] = number;
        let view = new Int8Array(array_buffer, ptr, 4);
        view.set(new Int8Array(arr.buffer));
        return 4;
    }

    // Serializes an 8 byte double into the array_buffer at the given location and returns 8
    serializeDouble(number, array_buffer, ptr) {
        // Array buffer doesn't allow misaligned view setting, so have to copy through byte array
        let arr = new Float64Array(1);
        arr[0] = number;
        let view = new Int8Array(array_buffer, ptr, 8);
        view.set(new Int8Array(arr.buffer));
        return 8;
    }

    // Serializes an Int32Array into the array_buffer and returns the length in bytes
    serializeIntArray(int_array, array_buffer, ptr) {
        let length = int_array.length;
        this.serializeInt(length, array_buffer, ptr);
        // Array buffer doesn't allow misaligned view setting, so have to copy through byte array
        let view = new Int8Array(array_buffer, ptr + 4, 4 * length);
        view.set(new Int8Array(int_array.buffer));
        return 4 + 4 * length;
    }

    // Serializes an Int16Array into the array_buffer and returns the length in bytes
    serializeShortArray(short_array, array_buffer, ptr) {
        let length = short_array.length;
        this.serializeInt(length, array_buffer, ptr);
        // Array buffer doesn't allow misaligned view setting, so have to copy through byte array
        let view = new Int8Array(array_buffer, ptr + 4, 2 * length);
        view.set(new Int8Array(short_array.buffer));
        return 4 + 2 * length;
    }

    // Serializes an Int8Array into the array_buffer and returns the length in bytes
    serializeByteArray(byte_array, array_buffer, ptr) {
        let length = byte_array.length;
        this.serializeInt(length, array_buffer, ptr);
        let view = new Int8Array(array_buffer, ptr + 4, length);
        view.set(byte_array);
        return 4 + length;
    }

    // Serializes a FloatArray into the array_buffer and returns the length in bytes
    serializeFloatArray(float_array, array_buffer, ptr) {
        let length = float_array.length;
        this.serializeInt(length, array_buffer, ptr);
        // Array buffer doesn't allow misaligned view setting, so have to copy through byte array
        let view = new Int8Array(array_buffer, ptr + 4, 4 * length);
        view.set(new Int8Array(float_array.buffer));
        return 4 + 4 * length;
    }

    // Serializes a Float64Array into the array_buffer and returns the length in bytes
    serializeDoubleArray(double_array, array_buffer, ptr) {
        let length = double_array.length;
        this.serializeInt(length, array_buffer, ptr);
        // Array buffer doesn't allow misaligned view setting, so have to copy through byte array
        let view = new Int8Array(array_buffer, ptr + 4, 8 * length);
        view.set(new Int8Array(double_array.buffer));
        return 4 + 8 * length;
    }

    // Deserializes and returns any javascript thing given the type 
    // thing will be read from the given buffer at ptr and must be the given length
    deserialize(type, array_buffer, ptr) {
        switch (type) {
            case Serializer.TYPES.OBJECT:
                return this.deserializeObject(array_buffer, ptr);
            case Serializer.TYPES.STRING:
                return this.deserializeString(array_buffer, ptr);
            case Serializer.TYPES.INT:
                return this.deserializeInt(array_buffer, ptr);
            case Serializer.TYPES.SHORT:
                return this.deserializeShort(array_buffer, ptr);
            case Serializer.TYPES.BYTE:
                return this.deserializeByte(array_buffer, ptr);
            case Serializer.TYPES.FLOAT:
                return this.deserializeFloat(array_buffer, ptr);
            case Serializer.TYPES.DOUBLE:
                return this.deserializeDouble(array_buffer, ptr);
            case Serializer.TYPES.INT_ARRAY:
                return this.deserializeIntArray(array_buffer, ptr);
            case Serializer.TYPES.SHORT_ARRAY:
                return this.deserializeShortArray(array_buffer, ptr);
            case Serializer.TYPES.BYTE_ARRAY:
                return this.deserializeByteArray(array_buffer, ptr);
            case Serializer.TYPES.FLOAT_ARRAY:
                return this.deserializeFloatArray(array_buffer, ptr);
            case Serializer.TYPES.DOUBLE_ARRAY:
                return this.deserializeDoubleArray(array_buffer, ptr);
            case Serializer.TYPES.VARIANT_ARRAY:
                return this.deserializeVariantArray(array_buffer, ptr);
            case Serializer.TYPES.NULL_VARIANT:
                return null ;
            default:
                console.error("Unsupported serialization type:" + type);
                console.trace();
                return 0;
        }
    }

    // deserializes an object from the array_buffer
    // relies on other deserialize functions to set last_deserialized_length
    deserializeObject(array_buffer, ptr) {
        let start_ptr = ptr;
        let obj = {};
        let num_keys = this.deserializeInt(array_buffer, ptr);
        ptr += this.last_deserialized_length;
        let rel_ptr; // need to read last value after loop to get length of object
        for (let k = 0; k < num_keys; k++) {
            let key = this.deserializeString(array_buffer, ptr);
            ptr += this.last_deserialized_length;
            let type = this.deserializeByte(array_buffer, ptr);
            ptr += this.last_deserialized_length;
            rel_ptr = this.deserializeInt(array_buffer, ptr);
            ptr += this.last_deserialized_length;
            obj[key] = this.deserialize(type, array_buffer, start_ptr + rel_ptr);
        }
        this.last_deserialized_length = rel_ptr + this.last_deserialized_length;
        return obj;
    }

    // deserializes a JS style variant array from the array_buffer
    // relies on other deserialize functions to set last_deserialized_length
    deserializeVariantArray(array_buffer, ptr) {
        let start_ptr = ptr;
        let length = this.deserializeInt(array_buffer, ptr);
        let array = new Array(length);
        ptr += this.last_deserialized_length;
        let rel_ptr; // need to read last value after loop to get length of object
        for (let k = 0; k < length; k++) {
            let type = this.deserializeByte(array_buffer, ptr);
            ptr += this.last_deserialized_length;
            rel_ptr = this.deserializeInt(array_buffer, ptr);
            ptr += this.last_deserialized_length;
            array[k] = this.deserialize(type, array_buffer, start_ptr + rel_ptr);
        }
        this.last_deserialized_length = rel_ptr + this.last_deserialized_length;
        return array;
    }

    /* Deserialize functions match to serialize functions
       All deserialize functions set the public class var last_deserialized_length 
       which is the length of the last thing deserialized

       Javascript does not support creating an array buffer view that is not "aligned" (i.e. Int32Array has to start where ptr%4=0)
       so many functions have an extra copy on read and write. It would likely be faster to pad out serialized data to avoid these copies
       but it would break if copied to a different buffer with a different alignment. Might be worth looking into if these are too slow.
    */



       utf8_read(buffer, start, end) {
        var len = end - start;
        if (len < 1)
            return "";
        var parts = null,
            chunk = [],
            i = 0, // char offset
            t;     // temporary
        while (start < end) {
            t = buffer[start++];
            if (t < 128)
                chunk[i++] = t;
            else if (t > 191 && t < 224)
                chunk[i++] = (t & 31) << 6 | buffer[start++] & 63;
            else if (t > 239 && t < 365) {
                t = ((t & 7) << 18 | (buffer[start++] & 63) << 12 | (buffer[start++] & 63) << 6 | buffer[start++] & 63) - 0x10000;
                chunk[i++] = 0xD800 + (t >> 10);
                chunk[i++] = 0xDC00 + (t & 1023);
            } else
                chunk[i++] = (t & 15) << 12 | (buffer[start++] & 63) << 6 | buffer[start++] & 63;
            if (i > 8191) {
                (parts || (parts = [])).push(String.fromCharCode.apply(String, chunk));
                i = 0;
            }
        }
        if (parts) {
            if (i)
                parts.push(String.fromCharCode.apply(String, chunk.slice(0, i)));
            return parts.join("");
        }
        return String.fromCharCode.apply(String, chunk.slice(0, i));
    };

    deserializeString(array_buffer, ptr) {
        let bytes = this.deserializeByteArray(array_buffer, ptr);
        return this.utf8_read(bytes, 0, bytes.length);
        //const decoder = new TextDecoder();
        //return decoder.decode(bytes);
    }

    deserializeInt(array_buffer, ptr) {
        this.last_deserialized_length = 4;
        let bytes = new Uint8Array(array_buffer, ptr, 4);
        return bytes[0] + (bytes[1]<<8) +(bytes[2] <<16) + (bytes[3] << 24) ;
    }

    deserializeShort(array_buffer, ptr) {
        this.last_deserialized_length = 2;
        let bytes = new Uint8Array(array_buffer, ptr, 2);
        return bytes[0] + (bytes[1]<<8) ;
    }

    deserializeByte(array_buffer, ptr) {
        this.last_deserialized_length = 1;
        return new Int8Array(array_buffer, ptr, 1)[0];
    }

    deserializeFloat(array_buffer, ptr) {
        this.last_deserialized_length = 4;
        return new Float32Array(array_buffer.slice(ptr, ptr + 4), 0, 1)[0];
    }

    deserializeDouble(array_buffer, ptr) {
        this.last_deserialized_length = 8;
        return new Float64Array(array_buffer.slice(ptr, ptr + 8), 0, 1)[0];
    }

    deserializeIntArray(array_buffer, ptr) {
        let length = this.deserializeInt(array_buffer, ptr);
        this.last_deserialized_length = 4 + length * 4;
        return new Int32Array(array_buffer.slice(ptr + 4, ptr + 4 + 4 * length), 0, length);
    }

    deserializeShortArray(array_buffer, ptr) {
        let length = this.deserializeInt(array_buffer, ptr);
        this.last_deserialized_length = 4 + length * 2;
        return new Int16Array(array_buffer.slice(ptr + 4, ptr + 4 + 2 * length), 0, length);
    }

    deserializeByteArray(array_buffer, ptr) {
        let length = this.deserializeInt(array_buffer, ptr);
        this.last_deserialized_length = 4 + length;
        return new Uint8Array(array_buffer, ptr + 4, length);
    }

    deserializeFloatArray(array_buffer, ptr) {
        let length = this.deserializeInt(array_buffer, ptr);
        this.last_deserialized_length = 4 + length * 4;
        return new Float32Array(array_buffer.slice(ptr + 4, ptr + 4 + 4 * length), 0, length);
    }

    deserializeDoubleArray(array_buffer, ptr) {
        let length = this.deserializeInt(array_buffer, ptr);
        this.last_deserialized_length = 4 + length * 8;
        return new Float64Array(array_buffer.slice(ptr + 4, ptr + 4 + 8 * length), 0, length);
    }
}