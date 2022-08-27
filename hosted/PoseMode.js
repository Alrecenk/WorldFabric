class PoseMode extends ExecutionMode{
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

    hand_pose = [mat4.create(),mat4.create()] ;

    pin = null;

    // Tools is an object with string keys that may include things such as the canvas,
    // API WASM Module, an Interface manager, and/or a mesh manager for shared webGL functionality
    constructor(tools){
        super(tools) ;
        this.model_pose = mat4.create();
        mat4.identity(this.model_pose);

        mat4.translate(this.hand_pose[0], this.model_pose,[2.5,1.5,0]);
        mat4.translate(this.hand_pose[1], this.model_pose,[-2.5,1.5,0]);
        
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
        let bones = tools.API.call("getBones", {mesh:"default_avatar"}, new Serializer()).bones ;
        tools.renderer.drawMesh("default_avatar", this.model_pose, bones);
        tools.renderer.drawMesh("HAND", this.hand_pose[0]);
        tools.renderer.drawMesh("HAND", this.hand_pose[1]);

    }


    pointerDown(pointers){
        if(this.mouse_down){ // ignore second mouse click while dragging
			return ;
		}
		this.mouse_down_x = pointers[0].x;
		this.mouse_down_y = pointers[0].y;
		this.mouse_down = true ;
		this.mouse_button = pointers[0].button ;
        
        if(this.mouse_button == 0){
            this.rotating = true;
            this.tools.renderer.startRotate(this.camera_focus, pointers[0]);
        }else{

            let ray = tools.renderer.getRay([this.mouse_down_x,this.mouse_down_y]);
            ray.name = "drag";
            tools.API.call("createPin", ray, new Serializer()); 
            this.dragging = true;
        }
    }

	pointerMove(pointers){
		this.mouse_x = pointers[0].x;
		this.mouse_y = pointers[0].y;
		if(this.mouse_down){
            if(this.dragging){
                let ray = tools.renderer.getRay([this.mouse_x,this.mouse_y]);
                ray.name = "drag";
                tools.API.call("setPinTarget", ray, new Serializer()); 
                tools.API.call("applyPins", {}, new Serializer());
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
            let params = {name:"drag"} ;
            tools.API.call("deletePin", params, new Serializer()); 
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
    }

	keyUpListener(event){

    }
    
    vrInputSourcesUpdated(xr_input){
        let which_hand = 0 ;
        let GRIP = 1 ;
        let TRIGGER = 0 ;
        let pose_hand = -1 ;
        let grab_hand = -1 ;
        for (let input_source of xr_input) {
            if(input_source.grip_pose){
                if(input_source.buttons[GRIP].pressed  && grab_hand < 0){
                    grab_hand = which_hand;
                }
                if(input_source.buttons[TRIGGER].pressed && pose_hand < 0){
                    pose_hand = which_hand ;
                }
                /*
                for(let button of input_source.buttons){
                    which_button++;
                    if(button.pressed){
                        //console.log("button pressed " + which_button);
                        if(which_button == GRIP && grab_hand < 0){
                            grab_hand = which_hand;
                        }
                        if(which_button == TRIGGER && pose_hand < 0){
                            pose_hand = which_hand ;
                        }
                    }
                }*/
                

                for(let axis of input_source.axes){
                    this.axis_total += axis ;
                }
                
                
                
                //let grip_pose = frame.getPose(inputSource.gripSpace, tools.renderer.xr_ref_space).transform.matrix;
                this.hand_pose[which_hand] = input_source.grip_pose;
                
                

                if(grab_hand == which_hand){
                    if(!this.grab_pose){ // start of grab, fetch starting poses
                        this.grab_pose = mat4.create();
                        this.grab_pose.set(input_source.grip_pose);
                        this.grab_model_pose = mat4.create();
                        this.grab_model_pose.set(this.model_pose) ;
                        this.grab_axis_total = this.axis_total ;
                    }

                    let MP = mat4.create();
                    let inv = mat4.create();
                    mat4.invert(inv, this.grab_pose); // TODO cache at grab time
                    mat4.multiply(MP,inv, MP);
                    mat4.multiply(MP,input_source.grip_pose, MP);
                    let scale = Math.pow(1.05, (this.axis_total - this.grab_axis_total)*0.3);
                    mat4.scale(MP, MP,[scale,scale,scale]);
                    mat4.multiply(this.model_pose, MP, this.grab_model_pose);

                }

                if(pose_hand == which_hand){
                    let params = {};
                    
                    let inv = mat4.create();
                    mat4.invert(inv,this.model_pose);
                    let MP = mat4.create();
                    mat4.multiply(MP,inv, input_source.grip_pose);


                    //console.log(grip_pose);
                    let gpos = [
                        vec4.fromValues(0, 0, 0, 1.0)
                    ];

                    
                    let creating = false;
                    if(!this.pin){
                        this.pin = [];
                        creating = true ;
                    }
                    for(let g = 0; g < gpos.length;g++){

                        params.name = "vr_pose" + g;
                    
                        vec4.transformMat4(gpos[g], gpos[g], MP);// move global position into model space
                        //console.log(gpos[0] +", " + gpos[1] +", " + gpos[2]);
                        params.p = new Float32Array([gpos[g][0], gpos[g][1], gpos[g][2]]);
                        if(creating){
                            this.pin.push(params.name); // create pin from current point
                            this.initial_pose  = tools.API.call("createPin", params, new Serializer()).matrix; 
                            this.initial_grip = input_source.grip_pose ;
                            tools.API.call("setIKParams", {stiffness_strength:0.00005, iter:10, tolerance:0}, new Serializer());
                        }else{
                            let current_grip = input_source.grip_pose ;
                            let model_inv = mat4.create();
                            mat4.invert(model_inv,this.model_pose);
                            let MP = mat4.create();
                            let inv = mat4.create();
                            // get change in grip
                            mat4.invert(inv, this.initial_grip);
                            mat4.multiply(MP,current_grip, inv);

                            // make both relative to model pose by wrapping it on both sides
                            mat4.multiply(MP,model_inv,MP);
                            mat4.multiply(MP,MP,this.model_pose);

                            mat4.multiply(MP,MP,this.initial_pose);

                            params.o = MP;

                            tools.API.call("setPinTarget", params, new Serializer()); 
                            tools.API.call("applyPins", {}, new Serializer()); 
                        }
                    }

                }
                
            }
            which_hand++;
        }


        if(grab_hand < 0){// stopped grabbing, clear saved poses
            this.grab_pose = null;
            this.grab_model_pose = null;
        }
        if(pose_hand < 0 && this.pin){
            for(let name of this.pin){
                let params = {name:name} ;
                tools.API.call("deletePin", params, new Serializer()); 
            }
            this.pin = null ;
        }
    }
}