var max_packet_size = 30000000; // maximum paramters or return size in bytes
var packet_ptr;

function _serializeToHeap(thing, serializer) {
    serializer.serialize(thing, serializer.constructor.getType(thing), Module.HEAPU8.buffer, packet_ptr);
}

function _deserializeFromHeap(serializer, type) {
    let out = serializer.deserialize(type, Module.HEAPU8.buffer, packet_ptr);
    return out;
}

Module["call"] = function (function_name, parameters = null, serializer = null, skip_output_deserialization = false) {
    if(!packet_ptr){
        packet_ptr = Module._malloc(max_packet_size);
        console.log("APIPost call Packet pointer mallocated:" + packet_ptr);
        return Module.ccall('setPacketPointer', 'number', ['number'], [packet_ptr]);
    }
    if (parameters != null)  {
        if (serializer == null) { // if no serializer then assume being passed int8Array
            if (parameters instanceof Int8Array) {
                let view = new Int8Array(Module.HEAPU8.buffer, packet_ptr, parameters.length);
                view.set(parameters); // copy data
            } else {
                console.log("Either pass a serializer or use an int8array of already serialized data for parameters.");
                return;
            }
        } else {
            _serializeToHeap(parameters, serializer);
        }
    }
    var output_ptr = Module.ccall(function_name, 'number', ['number'], [packet_ptr]);
    if (serializer == null || skip_output_deserialization) { // Function is returning raw bytes htta may be outside safe packet
        return output_ptr;
    } else {
        let output = serializer.deserialize(Serializer.TYPES.OBJECT, Module.HEAPU8.buffer, output_ptr);
        return output;
    }

}