class BallBounceMode extends ExecutionMode{
    /* Input trackers. */

    // Mouse position
	mouse_x;
	mouse_y;
    mouse_time;
    last_mouse_x ;
    last_mouse_y ;
    last_mouse_time ;
	// Where the mouse was last pressed down.
	mouse_down_x;
	mouse_down_y;
	//Whether the mouse is currently down.
	mouse_down = false ;
	//Which button is down (0 is left, 2 is right)
    mouse_button ;

    dragging = false;
    held_id ; // currently held id

    start_time ;
    last_time ;

    balls = []; // packed array of x,y,radius,id


    // Tools is an object with string keys that may include things such as the canvas,
    // API WASM Module, an Interface manager, and/or a mesh manager for shared webGL functionality
    constructor(tools){
        super(tools) ;

    }

    // Called when the mode is becoming active (previous mode will have already exited)
    enter(previous_mode){
        this.start_time = new Date().getTime();
        this.last_time = this.start_time ;
        

        

    }

    // Called when a mode is becoming inactive (prior to calling to enter on that mode)
    exit(next_mode){
    }

    // Called at regular intervals by the main app when the mode is active (30 or 60fps probably but not guarsnteed)
    timer(){
        /*
        let timeline_time = (new Date().getTime() - this.start_time)*0.001;
        console.log("time : " + timeline_time);
        let observables = tools.API.call("runTimeline", {time:timeline_time}, new Serializer()).observables;
        console.log(observables);
        */
       /*
        let current_time = new Date().getTime(); // limit amount to step in 1 frame to 1 second
        if(current_time - this.last_time > 1000){
            current_time = this.last_time + 1000;
        }
        this.last_time = current_time ;
        let timeline_time = (current_time - this.start_time)*0.001;

        if(timeline_time < 0.5){
            return ;
        }
        
        if(!this.loaded){
            console.log("initializing balls in JS!");
            this.loaded = true ;

            var params = {};
            params.width = tools.canvas.width;
            params.height = tools.canvas.height;
            params.amount = 500;
            params.min_radius = 10;
            params.max_radius = 15;
            params.max_speed = 200;

            //tools.API.call("runTimeline", {time:timeline_time}, new Serializer()).observables;

            tools.API.call("initialize2DBallTimeline", params, new Serializer()); // TODO why does the first call not call?
            tools.API.call("initialize2DBallTimeline", params, new Serializer());
        }
        */

        if(this.dragging){

            for(let k=0;k<this.observables.length;k+=this.stride){
                let id = this.observables[k];
                let type = this.observables[k+1];
                if(id == this.drag_id){
                    let x = this.observables[k+2]; // dragged items are gauranteed to be balls
                    let y = this.observables[k+3];
                    let r = this.observables[k+4];

                    let dist2 = (x-this.mouse_x)*(x-this.mouse_x) + (y-this.mouse_y)*(y-this.mouse_y) ;
                    let new_v = [0,0,0] ;
                    if(dist2 < r*r*.25){ // if mouse is on held object
                        // set velocity to mouse velocity
                        new_v[0] = (this.mouse_x - this.last_mouse_x) / (this.mouse_time  - this.last_mouse_time);
                        new_v[1] = (this.mouse_y - this.last_mouse_y) / (this.mouse_time  - this.last_mouse_time);
                    }else{
                        // move toward mouse at high speed (half the distance each tick)
                        new_v[0] = (this.mouse_x - x) ;
                        new_v[1] = (this.mouse_y - y) ;
                        let s = 200.0/Math.sqrt(new_v[0]*new_v[0] + new_v[1]*new_v[1]);
                        new_v[0]*= s ;
                        new_v[1]*= s ;
                    }
                    if(!this.last_v){
                        this.last_v = [-9999,-9999,-9999];
                    }
                    let change_mag = (new_v[0] - this.last_v[0])*(new_v[0] - this.last_v[0]) + (new_v[1] - this.last_v[1])*(new_v[1] - this.last_v[1]) ;
                    if(change_mag > 1000){
                        this.last_v = new_v;
                        tools.API.call("setBallVelocity",{id:this.drag_id,v:new Float32Array(new_v)}, new Serializer());
                    }else{
                        console.log("notenug hcvnasge!");
                    }
                }
            }
        }

        tools.API.call("runTimeline", {}, new Serializer());
        
        


    }

    // Called when the app should be redrawn
    // Note: the elements to draw onto or with should be included in the tools on construction and saved for the duration of the mode
    drawFrame(frame_id){
        tools.API.call("runTimeline", {}, new Serializer());
        let render_data = tools.API.call("getBallObjects", {}, new Serializer()) ;
        this.observables = render_data.observables;
        this.stride = render_data.stride ;

        //console.log(observables);
        let context = tools.canvas.getContext("2d");

        for(let k=0;k<this.observables.length;k+=this.stride){
            let id = this.observables[k];
            let type = this.observables[k+1];
            if(type == 1){ // ball
                let x = this.observables[k+2]; 
                let y = this.observables[k+3];
                let r = this.observables[k+4];

                this.drawCircle(x, y, r, null, "#004F00", 4) ;
            }else if(type == 2){ // wall
                let min_x = this.observables[k+2];
                let min_y = this.observables[k+3];
                let max_x = this.observables[k+4];
                let max_y = this.observables[k+5];
                InterfaceButton.drawRoundedRect(context, min_x, min_y, max_x-min_x, max_y-min_y, 2, null,  "#300030", 4);

            }
            /*
            let p = observables[k].p;
            let radius = observables[k].r ;
            this.drawCircle(p[0], p[1], radius, "#004F00", "#000000", 2) ;
            */
        }
        
    }


    pointerDown(pointers){
        if(this.mouse_down){ // ignore second mouse click while dragging
			return ;
		}
		this.mouse_down_x = pointers[0].x;
		this.mouse_down_y = pointers[0].y;
		this.mouse_down = true ;
		this.mouse_button = pointers[0].button ;
        
        if(this.mouse_button != 2){
            
            for(let k=0;k<this.observables.length;k+=this.stride){
                let id = this.observables[k];
                let type = this.observables[k+1];
                if(type == 1){ // ball
                    let x = this.observables[k+2]; 
                    let y = this.observables[k+3];
                    let r = this.observables[k+4];
                    let dist2 = (x-this.mouse_down_x)*(x-this.mouse_down_x) + (y-this.mouse_down_y)*(y-this.mouse_down_y) ;
                    if(dist2 < r*r){
                        this.dragging = true ;
                        this.drag_id = id ;
                        //tools.API.call("randomizeBallVelocity",{id:id,max_speed:300}, new Serializer());
                    }
                }
            }
        }else{

            
        }
    }

	pointerMove(pointers){
        let new_mouse_time = new Date().getTime() ;
        if(new_mouse_time != this.mouse_time){
            this.last_mouse_x = this.mouse_x ;
            this.last_mouse_y = this.mouse_y ;
            this.last_mouse_time =  this.mouse_time;
        }
        this.mouse_time = new_mouse_time;
		this.mouse_x = pointers[0].x;
		this.mouse_y = pointers[0].y;
		if(this.mouse_down){
            
		}
    }

    pointerUp(pointers){ // Note only pointers still down
        if(!this.mouse_down){
			return ;
        }
        if(this.dragging){
            /*
            let id = this.drag_id;
            let new_v = [0,0,0] ;
            // set velocity to mouse velocity
            new_v[0] = (this.mouse_x - this.last_mouse_x) / (1+(this.mouse_time  - this.last_mouse_time));
            new_v[1] = (this.mouse_y - this.last_mouse_y) / (1+(this.mouse_time  - this.last_mouse_time));
            let s = 300.0/Math.sqrt(new_v[0]*new_v[0] + new_v[1]*new_v[1]);
            if( s < 1.0){
                new_v[0]*= s ;
                new_v[1]*= s ;
            }
            tools.API.call("setBallVelocity",{id:id,v:new Float32Array(new_v)}, new Serializer());
            */
        }
        this.mouse_down = false ;
        this.dragging = false;
    }

    mouseWheelListener(event){
        // Firefox and Chrome get different numbers from different events. One is 3 the other is -120 per notch.
        var scroll = event.wheelDelta ? -event.wheelDelta*.2 : event.detail*8; 
        
    }

	keyDownListener(event){
	    var key_code = event.keyCode || event.which;
        var character = String.fromCharCode(key_code); // This only works with letters. Use key-code directly for others.
        //console.log(character);
        if(key_code == 104){// numpad 8
            tools.sync_link.update_delay += 5;
        }else if(key_code == 98){ // numpad 2
            tools.sync_link.update_delay -= 5;
            if(tools.sync_link.update_delay < 0){
                tools.sync_link.update_delay = 0 ;
            }
        }
    }

	keyUpListener(event){

    }

    vrInputSourcesUpdated(xr_input){

    }


    drawCircle(x, y, radius, fill_color, stroke_color, stroke_size){
        let context = tools.canvas.getContext("2d");
		context.beginPath();
		context.arc(x, y, radius, 0, 2 * Math.PI, false);
		context.closePath();
		if(fill_color != null){
			context.fillStyle = fill_color;
			context.fill();
		}
		if(stroke_color != null){
			context.lineWidth = stroke_size;
			context.strokeStyle = stroke_color;
			context.stroke();
		}
	}
}