class HoloRenderer{

    gl; // WebGl instance
    shaderProgram; 

    camera_pos;
    mvMatrix = mat4.create(); // Model view matrix
    pMatrix = mat4.create();  // Perspective matrix
    bgColor = [220,220,220];

    buffers = {} ; // current active mesh buffers maps id to object with position,color, and normal
 
    start_camera ; // A clone of the camera mvMatrix made when a camera operation starts
    action_focus = [0,0,0]; // the focal point of the current camera operation
    start_pointer ; // Mouse or touch pointers saved whena camera operation starts

    maxPanSpeed = 5;
    xOrbitSpeed=-0.004;
    yOrbitSpeed=0.007;
    rotate_speed=0.01;

    frame = 0 ;
    last_time = new Date().getTime();
    framerate = 0 ;

    xr_input = [];
    has_new_xr_input = false;

    buffer_lookup = {};

    hold_ui_context = false; 

    static texture_width = 1024 ; // fixed texture width for 1D data because there's no1d texture support

    
    // Performs the set-up for openGL canvas and shaders on construction
    constructor(webgl_canvas_id, ui_canvas_id , fragment_shader_id, vertex_shader_id, space_underneath_app){
        var canvas = document.getElementById(webgl_canvas_id);
        var ui_canvas = document.getElementById(ui_canvas_id);
        canvas.width = document.body.clientWidth; 
        canvas.height = document.body.clientHeight-space_underneath_app; 
        ui_canvas.width = document.body.clientWidth; 
        ui_canvas.height = document.body.clientHeight-space_underneath_app; 
        
        this.initGL(canvas);
        this.initShaderProgram(fragment_shader_id, vertex_shader_id);
        let gl = this.gl ;
        gl.clearColor(0.0, 0.0, 0.0, 1.0);
        gl.enable(gl.DEPTH_TEST);
        gl.disable(gl.CULL_FACE);
        //gl.cullFace(gl.BACK);
        console.log(gl.getParameter(gl.VERSION));
        console.log(gl.getParameter(gl.SHADING_LANGUAGE_VERSION));
        console.log(gl.getParameter(gl.VENDOR));
        gl.getExtension('OES_texture_float');
        

        mat4.perspective(this.pMatrix, 45, gl.viewportWidth / gl.viewportHeight, 0.1, 3000.0);
        this.setDefaultView();


        requestAnimationFrame(HoloRenderer.onFrame); // Timer at 60 hertz.
    }

    setDefaultView(){
        this.camera_pos = [1,1,1];
        mat4.lookAt(this.mvMatrix, this.camera_pos, [0,0,0], [0,1,0] );
    }

    // Initialize webGL on a canvas
    initGL(canvas){
        try {
            this.gl = canvas.getContext("webgl2",{ xrCompatible: true });
            this.gl.viewportWidth = canvas.width;
            this.gl.viewportHeight = canvas.height;
        } catch (e) {
        }
        if (!this.gl) {
            alert("Could not initialise WebGL, sorry :-(");
        }
    }

    // Fetches and compiles a shader script from a page element.
    // Used by initShaderProgram which is probably what you want to call.
    getShader(gl, id) {
        var shaderScript = document.getElementById(id);
        if (!shaderScript) {
            return null;
        }
        var str = "";
        var k = shaderScript.firstChild;
        while (k) {
            if (k.nodeType == 3) {
                str += k.textContent;
            }
            k = k.nextSibling;
        }
        var shader;
        if (shaderScript.type == "x-shader/x-fragment") {
            shader = gl.createShader(gl.FRAGMENT_SHADER);
        } else if (shaderScript.type == "x-shader/x-vertex") {
            shader = gl.createShader(gl.VERTEX_SHADER);
        } else {
            return null;
        }
        gl.shaderSource(shader, str);
        gl.compileShader(shader);
        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            console.log(gl.getShaderInfoLog(shader));
            return null;
        }
        return shader;
    }

    // Initialize shaders defined in html script elements into shaderProgram for webGL
    initShaderProgram(fragment_shader_id, vertex_shader_id) {
        let gl = this.gl ;
        var fragmentShader = this.getShader(gl, fragment_shader_id);
        var vertexShader = this.getShader(gl, vertex_shader_id);

        this.shaderProgram = gl.createProgram();
        gl.attachShader(this.shaderProgram, vertexShader);
        gl.attachShader(this.shaderProgram, fragmentShader);
        gl.linkProgram(this.shaderProgram);

        if (!gl.getProgramParameter(this.shaderProgram, gl.LINK_STATUS)) {
            console.error("Could not initialize shaders!");
        }

        gl.useProgram(this.shaderProgram);
        this.shaderProgram.input_vertex_position = gl.getAttribLocation(this.shaderProgram, "input_vertex_position");
        gl.enableVertexAttribArray(this.shaderProgram.input_vertex_position);

        this.shaderProgram.p_matrix= gl.getUniformLocation(this.shaderProgram, "p_matrix");
        this.shaderProgram.mv_matrix = gl.getUniformLocation(this.shaderProgram, "mv_matrix");
        this.shaderProgram.view_position = gl.getUniformLocation(this.shaderProgram, "view_position");


        this.shaderProgram.floats = gl.getUniformLocation(this.shaderProgram, "floats");
        this.shaderProgram.ints = gl.getUniformLocation(this.shaderProgram, "ints");
        this.shaderProgram.bytes = gl.getUniformLocation(this.shaderProgram, "bytes");
        
    }

    static onFrame(){

        var r = tools.renderer ;
        //Update FPS label
        r.frame++;
        if(tools.buttons && tools.buttons["fps_label"]){
            if(r.frame >= 30){
                var time = new Date().getTime();
                r.framerate = (r.frame*1000/ (time-r.last_time));
                r.last_time = time;
                r.frame = 0 ;
                if(tools && tools.buttons && tools.buttons["fps_label"]){ // TODO not really the responsibility of the general rendering
                    tools.buttons["fps_label"].text = "FPS: " + Math.round(r.framerate);
                }
                
                
            }

            // Rotate a character in the fps label as an indicator the app is not frozen
            let c = tools.buttons["fps_label"].text.charAt(4) ;
            if(c == '|'){
                c = '-' ;
            }else{
                c = '|' ;
            }
            tools.buttons["fps_label"].text = tools.buttons["fps_label"].text.substring(0, 4) + c + tools.buttons["fps_label"].text.substring(5);
        }
        
        
        var context = tools.canvas.getContext("2d");
        if(!r.hold_ui_context){
            context.clearRect(0, 0, context.canvas.width, context.canvas.height);
        }
        // Draw any buttons currently on the interface.
        for (let button_name in tools.buttons) {
            if (tools.buttons.hasOwnProperty(button_name)) {
                tools.buttons[button_name].draw(context);
            }
        }
        

        if(tools.current_mode){
            let gl = r.gl ;
            gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);
            gl.clearColor(r.bgColor[0]/255.0, r.bgColor[1]/255.0, r.bgColor[2]/255.0, 1.0);
            gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
            gl.uniformMatrix4fv(r.shaderProgram.p_matrix, false, r.pMatrix);
            tools.current_mode.drawFrame(0);
            gl.finish();
        }
        if(!r.xr_session){ // Stop drawing regular frames when VR sessions is entered
            requestAnimationFrame(HoloRenderer.onFrame);
        }
    }


    // Returns the point at which the ray through the centero fthe screen intersects the ground
    getCameraFocus(ground_height){
        // Get a ray through the center of the screen
        let ray = this.getRay([gl.viewportWidth*0.5, gl.viewportHeight*0.5]) ;
        //intersect it with the ground
        return HoloRenderer.getGroundIntersect(ray, ground_height) ;
    }

    static getGroundIntersect(ray, ground_height){
        let t = (ground_height-ray.p[1])/ray.v[1];
        return [ray.p[0] + ray.v[0]*t, ground_height, ray.p[2] + ray.v[2]*t];
    }

    startRotate(focus, pointer){
        let ray = this.getRay([this.gl.viewportWidth*0.5, this.gl.viewportHeight*0.5]) ;
        if(vec3.distance(this.action_focus, focus) > 1){
            this.action_focus = focus ;
        }

        this.start_position = [ray.p[0], ray.p[1], ray.p[2]] ;
        this.start_pointer = pointer ;
        this.start_camera = mat4.clone(this.mvMatrix);
    }

    continueRotate(pointer){
        var pointer_delta = [pointer.x - this.start_pointer.x, pointer.y - this.start_pointer.y] ;
        var movement = Math.sqrt(pointer_delta[0]*pointer_delta[0]+pointer_delta[1]*pointer_delta[1]);
        if(movement < 1){
            return ;
        }
        let x_axis = [this.start_camera[0], this.start_camera[4], this.start_camera[8]];
        let y_axis = [this.start_camera[1], this.start_camera[5], this.start_camera[9]];
        //axis is 90 degree rotated from mouse move in camera space
        let r = [-pointer_delta[1]*x_axis[0] - pointer_delta[0] * y_axis[0],
                        -pointer_delta[1]*x_axis[1] - pointer_delta[0] * y_axis[1],
                        -pointer_delta[1]*x_axis[2] - pointer_delta[0] * y_axis[2],
                    ];
        let len = Math.sqrt(r[0]*r[0]+r[1]*r[1]+r[2]*r[2]);
        r[0]/=len;
        r[1]/=len;
        r[2]/=len;
        // rotation speed is arbitrarily scaled pointer move distance
        let dtheta = this.rotate_speed * movement;
        let M = mat4.create() ;          
        mat4.rotate(M, M, dtheta, r) ;
        //rotate the position relative to the focus
        let rel_pos = [this.start_position[0]-this.action_focus[0], this.start_position[1]-this.action_focus[1], this.start_position[2]-this.action_focus[2]]; 
        vec3.transformMat4(rel_pos, rel_pos, M) ;
        this.camera_pos = [rel_pos[0] + this.action_focus[0], rel_pos[1] + this.action_focus[1], rel_pos[2] + this.action_focus[2]] ;
        //rotate the up axis, so there's no preferred up direction
        vec3.transformMat4(y_axis, y_axis, M) ;
        mat4.lookAt(this.mvMatrix, this.camera_pos, this.action_focus, y_axis );

        this.start_position = [this.camera_pos[0], this.camera_pos[1], this.camera_pos[2]] ;
        this.start_pointer = pointer ;
        this.start_camera = mat4.clone(this.mvMatrix);

    }

    moveCamera(move){
        mat4.translate(this.mvMatrix, this.mvMatrix, [-1*move[0], -1*move[1], -1*move[2]]);
        this.camera_pos[0] += move[0];
        this.camera_pos[1] += move[1];
        this.camera_pos[2] += move[2];
    }

    setZoom(zoom, ground_height){
        if(!ground_height){
            ground_height = this.action_focus[1] ;
        }
        // Send a ray through the center of the screen
        let ray = this.getRay([this.gl.viewportWidth*0.5, this.gl.viewportHeight*0.5]) ;
        //intersect it with the ground then back up by zoom
        let t = (ground_height-ray.p[1])/ray.v[1] - zoom ;
        let position = [ray.p[0] + ray.v[0]*t, ray.p[1] + ray.v[1]*t, ray.p[2] + ray.v[2]*t] ;
        this.moveCamera([position[0]-ray.p[0], position[1]-ray.p[1], position[2]-ray.p[2]]) ;
    }

    getZoom(ground_height){
       // Send a ray through the center of the screen
       let ray = this.getRay([this.gl.viewportWidth*0.5, this.gl.viewportHeight*0.5]) ;
       //intersect it with the ground
       let t = (ground_height-ray.p[1])/ray.v[1] ;
       return t ;
    }    


    // Given a 3D point return the point on the canvas it would be on
    projectToScreen(point){
        let p = vec4.fromValues(point[0], point[1], point[2], 1);
        let np = [0,0,0] ;
        vec4.transformMat4(np, p, this.mvMatrix);
        vec4.transformMat4(np, np, this.pMatrix);
        if(np[3] < 0){ // behind camera
            return null;
        }else{
            np[0]/=np[3];
            np[1]/=np[3];
            np[2]/=np[3];
            return [(np[0]+1) * gl.viewportWidth * 0.5, (-np[1]+1) * gl.viewportHeight * 0.5];
        }
    }

    // Takes a pixel position on the screen and creates a 3D ray from the viewpoint toward that pixel in voxel space.
    // returns an object with p and v both float32 arrays with 3 elements (compatible with CPP API raytrace)
    getRay(screen_pos){
        return HoloRenderer.getPixelRay(this.camera_pos, screen_pos, this.pMatrix, this.mvMatrix, this.gl);
    }
    static getPixelRay(camera_pos, screen_pos, pMatrix, mvMatrix, gl){
        // recreate the transformation used by the viewing pipleline
        var M  = mat4.create();
        mat4.multiply(M, pMatrix, mvMatrix ) ;
        mat4.invert(M,M); // invert it

        let pos = new Float32Array([camera_pos[0], camera_pos[1], camera_pos[2]]);

        // Get the pixel vector in screen space using viewport parameters.
        var p = vec4.fromValues(2*screen_pos[0]/gl.viewportWidth-1, -1*(2*screen_pos[1]/gl.viewportHeight-1), 1, 1);
        vec4.transformMat4(p, p, M);
        let v = new Float32Array([p[0]/p[3]-pos[0], p[1]/p[3]-pos[1], p[2]/p[3]-pos[2]]);
        
        var n = Math.sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
        v[0]/=n;
        v[1]/=n; // Normalize the direction.
        v[2]/=n;

        return {p:pos,v:v};
    }

    clearBuffers(){
        this.buffers = {} ;
        //TODO delete textures?
    }

    // Binds a webGl buffer to the buffer data provided and puts it in buffers[id]
    prepareBuffer(id, buffer_data){
        let gl = tools.renderer.gl ;
        if(!(id in this.buffers)){ // New buffer
            this.buffers[id] = {};
            this.buffers[id].position = gl.createBuffer();
            this.buffers[id].floats = gl.createTexture();
            this.buffers[id].ints = gl.createTexture();
            this.buffers[id].bytes = gl.createTexture();
        }
        let num_vertices = buffer_data.vertices ;
        if(num_vertices == 0){
            this.buffers[id].ready = false;
            return ;
        }
        if(buffer_data.position){
            gl.bindBuffer(gl.ARRAY_BUFFER, this.buffers[id].position );
            gl.bufferData(gl.ARRAY_BUFFER, buffer_data.position, gl.STATIC_DRAW);
            this.buffers[id].position.itemSize = 3;
            this.buffers[id].position.numItems = num_vertices;
        }

        if(buffer_data.floats){
            //set up texture on buffer
            gl.bindTexture(gl.TEXTURE_2D, this.buffers[id].floats);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);	
            //generate texture from raw Float32Array in buffer data	
            var height = Math.ceil(buffer_data.floats.length/(1.0*HoloRenderer.texture_width));

            let padded = new Float32Array(HoloRenderer.texture_width*height);
            for(let k = 0 ; k < buffer_data.floats.length;k++){
                padded[k] = buffer_data.floats[k];
            }

            gl.texImage2D(gl.TEXTURE_2D, 0, gl.R32F, HoloRenderer.texture_width, height, 0, gl.RED, gl.FLOAT, padded);
            // Bind the data texture to a slot (fixed at 2 for this) and set the shader uniform to point at the same slot
            //these 3 lines are laos what you need to do to draw, but doing it at load time "warms it up" for later maybe
            gl.activeTexture(gl.TEXTURE0 + 2);
		    gl.bindTexture(gl.TEXTURE_2D, this.buffers[id].floats);
		    gl.uniform1i(this.shaderProgram.floats, 2);
        }

        if(buffer_data.ints){
            //set up texture on buffer
            gl.bindTexture(gl.TEXTURE_2D, this.buffers[id].ints);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);	
            //generate texture from raw Float32Array in buffer data	
            var height = Math.ceil(buffer_data.ints.length/(1.0*HoloRenderer.texture_width));	

            let padded = new Int32Array(HoloRenderer.texture_width*height);
            for(let k = 0 ; k < buffer_data.ints.length;k++){
                padded[k] = buffer_data.ints[k];
            }

            gl.texImage2D(gl.TEXTURE_2D, 0, gl.R32I, HoloRenderer.texture_width, height, 0, gl.RED_INTEGER, gl.INT, padded);
            // Bind the data texture to a slot (fixed at 3 for this) and set the shader uniform to point at the same slot
            //these 3 lines are laos what you need to do to draw, but doing it at load time "warms it up" for later maybe
            gl.activeTexture(gl.TEXTURE0 + 3);
		    gl.bindTexture(gl.TEXTURE_2D, this.buffers[id].ints);
		    gl.uniform1i(this.shaderProgram.ints, 3);
        }

        if(buffer_data.bytes){
            //set up texture on buffer
            gl.bindTexture(gl.TEXTURE_2D, this.buffers[id].bytes);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);	
            //generate texture from raw Float32Array in buffer data	
            var height = Math.ceil(buffer_data.bytes.length/(1.0*HoloRenderer.texture_width));	
            let padded = new Uint8Array(HoloRenderer.texture_width*height);
            for(let k = 0 ; k < buffer_data.bytes.length;k++){
                padded[k] = buffer_data.bytes[k];
            }
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.R8UI, HoloRenderer.texture_width, height, 0, gl.RED_INTEGER, gl.UNSIGNED_BYTE, padded);
            // Bind the data texture to a slot (fixed at 3 for this) and set the shader uniform to point at the same slot
            //these 3 lines are laos what you need to do to draw, but doing it at load time "warms it up" for later maybe
            gl.activeTexture(gl.TEXTURE0 + 4);
		    gl.bindTexture(gl.TEXTURE_2D, this.buffers[id].bytes);
		    gl.uniform1i(this.shaderProgram.bytes, 4);
        }


        this.buffers[id].ready = true;
    }

    drawHologramBuffer(buffer){
        //console.log(buffer);
        let gl = this.gl ;
        if(buffer.ready){
            let position_buffer = buffer.position;
           
            gl.bindBuffer(gl.ARRAY_BUFFER, buffer.position);
            gl.vertexAttribPointer(this.shaderProgram.input_vertex_position, buffer.position.itemSize, gl.FLOAT, false, 0, 0);
            

            gl.activeTexture(gl.TEXTURE0 + 2 );
            gl.bindTexture(gl.TEXTURE_2D, buffer.floats);
            gl.uniform1i(this.shaderProgram.floats, 2 );

            gl.activeTexture(gl.TEXTURE0 + 3);
            gl.bindTexture(gl.TEXTURE_2D, buffer.ints);
            gl.uniform1i(this.shaderProgram.ints, 3);

            gl.activeTexture(gl.TEXTURE0 + 4);
            gl.bindTexture(gl.TEXTURE_2D, buffer.bytes);
            gl.uniform1i(this.shaderProgram.bytes, 4);


            gl.drawArrays(gl.TRIANGLES, 0, position_buffer.numItems);
        }
    }

    drawHologram(holo_name, transform = null){
        if(!transform){
            transform = mat4.create();
            mat4.identity(transform);
        }
        let M = mat4.create();
        mat4.multiply(M,this.mvMatrix, transform);
        this.gl.uniformMatrix4fv(this.shaderProgram.mv_matrix, false, M);
        
        // extract view position from MV matrix (can't use camera_pos as it doesn't move with VR head poses)
        let mvi = mat4.create();
        mat4.invert(mvi,M);
        let view_pos = new Float32Array([mvi[12], mvi[13], mvi[14]]);
        this.gl.uniform3fv(this.shaderProgram.view_position, view_pos);

        if(holo_name in this.buffers){
            this.drawHologramBuffer(this.buffers[holo_name]);
        }
    }

    removeHologram(holo_name){
        delete this.buffers[holo_name] ;
    }


    static deserializeInt(array_buffer, ptr) {
        let bytes = new Uint8Array(array_buffer, ptr, 4);
        return bytes[0] + (bytes[1]<<8) +(bytes[2] <<16) + (bytes[3] << 24) ;
    }

    static deserializeShort(array_buffer, ptr) {
        let bytes = new Uint8Array(array_buffer, ptr, 2);
        return bytes[0] + (bytes[1]<<8) ;
    }

    static deserializeByte(array_buffer, ptr) {
        return new Int8Array(array_buffer, ptr, 1)[0];
    }

    static deserializeFloat(array_buffer, ptr) {
        return new Float32Array(array_buffer.slice(ptr, ptr + 4), 0, 1)[0];
    }

    static deserializeDouble(array_buffer, ptr) {
        return new Float64Array(array_buffer.slice(ptr, ptr + 8), 0, 1)[0];
    }

    static deserializeIntArray(array_buffer, ptr, length) {
        return new Int32Array(array_buffer.slice(ptr, ptr + 4 * length), 0, length);
    }

    static deserializeShortArray(array_buffer, ptr, length) {
        return new Int16Array(array_buffer.slice(ptr, ptr + 2 * length), 0, length);
    }

    static deserializeByteArray(array_buffer, ptr, length) {
        return new Uint8Array(array_buffer, ptr, length);
    }

    static deserializeFloatArray(array_buffer, ptr, length) {
        return new Float32Array(array_buffer.slice(ptr, ptr + 4 * length), 0, length);
    }

    static deserializeDoubleArray(array_buffer, ptr, length) {
        return new Float64Array(array_buffer.slice(ptr, ptr + 8 * length), 0, length);
    }

    //data is an int8array of the raw bytes of a file exported with the downloadHologram function in the c++ API
    addHologramFile(holo_name, data, r = 10.0){
        let file_buffer = data.buffer ;

        //let holo_size = deserializeInt(file_buffer, 0) ;
        let num_floats = HoloRenderer.deserializeInt(file_buffer, 4) ;
        let num_ints = HoloRenderer.deserializeInt(file_buffer, 8) ;
        let num_bytes = HoloRenderer.deserializeInt(file_buffer, 12) ;
        console.log("floats:" + num_floats +" ints: " + num_ints +" bytes:" + num_bytes);
        let buffer = {};
        
        buffer.floats = HoloRenderer.deserializeFloatArray(file_buffer, 16,num_floats);
        buffer.ints = HoloRenderer.deserializeIntArray(file_buffer, 16 + num_floats*4, num_ints);
        buffer.bytes = HoloRenderer.deserializeByteArray(file_buffer, 16 + num_floats*4 + num_ints*4, num_bytes);

        // triangles that form an octahedron around 0,0,0 of radius r as a flattened array
        /*
        buffer.position = new Float32Array(8*3*3);

        let v = [
            [0, 0 - r, 0], // 0
            [0 - r, 0, 0], // 1
            [0, 0, 0 - r], // 2
            [0, 0 + r, 0], // 3
            [0 + r, 0, 0], // 4
            [0, 0, 0 + r] // 5
        ]; 

        let f = [
            [0, 1, 2],
            [0, 2, 4],
            [1, 0, 5],
            [5, 0, 4],
            [1, 5, 3],
            [5, 4, 3],
            [2, 1, 3],
            [4, 2, 3]
        ];

        for(let k=0;k<f.length;k++){
            let j = k*9;
            let A = v[f[k][0]];
            buffer.position[j] = A[0];
            buffer.position[j+1] = A[1];
            buffer.position[j+2] = A[2];
            let B = v[f[k][1]];
            buffer.position[j+3] = B[0];
            buffer.position[j+4] = B[1];
            buffer.position[j+5] = B[2];
            let C = v[f[k][2]];
            buffer.position[j+6] = C[0];
            buffer.position[j+7] = C[1];
            buffer.position[j+8] = C[2];
        }
        console.log(buffer.position);
        */
        buffer.position = new Float32Array([0, -r, 0, -r, 0, 0, 0, 0, -r, 0, -r, 0, 0, 0, -r, r, 0, 0, -r, 
            0, 0, 0, -r, 0, 0, 0, r, 0, 0, r, 0, -r, 0, r, 0, 0, -r, 0, 0, 0, 0, r, 0, r, 0, 0, 0, r, r,
             0, 0, 0, r, 0, 0, 0, -r, -r, 0, 0, 0, r, 0, r, 0, 0, 0, 0, -r, 0, r, 0]);
        buffer.vertices = 24 ;

        this.prepareBuffer(holo_name, buffer);


    }

    startXRSession(){
        if(tools.renderer.xr_session == null && !this.started_vr){
            this.started_vr = true ;
            navigator.xr.requestSession('immersive-vr',{optionalFeatures: ['high-fixed-foveation-level', 'high-refresh-rate']})
                .then(tools.renderer.onXRSessionStarted);
        }
    }

    onXRSessionStarted(session){
        console.log("XR session started.");
        if(tools.renderer.xr_session != null){ // prevent more than one Xr session at once
            tools.renderer.xr_session.end().then(tools.renderer.onXRSessionEnded);
        }

        tools.renderer.xr_session = session;

        //xrButton.textContent = 'Exit VR';

        // Listen for the sessions 'end' event so we can respond if the user
        // or UA ends the session for any reason.
        session.addEventListener('end', tools.renderer.onXRSessionEnded);

        // Create a WebGL context to render with, initialized to be compatible
        // with the XRDisplay we're presenting to.
        //let canvas = document.createElement('canvas');
        //gl = canvas.getContext('webgl2', { xrCompatible: true });

        // Use the new WebGL context to create a XRWebGLLayer and set it as the
        // sessions baseLayer. This allows any content rendered to the layer to
        // be displayed on the XRDevice.
        session.updateRenderState({ baseLayer: new XRWebGLLayer(session, tools.renderer.gl) });

        // Initialize the shaders
        //initShaderProgram(gl, "shader-fs", "shader-vs");

        // Get a reference space, which is required for querying poses. In this
        // case an 'local' reference space means that all poses will be relative
        // to the location where the XRDevice was first detected.
        session.requestReferenceSpace('local').then((refSpace) => {
            tools.renderer.xr_ref_space = refSpace;

            // Inform the session that we're ready to begin drawing.
            session.requestAnimationFrame(tools.renderer.onXRFrame);
        });
    }

      // Called either when the user has explicitly ended the session by calling
      // session.end() or when the UA has ended the session for any reason.
      // At this point the session object is no longer usable and should be
      // discarded.
    onXRSessionEnded(event) {
        tools.renderer.xr_session = null;
        console.log("XR session ended.");
        //requestAnimationFrame(Renderer.onFrame); // turn canvas rendering back on
    }


      // Called every time the XRSession requests that a new frame be drawn.
    onXRFrame(time, frame) {
        tools.xr_frame = frame;
        
        // console.log(time);
        let session = frame.session;

        // Inform the session that we're ready for the next frame.
        if(tools.renderer.xr_session != null){

            if(tools.renderer.xr_session.visibilityState == "hidden"){
                // Kill the session if a user minimizes it (we can always make a new session, 
                // but we don't want old ones piling up oif not properly closed)
                tools.renderer.xr_session.end().then(tools.renderer.onXRSessionEnded);
            }else{
                session.requestAnimationFrame(tools.renderer.onXRFrame);
            }
        }else{
            console.log("No xr sessions, not requesting frame");
        }
        let gl = tools.renderer.gl ;

        // Get the XRDevice pose relative to the reference space we created
        // earlier.
        tools.renderer.head_pose = frame.getViewerPose(tools.renderer.xr_ref_space);

        // Getting the pose may fail if, for example, tracking is lost. So we
        // have to check to make sure that we got a valid pose before attempting
        // to render with it. If not in this case we'll just leave the
        // framebuffer cleared, so tracking loss means the scene will simply
        // disappear.
        if(tools.renderer.head_pose) {
            let glLayer = frame.session.renderState.baseLayer;

            // If we do have a valid pose, bind the WebGL layer's framebuffer,
            // which is where any content to be displayed on the XRDevice must be
            // rendered.
            gl.bindFramebuffer(gl.FRAMEBUFFER, glLayer.framebuffer); // TODO do we need to do this every frame?

            // Clear the framebuffer
            gl.clearColor(tools.renderer.bgColor[0]/255.0, tools.renderer.bgColor[1]/255.0, tools.renderer.bgColor[2]/255.0, 1.0);
            gl.enable(gl.DEPTH_TEST);
            gl.disable(gl.CULL_FACE);
            //gl.cullFace(gl.BACK);
            gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
            
            let frame_id = 0 ;
            for (let view of tools.renderer.head_pose.views) {
                let viewport = glLayer.getViewport(view);
                gl.viewport(viewport.x, viewport.y,
                            viewport.width, viewport.height);

                gl.uniformMatrix4fv(tools.renderer.shaderProgram.p_matrix, false, view.projectionMatrix);

                tools.renderer.mvMatrix = view.transform.inverse.matrix ;
                tools.current_mode.drawFrame(frame_id);

                frame_id++;
            }

            //Send VR controller data to the execution mode
            tools.renderer.captureXRInput(frame.session.inputSources, frame);
            //tools.current_mode.vrInputSourcesUpdated(frame.session.inputSources, frame);
        }
    }

    captureXRInput(input_sources, frame){
        tools.renderer.xr_input = [];
        tools.renderer.has_new_xr_input = false;
        for (let inputSource of input_sources) {
            let this_input = {};
            let ray_pose = frame.getPose(inputSource.targetRaySpace, tools.renderer.xr_ref_space);
            if(ray_pose && inputSource.gripSpace){
                this_input.ray_pose = ray_pose.transform.matrix;
                if(inputSource.gamepad){
                    this_input.buttons=[];
                    for(let button of inputSource.gamepad.buttons){
                        this_input.buttons.push({pressed:button.pressed, touched:button.touched, value:button.value});
                    }
                    
                    this_input.axes = [];
                    for(let axis of inputSource.gamepad.axes){
                        this_input.axes.push(axis);
                    }
                }
                
                
                this_input.grip_pose = frame.getPose(inputSource.gripSpace, tools.renderer.xr_ref_space).transform.matrix;
                this_input.handedness = inputSource.handedness ;
                tools.renderer.xr_input.push(this_input);
                tools.renderer.has_new_xr_input = true;
            }

        }
 
    }

    
}