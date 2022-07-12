class AvatarMode extends ExecutionMode{
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

    mirror_dist = 2;

    head_pins = null
    hand_pins = [null, null];


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
        

        let mirror = mat4.create();
        let M = mat4.create();
        mat4.scale(mirror, mirror, vec3.fromValues(1,1,-1));
        mat4.translate(mirror, mirror,[0,0,2]);

        // Draw the hands
        for(let h = 0 ; h < 2; h++){
            if(!this.hand_pins[h]){
                tools.renderer.drawMesh("HAND", this.hand_pose[h]);

                mat4.multiply(M,mirror, this.hand_pose[h]);
                tools.renderer.drawMesh("HAND", M);

            }
        }

        // Draw the model
        tools.renderer.setMeshDoubleSided("MAIN", false);
        tools.renderer.drawMesh("MAIN", this.model_pose);

        //Draw the mirror
        tools.renderer.setMeshDoubleSided("MAIN", true);

        mat4.multiply(M,mirror, this.model_pose);
        tools.renderer.drawMesh("MAIN", M);

/*
        let mirror = mat4.create();
        mat4.scale(mirror, this.model_pose, vec3.fromValues(1,1,-1));
        mat4.translate(mirror, mirror,[0,0,2]);

        tools.renderer.drawMesh("MAIN", mirror);
*/
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
        //console.log(character);
    }

	keyUpListener(event){

    }
    
    vrInputSourcesUpdated(xr_input){

        if(this.head_pins == null){
            /*
            let head_transform = tools.API.call("getNodeTransform", {name:"Head"}, new Serializer()).transform ;
            let scale = 1.0/head_transform[0];//tools.renderer.mvMatrix[13]/head_transform[13]; // scale all model so model head is at player head 
            let MP = mat4.create();
            
            mat4.identity(MP);
            mat4.scale(MP, MP,[scale,scale,scale]);
            mat4.multiply(this.model_pose, head_transform, MP);
            mat4.multiply(this.model_pose, this.model_pose, tools.renderer.head_pose.transform.matrix);

            console.log("head:");
            console.log(head_transform);
            console.log("mv:");
            console.log(tools.renderer.mvMatrix);
            console.log("scale:" + scale);
            console.log(this.model_pose);
            
            
            console.log("head pose:");
            console.log(tools.renderer.head_pose);

            */
            this.head_pins = [];

        }

        // Move entire model to line up with head
        let current_camera_head_pose = tools.renderer.head_pose.transform.matrix ;
        console.log("camera head:" + JSON.stringify(current_camera_head_pose));
        let model_head_position = tools.API.call("getFirstPersonPosition", {}, new Serializer()).position; 
        model_head_position = vec4.fromValues(model_head_position[0], model_head_position[1], model_head_position[2], 1);
        console.log("model head:" + JSON.stringify(model_head_position));
        vec4.transformMat4(model_head_position, model_head_position, this.model_pose);
        console.log("transformed model head:" + JSON.stringify(model_head_position));
        let ch = vec4.create();
        // translate model pose so origin in camera pose lines up with head position
        vec4.transformMat4(ch, vec4.fromValues(0,0,0,1), current_camera_head_pose);
        
        let delta_head = [ch[0]-model_head_position[0], ch[1]-model_head_position[1], ch[2]-model_head_position[2]] ;
        console.log("change:" + JSON.stringify(delta_head));
        mat4.translate(this.model_pose, this.model_pose, delta_head);



        let which_hand = 0 ;
        let GRIP = 1 ;
        let TRIGGER = 0 ;
        let pose_hand = -1 ;
        let grab_hand = -1 ;

        let params = {};
        let model_inv = mat4.create();
        mat4.invert(model_inv,this.model_pose);

        




        for (let input_source of xr_input) {
            if(input_source.grip_pose){
                if(input_source.buttons[GRIP].pressed  && grab_hand < 0){
                    grab_hand = which_hand;
                }
                if(input_source.buttons[TRIGGER].pressed && pose_hand < 0 && !this.hand_pins[which_hand]){
                    pose_hand = which_hand ;
                }

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
                    mat4.invert(model_inv,this.model_pose);

                }

                
                let params = {};
                
                let MP = mat4.create();
                mat4.multiply(MP,model_inv, input_source.grip_pose);


                //console.log(grip_pose);
                /*let gpos = [
                    vec4.fromValues(0.0025, 0.0025, -0.0025, 1.0),
                    vec4.fromValues(-0.0025, 0.0025, -.0025, 1.0),
                    vec4.fromValues(0, -0.0025, -0.0025, 1.0),
                    vec4.fromValues(0, 0, 0.0025, 1.0)
                ];*/
                let gpos = [vec4.fromValues(0, 0, 0, 1.0)];

                
                let creating = false;
                // first grab with a hand that isn't the head grab
                if(!this.hand_pins[which_hand] && pose_hand == which_hand && this.head_pins.length > 0 && !this.head_posing){
                    this.hand_pins[which_hand] = [];
                    creating = true ;
                }
                if(this.hand_pins[which_hand] || creating){
                    for(let g = 0; g < gpos.length;g++){

                        params.name = "vr_hand_"+which_hand+"_pose_" + g;
                    
                        vec4.transformMat4(gpos[g], gpos[g], MP);// move global position into model space
                        //console.log(gpos[0] +", " + gpos[1] +", " + gpos[2]);
                        params.p = new Float32Array([gpos[g][0], gpos[g][1], gpos[g][2]]);
                        if(creating){
                            params.initial_matrix = tools.API.call("createRotationPin", params, new Serializer()).matrix;
                            tools.API.call("createPin", params, new Serializer());
                            params.initial_grip = mat4.create();
                            params.initial_grip.set(input_source.grip_pose);
                            this.hand_pins[which_hand].push(params); // create pin from current point    
                        }else{
                            //console.log(this.hand_pins[which_hand][g]);
                            let initial = this.hand_pins[which_hand][g].initial_matrix ;
                            let initial_grip = this.hand_pins[which_hand][g].initial_grip ;
                            let current_grip = input_source.grip_pose ;

                            let MP = mat4.create();
                            let inv = mat4.create();
                            // get change in grip
                            mat4.invert(inv, initial_grip);
                            mat4.multiply(MP,current_grip, inv);

                            // make both relative to model pose by wrapping it on both sides
                            mat4.multiply(MP,model_inv,MP);
                            mat4.multiply(MP,MP,this.model_pose);

                            mat4.multiply(MP,MP,initial);


                            params.target = MP;
                            tools.API.call("setRotationPinTarget", params, new Serializer()); 

                            tools.API.call("setPinTarget", params, new Serializer()); 

                            
                        }
                    }
                }

                
            }

            which_hand++;
        }

        
        let MP = mat4.create();
        mat4.multiply(MP,model_inv, tools.renderer.head_pose.transform.matrix);
        let creating = this.head_pins.length == 0 && pose_hand >=0; // the first time you press a trigger, pin the head
        
        /*let head_pos = [
            vec4.fromValues(0.02, 0, 0, 1.0),
            vec4.fromValues(-0.02, 0, 0, 1.0),
            vec4.fromValues(0, 0.02, 0, 1.0),
            vec4.fromValues(0, -0.02, 0, 1.0),
            vec4.fromValues(0, 0, 0.02,1.0),
            vec4.fromValues(0, 0,-0.02, 1.0)
        ];*/
        let head_pos = [vec4.fromValues(0, 0, 0, 1.0)];
        for(let g = 0; g < head_pos.length;g++){
            params.name = "vr_head_pose_" + g;
            vec4.transformMat4(head_pos[g], head_pos[g], MP);// move global position into model space
            params.p = new Float32Array([head_pos[g][0], head_pos[g][1], head_pos[g][2]]);
            //console.log(JSON.stringify(params));
            
            if(creating){
                params.initial_matrix = tools.API.call("createRotationPin", params, new Serializer()).matrix;
                tools.API.call("createPin", params, new Serializer()); 

                params.initial_pose = mat4.create();
                params.initial_pose.set(tools.renderer.head_pose.transform.matrix);
                this.head_pins.push(params); // create pin from current point
                this.head_posing = true;
            }else if(this.head_pins.length>0){
                
                let initial = this.head_pins[g].initial_matrix ;
                let initial_pose = this.head_pins[g].initial_pose ;
                let current_pose = tools.renderer.head_pose.transform.matrix ;
                //console.log("i, ip, cp, params");
                //console.log(initial);
                //console.log(initial_pose);
                //console.log(current_pose);
                let MP = mat4.create();
                let inv = mat4.create();
                
                // get change in grip
                mat4.invert(inv, initial_pose);
                mat4.multiply(MP,current_pose, inv);

                // make both relative to model pose by wrapping it on both sides
                mat4.multiply(MP,model_inv,MP);
                mat4.multiply(MP,MP,this.model_pose);

                mat4.multiply(MP,MP,initial);

                params.target = MP;
                tools.API.call("setRotationPinTarget", params, new Serializer()); 
                //tools.API.call("setPinTarget", params, new Serializer()); 
            }
            
        }


        if(grab_hand < 0){// stopped grabbing, clear saved poses
            this.grab_pose = null;
            this.grab_model_pose = null;
        }
        
        if(pose_hand < 0 && this.head_posing){
            this.head_posing = false;
        }
        
        tools.API.call("applyPins", {}, new Serializer()); 
    }
}