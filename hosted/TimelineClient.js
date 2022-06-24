var timeline_client ; //TODO singleton patterned global is not great
var wasm_module;
var last_sync ;
var quick_send = false;
class TimelineClient{
    //The address with protocol and port
    address = null;
    //The websocket
    socket = null ; 
    active = false;
    update_delay = 5; // milliseconds to wait before actually sending a packet (used for simulating latency)
    sync_ping = 0 ;
    
    // Opens a websocket on creation to connect to a TableServer (cpp) on the same server as the web-hosting
    constructor(port, module){
        wasm_module = module ;
        timeline_client = this;
        this.address = "ws://" + location.hostname + ":" + port;
        this.socket = new WebSocket(this.address);
        this.socket.binaryType = 'arraybuffer';
        this.socket.onmessage = function(msg) {
            if(msg.data instanceof ArrayBuffer){
                if(timeline_client.active){
                    let byte_array = new Int8Array(msg.data);
                    //timeline_client.synchronizeTimeline(byte_array) ;
                    setTimeout(timeline_client.synchronizeTimeline, timeline_client.update_delay, byte_array);
                }
            }else{
                console.log("Timeline Server sent unrecognized formatted data:");
                console.log(msg.data);
            }
        };
    }

    ready(){
        return this.socket.readyState == WebSocket.OPEN ;
    }

    send(message){
        timeline_client.socket.send(message);
    }

    sendUpdate(message){
        setTimeout(timeline_client.send, timeline_client.update_delay, message);
    }

    

    // Sends all table data requests currently pending on any WASM TableReader implementers
    // If active is set to true, pending requests will be sent continuously until active is set back to false
    sendInitialRequest(active=true){
        this.active = active;

        let request_ptr = wasm_module.call("getInitialTimelineRequest") ;
        request_ptr = wasm_module.call("getInitialTimelineRequest") ; // TODO for some reason the first wasm module call always fails

        //console.log("pointer:" + request_ptr);
        let request_size = wasm_module.getReturnSize();
        let request = wasm_module.getByteArray(request_ptr, request_size);
        //console.log("request bytes:" + request);
        this.socket.send(request.buffer);
    }

    synchronizeTimeline(packet_byte_array, active=true){
        if(!last_sync){
            last_sync = new Date().getTime();
        }
        timeline_client.sync_ping = (new Date().getTime() - last_sync) ;
        last_sync = new Date().getTime();
        timeline_client.active = active;
        let request_ptr = wasm_module.call("synchronizeTimeline", packet_byte_array); 
        let request_size = wasm_module.getReturnSize();
        let request = wasm_module.getByteArray(request_ptr, request_size);
        //this.socket.send(request.buffer);
        timeline_client.sendUpdate(request.buffer);
        timeline_client.quick_send = true ;
    }

    sendQuickEvents(){
        if(this.quick_send){
            let request_ptr = wasm_module.call("popPendingQuickSends", new Int8Array(1)); //TODO support null params on serializerless wasm calls
            let request_size = wasm_module.getReturnSize();
            let request = wasm_module.getByteArray(request_ptr, request_size);
            if(request_size > 10){
                //this.socket.send(request.buffer);
                timeline_client.sendUpdate(request.buffer);
            }
        }
    }


}