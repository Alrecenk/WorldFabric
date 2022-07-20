class ScanMode extends ExecutionMode{
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
    held_id ; // currently held box id
    
    rotating = false;
    camera_zoom;
    camera_focus = [0,0,0];

    axis_total = 0 ;

    model_pose = null;

    // Tools is an object with string keys that may include things such as the canvas,
    // API WASM Module, an Interface manager, and/or a mesh manager for shared webGL functionality
    constructor(tools){
        super(tools) ;
        this.model_pose = mat4.create();
        mat4.identity(this.model_pose);
    }

    // Called when the mode is becoming active (previous mode will have already exited)
    enter(previous_mode){
        this.camera_zoom = tools.renderer.getZoom(0); 
    }

    // Called when a mode is becoming inactive (prior to calling to enter on that mode)
    exit(next_mode){
    }

    // Called at regular intervals by the main app when the mode is active (30 or 60fps probably but not guarsnteed)
    timer(){

    }

    // Called when the app should be redrawn
    // Note: the elements to draw onto or with should be included in the tools on construction and saved for the duration of the mode
    drawFrame(frame_id){
        // Get any mesh updates pending in the module
        if(frame_id == 0){
            let new_buffer_data = tools.API.call("getUpdatedBuffers", null, new Serializer());
            for(let id in new_buffer_data){
                tools.renderer.prepareBuffer(id, new_buffer_data[id]);
            }
        }
        // Draw the models
        let bones = tools.API.call("getBones", {mesh:"MAIN"}, new Serializer()).bones ;
        tools.renderer.drawMesh("MAIN", this.model_pose, bones);
        /*for(let dz = -1; dz <=1; dz++){
            for(let dy = -1; dy <=1; dy++){
                for(let dx = -1; dx <=1; dx++){
                    let M = mat4.create();
                    mat4.translate(M, this.model_pose,[dx*1.5,dy*1.5,dz*1.5]);
                    tools.renderer.drawMesh("MAIN", M);
                }
            }
        }*/
        
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
            this.rotating = true;
            this.tools.renderer.startRotate(this.camera_focus, pointers[0]);
        }else{

            let ray = tools.renderer.getRay([this.mouse_down_x,this.mouse_down_y]);
            console.log(tools.API.call("scan", ray, new Serializer())); 
            
            
            
        }
    }

	pointerMove(pointers){
		this.mouse_x = pointers[0].x;
		this.mouse_y = pointers[0].y;
		if(this.mouse_down){
            if(this.dragging){
                /*
                let ray = tools.renderer.getRay([this.mouse_x,this.mouse_y]);
                let trace_data = tools.API.call("rayTrace", ray, new Serializer()); 
                let t = trace_data.t ;
                if(t > 0){
                    
                }
                */
            }else if(this.rotating){
               this.tools.renderer.continueRotate(pointers[0]);
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
        // Adjust camera zoom by mouse wheel scroll.
		this.camera_zoom *= Math.pow(1.005,scroll);
        tools.renderer.setZoom(this.camera_zoom) ;
    }

	keyDownListener(event){
	    var key_code = event.keyCode || event.which;
        var character = String.fromCharCode(key_code); // This only works with letters. Use key-code directly for others.
        console.log(character);
        if(character == 'M'){
            console.log("testing memory:");
            let params = {};
                params.size = 100000000;
                params.num = 1 ; 
                let ptr = tools.API.call("testAllocate", params, new Serializer()).ptr;
                console.log(ptr);

        }
    }

	keyUpListener(event){

    }

    vrInputSourcesUpdated(xr_input){
        let any_grab = false;
        //console.log(xr_input);
        for (let input_source of xr_input) {
            if(input_source.grip_pose){
                let grabbing = false; 
                for(let button of input_source.buttons){
                    grabbing = grabbing || button.pressed;
                    if(button.pressed){
                        console.log("scanmode saw button press!");
                        console.log(button);
                    }
                }
                for(let axis of input_source.axes){
                    this.axis_total += axis ;
                }
            
                any_grab = any_grab || grabbing ;
                
                // start of grab, fetch starting poses
                if(grabbing && !this.grab_pose){
                    this.grab_pose = mat4.create();
                    this.grab_pose.set(input_source.grip_pose);
                    this.grab_model_pose = mat4.create();
                    this.grab_model_pose.set(this.model_pose) ;
                    this.grab_axis_total = this.axis_total ;
                }
                
                if(grabbing){
                    let MP = mat4.create();
                    let inv = mat4.create();
                    mat4.invert(inv, this.grab_pose); // TODO cache at grab time
                    mat4.multiply(MP,inv, MP);
                    mat4.multiply(MP,input_source.grip_pose, MP);
                    let scale = Math.pow(1.05, (this.axis_total - this.grab_axis_total)*0.3);
                    mat4.scale(MP, MP,[scale,scale,scale]);
                    mat4.multiply(this.model_pose, MP, this.grab_model_pose);
                    break ; // don't check next controllers
                }
            }
        }
        if(!any_grab){// stopped grabbing, clear saved poses
            this.grab_pose = null;
            this.grab_model_pose = null;
        }
    }
}