class WorldChatMode extends ExecutionMode{
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

    pin = null;

    mirror_dist = 2;

    head_pins = null
    hand_pins = [null, null];

    my_name = "player " + Math.floor(Math.random() * 1000000000) ;
    my_avatar = "default_avatar" ;
    avatars = ["default_avatar","alternate_avatar"/*, "avatar_4"*/];
    avatar_id = 0 ;
    my_mesh_id = -1;
    last_avatar_change = 0 ;

    frame = 0 ;
    last_time = new Date().getTime();
    framerate = 0 ;

    player_space = mat4.create();
    inv_player_space = mat4.create();
    player_position = vec3.create();
    player_angle = 0 ;
    player_speed = 0.02 ;
    player_spin_speed = 0.006;

    held_id = [-1,-1]; // objects currently held by each hand
    held_offset = [mat4.create(), mat4.create()];
    last_pose = [mat4.create(), mat4.create()] ;
    last_hand_time = [0,0] ;


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
        mat4.identity(this.player_space);
        mat4.identity(this.inv_player_space);
    }

    // Called when a mode is becoming inactive (prior to calling to enter on that mode)
    exit(next_mode){
    }

    // Called at regular intervals by the main app when the mode is active (30 or 60fps probably but not guarsnteed)
    timer(){
        
        let new_buffer_data = tools.API.call("getUpdatedBuffers", null, new Serializer());
        for(let id in new_buffer_data){
            tools.renderer.prepareBuffer(id, new_buffer_data[id]);
        }

        if(tools.sync_link.ready()){
            tools.API.call("runTimeline", {}, new Serializer());
            this.instances = tools.API.call("getMeshInstances", null, new Serializer());
            //console.log(instances);
            }
    }

    // Called when the app should be redrawn
    // Note: the elements to draw onto or with should be included in the tools on construction and saved for the duration of the mode
    drawFrame(frame_id){
        // Get any mesh updates pending in the module
        if(frame_id == 0){
            //Update FPS
            this.frame++;
            if(this.frame >= 90){
                var time = new Date().getTime();
                this.framerate = (this.frame*1000/ (time-this.last_time));
                this.last_time = time;
                this.frame = 0 ;
                //console.log("fps:" + this.framerate);
            }

            this.my_bones = tools.API.call("getBones", {mesh:this.my_avatar}, new Serializer()).bones ;
        }
        
        //TODO fnd a way to fix mesh files that don't export double sided property correctly
        tools.renderer.setMeshDoubleSided("default_world", false);
        tools.renderer.setMeshDoubleSided("default_avatar", false);
        tools.renderer.setMeshDoubleSided("alternate_avatar", false);
        //tools.renderer.setMeshDoubleSided("avatar_4", false);
        //tools.renderer.setMeshDoubleSided("dragon", true);

        let has_model = false;
        let m = mat4.create();
        if(this.instances){
            for( let k in this.instances){
                if(this.instances[k].owner != this.my_name){
                        mat4.multiply(m,this.inv_player_space, this.instances[k].pose);
                        if(this.instances[k].mesh.substr(0,5) === "shape"){ // shapes don't need to push bone data
                            tools.renderer.drawMesh(this.instances[k].mesh, m);
                        }else{
                            tools.renderer.drawMesh(this.instances[k].mesh, m , this.instances[k].bones);
                        }
                }else{
                    has_model = true;
                }
            }
        }

        // Draw your model with no delay
        if(has_model){
            tools.renderer.drawMesh(this.my_avatar, this.model_pose, this.my_bones);
        }
    }

    cycleAvatar(){
        var time = new Date().getTime() ;
        if(time - this.last_avatar_change > 1000){ // speed limit avatar switching
            this.avatar_id = (this.avatar_id+1)%this.avatars.length ;
            this.my_avatar = this.avatars[this.avatar_id] ;
            tools.API.call("setAvatar", {avatar:this.my_avatar}, new Serializer()); // sets the avatar in the wasm module for the IK 
            this.head_pins = null ; // trigger a remake of the Ik pins
            this.hand_pins = [null, null];
            this.last_avatar_change = time ;
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
                /*
                let ray = tools.renderer.getRay([this.mouse_x,this.mouse_y]);
                ray.name = "drag";
                tools.API.call("setPinTarget", ray, new Serializer()); 
                tools.API.call("applyPins", {}, new Serializer()); 
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
        let GRIP = 1 ; //constant for grip button on controllers
        let B_BUTTON = 5 ;
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


        // Check for change of avatar button pres before we do any avatar prosessing
        for (let input_source of xr_input) {
            if(!(input_source.grip_pose)){
                continue ; // not a tracked hand
            }
            if(input_source.handedness == "right" && input_source.buttons[B_BUTTON].pressed){
                this.cycleAvatar() ;
            }
            for( let b = 0 ; b < input_source.buttons.length; b++){
                if(input_source.buttons[b].pressed){
                    //console.log("Pressed " + b +" on " + input_source.handedness);
                }
            }
        }


        if(this.head_pins == null){
            // determine which input is which hand
            let left = -1, right = -1;
            for(let k=0;k<xr_input.length;k++){
                if(xr_input[k].handedness == "left"){
                    left = k ;
                }
                if(xr_input[k].handedness == "right"){
                    right = k ;
                }
            }

            let initial_matrices = tools.API.call("createVRMPins", {}, new Serializer()); 
            if(initial_matrices["error"]){
                console.log(initial_matrices["error"]);
                return;
            }
            this.head_pins = {name:"head", initial_matrix : initial_matrices["head"], initial_pose: initial_matrices["head"]};
            
            let left_grip = new Float32Array([  0,-1,0,0,
                                                1,0,0,0,
                                                0,0,1,0,
                                                0,0,0,1]);
            this.hand_pins[left] = {name:"left_hand", initial_matrix : initial_matrices["left_hand"], initial_grip:left_grip };
            
            let right_grip = new Float32Array([ 0,1,0,0,
                                                -1,0,0,0,
                                                0,0,1,0,
                                                0,0,0,1]);
            this.hand_pins[right] = {name:"right_hand", initial_matrix : initial_matrices["right_hand"], initial_grip: right_grip};    
            
            if(this.my_mesh_id < 0){
                tools.API.call("createMeshInstance", {owner:this.my_name}, new Serializer());
            }
        }

        let which_hand = 0 ;

        let params = {};
        let model_inv = mat4.create();
        mat4.invert(model_inv,this.model_pose);

        for (let input_source of xr_input) {
            if(!(input_source.grip_pose)){
                continue ; // not a tracked hand
            }

            if(input_source.handedness == "right"){
                if(Math.abs(input_source.axes[2]) > 0.08){
                    this.player_angle -= this.player_spin_speed * input_source.axes[2] ; // rotate on horizontal axis
                }
                if(Math.abs(input_source.axes[3]) > 0.08){
                    this.player_position[1] -= this.player_speed * input_source.axes[3] ; // vertical move on vertical axis
                }
            }

            let mx = Math.sin(this.player_angle);
            let mz = Math.cos(this.player_angle);
            // Normalize camera z with y removed
            let n = 1.0/Math.sqrt(ch[0]*ch[0] + ch[2]*ch[2]);
            ch[0]*=n;
            ch[2]*=n;

            if(input_source.handedness == "left"){ // horizontal movement on left stick
                if(Math.sqrt(input_source.axes[3]*input_source.axes[3] + input_source.axes[2]*input_source.axes[2]) > 0.08){
                    // move direction by player angle
                    let dx = input_source.axes[3] * mz - input_source.axes[2] * mx;
                    let dz = input_source.axes[3] * mx + input_source.axes[2] * mz ;
                    // rotated by head angle
                    this.player_position[0] +=  this.player_speed*(dx * ch[0] + dz * ch[2]) ;
                    this.player_position[2] +=  this.player_speed*(dx * ch[2] - dz * ch[0]) ;
                }
            }
            this.player_space = mat4.fromValues(  mz, 0, -mx, 0,
                                                    0, 1, 0, 0,
                                                    mx, 0, mz, 0,
                                                    this.player_position[0], this.player_position[1], this.player_position[2], 1);

            mat4.invert(this.inv_player_space, this.player_space);

            let params = {};
            
            let MP = mat4.create();
            mat4.multiply(MP,model_inv, input_source.grip_pose);

            let gpos = vec4.fromValues(0, 0, 0, 1.0);

            if(this.hand_pins[which_hand]/* || creating*/){
                vec4.transformMat4(gpos, gpos, MP);// move global position into model space
                //console.log(gpos[0] +", " + gpos[1] +", " + gpos[2]);
                params.p = new Float32Array([gpos[0], gpos[1], gpos[2]]);
                params.name = this.hand_pins[which_hand].name ;
                //console.log(this.hand_pins[which_hand]);
                let initial = this.hand_pins[which_hand].initial_matrix ;
                let initial_grip = this.hand_pins[which_hand].initial_grip ;
                let current_grip = input_source.grip_pose ;

                let MP2 = mat4.create();
                let inv = mat4.create();
                // get change in grip
                mat4.invert(inv, initial_grip);
                mat4.multiply(MP2,current_grip, inv);

                // makerelative to model pose
                mat4.multiply(MP2,model_inv,MP2);
                mat4.multiply(MP2,MP2,initial);

                params.o = MP2;
                tools.API.call("setPinTarget", params, new Serializer()); 

                // if gripping with this hand
                if(input_source.buttons[GRIP].pressed){
                    //console.log("grip button pressed");
                    let pose = mat4.create();
                    mat4.multiply(pose,this.player_space,current_grip);
                    

                    if(this.held_id[which_hand] == -1){
                        let inv_pose = mat4.create();
                        mat4.invert(inv_pose, pose);
                        //console.log("getting nearest solid:");
                        let gp = new Float32Array([pose[12], pose[13], pose[14]]);

                        let result = tools.API.call("getNearestSolid", {p:gp}, new Serializer()); 
                        //console.log(result);
                        if(result.distance < 0.3){
                            
                            this.held_id[which_hand] = result.id ;

                            mat4.multiply(this.held_offset[which_hand],inv_pose ,result.initial_pose);
                        }
                    }
                    if(this.held_id[which_hand] != -1){
                        //console.log("setting solid pose");
                        mat4.multiply(pose, pose,this.held_offset[which_hand]);
                        let time = new Date().getTime();
                        if(this.last_hand_time[which_hand] == 0 ){
                            tools.API.call("setSolidPose", {id:this.held_id[which_hand], pose: pose, freeze:1}, new Serializer()); 
                        }else{
                            //set velocity for physics but freeze so it doesn't integrate
                            tools.API.call("setSolidPose", {id:this.held_id[which_hand], pose: pose, freeze:1,
                                last_pose:this.last_pose[which_hand], dt:(time-this.last_hand_time[which_hand])*0.001 }, new Serializer()); 
                        }
                        this.last_pose[which_hand] = pose;
                        this.last_hand_time[which_hand] = time ;
                    }
                }else{
                    if(this.held_id[which_hand] != -1){ // on release
                        let pose = mat4.create();
                            mat4.multiply(pose,this.player_space,current_grip);
                            mat4.multiply(pose, pose,this.held_offset[which_hand]);
                        if(this.last_hand_time[which_hand] != 0 ){ // if had two frames
                            let time = new Date().getTime();
                            // apply exit velocity
                            tools.API.call("setSolidPose", {id:this.held_id[which_hand], pose: pose,
                                last_pose:this.last_pose[which_hand], dt:(time-this.last_hand_time[which_hand])*0.001 }, new Serializer()); 
                        }else{
                            tools.API.call("setSolidPose", {id:this.held_id[which_hand], pose: pose}, new Serializer());
                        }
                        
                    }


                    this.held_id[which_hand] = -1 ;
                    this.last_hand_time[which_hand] = 0 ;
                }

            }
            which_hand++;
        }

        
        let MP = mat4.create();
        mat4.multiply(MP,model_inv, tools.renderer.head_pose.transform.matrix);

        let head_pos = vec4.fromValues(0, 0, 0, 1.0);
        vec4.transformMat4(head_pos, head_pos, MP);// move global position into model space
        params.p = new Float32Array([head_pos[0], head_pos[1], head_pos[2]]);

        params.name = this.head_pins.name ;
        let initial = this.head_pins.initial_matrix ;
        let initial_pose = this.head_pins.initial_pose ;
        let current_pose = tools.renderer.head_pose.transform.matrix ;
        let MP2 = mat4.create();
        let inv = mat4.create();
        
        // get change in grip
        mat4.invert(inv, initial_pose);
        mat4.multiply(MP2,current_pose, inv);

        // makerelative to model pose
        mat4.multiply(MP2,model_inv,MP2);

        mat4.multiply(MP2,MP2,initial);

        delete params.p;
        params.o = MP2;
        tools.API.call("setPinTarget", params, new Serializer()); 
 
        tools.API.call("applyPins", {}, new Serializer()); 

        this.my_mesh_id =-1;
        //console.log(this.instances);
        for(let k in this.instances){
            if(this.instances[k].owner == this.my_name){
                this.my_mesh_id = parseInt(k); // k is a string
                break ;
            }
        }
        if(this.my_mesh_id >= 0){
            //console.log("Found my owned model: " + id);
            mat4.multiply(MP,this.player_space, this.model_pose);
            tools.API.call("setMeshInstance", {id:this.my_mesh_id, pose:MP}, new Serializer());
            
        }
    }
}