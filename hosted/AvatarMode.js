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
        tools.renderer.setMeshDoubleSided("default_avatar", false);
        let bones = tools.API.call("getBones", {mesh:"default_avatar"}, new Serializer()).bones ;
        tools.renderer.drawMesh("default_avatar", this.model_pose, bones);

        //Draw the mirror
        tools.renderer.setMeshDoubleSided("default_avatar", true);
        mat4.multiply(M,mirror, this.model_pose);
        tools.renderer.drawMesh("default_avatar", M, bones);

/*
        let mirror = mat4.create();
        mat4.scale(mirror, this.model_pose, vec3.fromValues(1,1,-1));
        mat4.translate(mirror, mirror,[0,0,2]);

        tools.renderer.drawMesh("default_avatar", mirror);
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

        //console.log("model pose start:" + JSON.stringify(this.model_pose));

        // Move entire model to line up with head
        let current_camera_head_pose = tools.renderer.head_pose.transform.matrix ;
        //console.log("camera:" + JSON.stringify(current_camera_head_pose ));
        let model_head_position = tools.API.call("getFirstPersonPosition", {}, new Serializer()).position; 
        model_head_position = vec4.fromValues(model_head_position[0], model_head_position[1], model_head_position[2], 1);
        //console.log("model_head_position:" + JSON.stringify(model_head_position));
        vec4.transformMat4(model_head_position, model_head_position, this.model_pose);
        let ch = vec4.create();
        // translate model pose so origin in camera pose lines up with head position
        vec4.transformMat4(ch, vec4.fromValues(0,0,0,1), current_camera_head_pose);
        let delta_head = [ch[0]-model_head_position[0], ch[1]-model_head_position[1], ch[2]-model_head_position[2]] ;
        //console.log("delta_head:" + JSON.stringify(delta_head));
        
        //mat4.translate(this.model_pose, this.model_pose, delta_head);
        let t = mat4.create();
        mat4.identity(t);
        mat4.translate(t, t, delta_head);
        mat4.multiply(this.model_pose, t, this.model_pose);

        //console.log("model pose mid:" + JSON.stringify(this.model_pose));
        
        // get z from camera
        vec4.transformMat4(ch, vec4.fromValues(0,0,1,0), current_camera_head_pose);
        //console.log("ch:" + JSON.stringify(ch));

        let mh = vec4.create();
        // get Z from model
        vec4.transformMat4(mh, vec4.fromValues(0,0,1,0), this.model_pose);
        //console.log("mh:" + JSON.stringify(mh));
        //normalized dot product to find angle between z axes (ignoring y)
        let angle = Math.acos((ch[0]*mh[0]+ch[2]*mh[2])/ Math.sqrt((ch[0]*ch[0]+ch[2]*ch[2]) * (mh[0]*mh[0]+mh[2]*mh[2]))) ;
        angle = Math.min(angle,0.1);
        if(ch[0]*mh[2] - ch[2]*mh[0] < 0){ // relevant component of cross to find direction to rotate
            angle*=-1;
        }
        //console.log("angle:" + angle);
        //let mt = [this.model_pose[12], this.model_pose[13], this.model_pose[14]];
        mat4.rotate(this.model_pose, this.model_pose, angle, [0,1,0]);
        //console.log("model pose end:" + JSON.stringify(this.model_pose));

        if(this.head_pins == null){
            
            //console.log("xr_input:" + JSON.stringify(xr_input));
            
            this.head_pins = [];


            let left = -1, right = -1;
            for(let k=0;k<xr_input.length;k++){
                if(xr_input[k].handedness == "left"){
                    left = k ;
                }
                if(xr_input[k].handedness == "right"){
                    right = k ;
                }
                console.log(xr_input[k]);
            }
            
            // If handedness not fully defined, try your best.
            if(left == -1 && right == -1){
                left = 0 ;
                right = 1 ;
            }else if(left == -1){
                left = 1-right;
            }else if(right == -1){
                right = 1-left;
            }


            
            let initial_matrices = tools.API.call("createVRMPins", {}, new Serializer()); 
            this.head_pins.push({name:"head", initial_matrix : initial_matrices["head"], initial_pose: initial_matrices["head"]});
            this.hand_pins[left] = [];
            let left_grip = new Float32Array([  0,-1,0,0,
                                                1,0,0,0,
                                                0,0,1,0,
                                                0,0,0,1]);
            this.hand_pins[left].push({name:"left_hand", initial_matrix : initial_matrices["left_hand"], initial_grip:left_grip });
            this.hand_pins[right] = [];
            let right_grip = new Float32Array([ 0,1,0,0,
                                                -1,0,0,0,
                                                0,0,1,0,
                                                0,0,0,1]);
            this.hand_pins[right].push({name:"right_hand", initial_matrix : initial_matrices["right_hand"], initial_grip: right_grip});
            
            
            
            console.log(this.hand_pins);
            
        }



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

                        
                    
                        vec4.transformMat4(gpos[g], gpos[g], MP);// move global position into model space
                        //console.log(gpos[0] +", " + gpos[1] +", " + gpos[2]);
                        params.p = new Float32Array([gpos[g][0], gpos[g][1], gpos[g][2]]);
                        if(creating){
                            params.name = "vr_hand_"+which_hand+"_pose_" + g;
                            params.initial_matrix = tools.API.call("createPin", params, new Serializer()).matrix;
                            params.initial_grip = mat4.create();
                            params.initial_grip.set(input_source.grip_pose);
                            this.hand_pins[which_hand].push(params); // create pin from current point    
                            console.log("hand " + which_hand + " : " + JSON.stringify(params));
                        }else{
                            params.name = this.hand_pins[which_hand][g].name ;
                            //console.log(this.hand_pins[which_hand][g]);
                            let initial = this.hand_pins[which_hand][g].initial_matrix ;
                            let initial_grip = this.hand_pins[which_hand][g].initial_grip ;
                            let current_grip = input_source.grip_pose ;

                            let MP = mat4.create();
                            let inv = mat4.create();
                            // get change in grip
                            mat4.invert(inv, initial_grip);
                            mat4.multiply(MP,current_grip, inv);

                            // makerelative to model pose
                            mat4.multiply(MP,model_inv,MP);

                            mat4.multiply(MP,MP,initial);


                            params.o = MP;
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
        
        let head_pos = [vec4.fromValues(0, 0, 0, 1.0)];
        for(let g = 0; g < head_pos.length;g++){
            
            vec4.transformMat4(head_pos[g], head_pos[g], MP);// move global position into model space
            params.p = new Float32Array([head_pos[g][0], head_pos[g][1], head_pos[g][2]]);
            
            
            if(creating){
                params.name = "vr_head_pose_" + g;
                console.log(JSON.stringify(params));
                params.initial_matrix = tools.API.call("createPin", params, new Serializer()).matrix;

                params.initial_pose = mat4.create();
                params.initial_pose.set(tools.renderer.head_pose.transform.matrix);
                this.head_pins.push(params); // create pin from current point
                this.head_posing = true;
            }else if(this.head_pins.length>0){
                params.name = this.head_pins[g].name ;
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

                // makerelative to model pose
                mat4.multiply(MP,model_inv,MP);

                mat4.multiply(MP,MP,initial);

                params.o = MP;
                delete params.p ;
                tools.API.call("setPinTarget", params, new Serializer()); 
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