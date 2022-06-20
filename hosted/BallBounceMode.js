class BallBounceMode extends ExecutionMode{
    /* Input trackers. */

    // Mouse position
	mouse_x;
	mouse_y;
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

        tools.API.call("runTimeline", {}, new Serializer());
        
        


    }

    // Called when the app should be redrawn
    // Note: the elements to draw onto or with should be included in the tools on construction and saved for the duration of the mode
    drawFrame(frame_id){
        tools.API.call("runTimeline", {}, new Serializer());
        this.observables = tools.API.call("getTimelineCircles", {}, new Serializer()).observables;

        //console.log(observables);
        let context = tools.canvas.getContext("2d");
        context.clearRect(0, 0, context.canvas.width, context.canvas.height);

        for(let k=0;k<this.observables.length;k+=4){
            let x = this.observables[k];
            let y = this.observables[k+1];
            let r = this.observables[k+2];
            this.drawCircle(x, y, r, "#004F00", "#000000", 2) ;
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
            for(let k=0;k<this.observables.length;k+=4){
                let x = this.observables[k];
                let y = this.observables[k+1];
                let r = this.observables[k+2];
                let id = this.observables[k+3];
                let dist2 = (x-this.mouse_down_x)*(x-this.mouse_down_x) + (y-this.mouse_down_y)*(y-this.mouse_down_y) ;
                if(dist2 < r*r){
                    tools.API.call("randomizeBallVelocity",{id:id,max_speed:300}, new Serializer());
                }
            }
        }else{

            
        }
    }

	pointerMove(pointers){
		this.mouse_x = pointers[0].x;
		this.mouse_y = pointers[0].y;
		if(this.mouse_down){
            if(this.dragging){
  
            }else if(this.rotating){
            }
		}
    }

    pointerUp(pointers){ // Note only pointers still down
        if(!this.mouse_down){
			return ;
        }
        if(this.dragging){
            
        }
        this.mouse_down = false ;
        this.dragging = false;
        this.rotating = false;
    }

    mouseWheelListener(event){
        // Firefox and Chrome get different numbers from different events. One is 3 the other is -120 per notch.
        var scroll = event.wheelDelta ? -event.wheelDelta*.2 : event.detail*8; 
        
    }

	keyDownListener(event){
	    var key_code = event.keyCode || event.which;
        var character = String.fromCharCode(key_code); // This only works with letters. Use key-code directly for others.
        console.log(character);
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