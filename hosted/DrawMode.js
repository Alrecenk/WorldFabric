class DrawMode extends ExecutionMode{
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

    line_start_time = 0 ;
    current_line = []; // [which_point][{t,x.y}];

    all_lines = [];
    redo_stack = [] ;

    degree = 3 ;


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
        let bones = tools.API.call("getBones", {mesh:"default_avatar"}, new Serializer()).bones ;
        tools.renderer.drawMesh("default_avatar", this.model_pose, bones);
        /*for(let dz = -1; dz <=1; dz++){
            for(let dy = -1; dy <=1; dy++){
                for(let dx = -1; dx <=1; dx++){
                    let M = mat4.create();
                    mat4.translate(M, this.model_pose,[dx*1.5,dy*1.5,dz*1.5]);
                    tools.renderer.drawMesh("default_avatar", M);
                }
            }
        }*/
        if(this.current_line.length > this.degree+1){
            /*
            for(let k = 1; k < this.current_line.length; k++){
                this.drawLine("#F00000",2, this.current_line[k][1], this.current_line[k][2], this.current_line[k-1][1], this.current_line[k-1][2]);
            }*/

            let smooth_input = [];
            for(let k = 0; k < this.current_line.length; k++){
                smooth_input.push(new Float32Array([this.current_line[k][0], this.current_line[k][1],this.current_line[k][2]]));
            }
            let smooth_line = tools.API.call("smoothLine", {input:smooth_input, degree:this.degree}, new Serializer()).result ;

            for(let k = 1; k < smooth_line.length; k++){
                this.drawLine("#000000",2, smooth_line[k][1], smooth_line[k][2], smooth_line[k-1][1], smooth_line[k-1][2]);
            }
        }

        for(let i=0;i<this.all_lines.length;i++){
            for(let k = 1; k < this.all_lines[i].length; k++){
                this.drawLine("#000000",2, this.all_lines[i][k][1], this.all_lines[i][k][2], this.all_lines[i][k-1][1], this.all_lines[i][k-1][2]);
            }
        }

        
    }

    // Draw a canvas line with a single call.
	drawLine(color, size, x1, y1, x2, y2){
        let context = tools.canvas.getContext("2d") ;
	    context.beginPath();
	    context.lineWidth = size;
	    context.strokeStyle = color;// set line color
	    context.moveTo(x1, y1);
	    context.lineTo(x2, y2);
	    context.stroke();
    }


    pointerDown(pointers){
        if(this.mouse_down){ // ignore second mouse click while dragging
			return ;
		}
		this.mouse_down_x = pointers[0].x;
		this.mouse_down_y = pointers[0].y;
		this.mouse_down = true ;
		this.mouse_button = pointers[0].button ;
        
       
        this.line_start_time= new Date().getTime() ;
        this.current_line = [[0, this.mouse_down_x,this.mouse_down_y]] ;
        this.dragging = true;
    }

	pointerMove(pointers){
		this.mouse_x = pointers[0].x;
		this.mouse_y = pointers[0].y;
		if(this.mouse_down){
            if(this.dragging){
                let time = new Date().getTime() - this.line_start_time;
                let last_x = this.current_line[this.current_line.length-1][1];
                let last_y = this.current_line[this.current_line.length-1][2];
                let dist = Math.sqrt((this.mouse_x-last_x)*(this.mouse_x-last_x) + (this.mouse_y-last_y)*(this.mouse_y-last_y));
                if(dist > 3){
                    this.current_line.push([time, this.mouse_x /*+ (Math.random()-0.5)*15*/,this.mouse_y /*+ (Math.random()-0.5)*15*/]);
                }
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
            let time = new Date().getTime() - this.line_start_time;
            //this.current_line.push([time, this.mouse_x ,this.mouse_y ]);
            let smooth_input = [];
            for(let k = 0; k < this.current_line.length; k++){
                smooth_input.push(new Float32Array([this.current_line[k][0], this.current_line[k][1],this.current_line[k][2]]));
            }
            let smooth_line = tools.API.call("smoothLine", {input:smooth_input, degree:this.degree}, new Serializer()).result ;
            this.all_lines.push(smooth_line);
            this.current_line = [] ;
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
        if(key_code == 38){// up
            this.degree++;
            if(this.degree > 10){
                this.degree = 10; 
            }
        }
        if(key_code == 40){//down
            this.degree--;
            if(this.degree < 1){
                this.degree = 1; 
            }
        }

        if(key_code == 37){// left
            this.redo_stack.push(this.all_lines.pop());
        }

        if(key_code == 39){// right
            this.all_lines.push(this.redo_stack.pop());
        }


        tools.buttons["degree_label"].text = "degree:"+this.degree ;

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
                        console.log("DrawMode saw button press!");
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