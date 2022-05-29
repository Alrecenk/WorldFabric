var table_client ; //TODO singleton patterned global is not great
var table_module;
class TableClient{
    //The address with protocol and port
    address = null;
    //The websocket
    socket = null ; 
    active = false;

    
    // Opens a websocket on creation to connect to a TableServer (cpp) on the same server as the web-hosting
    constructor(port, module){
        table_module = module ;
        table_client = this;
        this.address = "ws://" + location.hostname + ":" + port;
        this.socket = new WebSocket(this.address);
        this.socket.binaryType = 'arraybuffer';
        this.socket.onmessage = function(msg) {
            if(msg.data instanceof ArrayBuffer){
                let byte_array = new Int8Array(msg.data);
                table_module.call("distributeTableNetworkData", byte_array); 
                if(table_client.active){
                    table_client.sendPendingRequests();
                }
            }else{
                console.log("Server sent unrecognized formatted data:");
                console.log(msg.data);
            }
        };
    }

    ready(){
        return this.socket.readyState == WebSocket.OPEN ;
    }

    // Sends all table data requests currently pending on any WASM TableReader implementers
    // If active is set to true, pending requests will be sent continuously until active is set back to false
    sendPendingRequests(active=false){
        this.active = active;
        let request_ptr = table_module.call("getTableNetworkRequest") ;
        let request_size = table_module.getReturnSize();
        let request = table_module.getByteArray(request_ptr, request_size);
        this.socket.send(request.buffer);
    }


}