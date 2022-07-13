var table_client ; //TODO singleton patterned global is not great
var table_module;
class TableClient{
    //The address with protocol and port
    address = null;
    //The websocket
    socket = null ; 
    active = false;

    last_response = 0;
    ping = -1;
    active_delay = 2000;

    
    // Opens a websocket on creation to connect to a TableServer (cpp) on the same server as the web-hosting
    constructor(port, module){
        table_module = module ;
        table_client = this;
        try {
            this.address = "ws://" + location.hostname + ":" + port;
            this.socket = new WebSocket(this.address);
            this.socket.binaryType = 'arraybuffer';
            this.socket.onmessage = function(msg) {
                let time = new Date().getTime() ;
                table_client.ping = time - table_client.last_response ;
                table_client.last_response = time ;
                if(msg.data instanceof ArrayBuffer){
                    let byte_array = new Int8Array(msg.data);
                    table_module.call("distributeTableNetworkData", byte_array); 
                    if(table_client.active){
                        setTimeout(table_client.sendPendingRequests, table_client.active_delay, table_client.active);
                    }
                }else{
                    console.log("Server sent unrecognized formatted data:");
                    console.log(msg.data);
                }
            };
        }catch(error){
            console.error(error);
        }
    }

    ready(){
        return this.socket.readyState == WebSocket.OPEN;
    }
    timedOut(){
        return (new Date().getTime() - this.last_response) > 5000 ;
    }

    // Sends all table data requests currently pending on any WASM TableReader implementers
    // If active is set to true, pending requests will be sent continuously until active is set back to false
    sendPendingRequests(active=false){
        table_client.active = active;
        let request_ptr = table_module.call("getTableNetworkRequest") ;
        if(request_ptr){ // can cometimes be called before table_module fully initialized
            let request_size = table_module.getReturnSize();
            let request = table_module.getByteArray(request_ptr, request_size);
            table_client.socket.send(request.buffer);
        }else{
            table_client.active = false;
        }
    }


}