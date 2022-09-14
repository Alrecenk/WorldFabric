class Renderer{

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
        gl.enable(gl.CULL_FACE);
        gl.cullFace(gl.BACK);
        console.log(gl.getParameter(gl.VERSION));
        console.log(gl.getParameter(gl.SHADING_LANGUAGE_VERSION));
        console.log(gl.getParameter(gl.VENDOR));
        gl.getExtension('OES_texture_float');
        

        mat4.perspective(this.pMatrix, 45, gl.viewportWidth / gl.viewportHeight, 0.1, 3000.0);
        this.camera_pos = [1,1,1];
        mat4.lookAt(this.mvMatrix, this.camera_pos, [0,0,0], [0,1,0] );

        //console.log(gl);
        this.setLightPosition([this.camera_pos[0], this.camera_pos[1] , this.camera_pos[2]]);

        requestAnimationFrame(Renderer.onFrame); // Timer at 60 hertz.
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
        this.shaderProgram.vertexPositionAttribute = gl.getAttribLocation(this.shaderProgram, "aVertexPosition");
        gl.enableVertexAttribArray(this.shaderProgram.vertexPositionAttribute);
        this.shaderProgram.vertexNormalAttribute = gl.getAttribLocation(this.shaderProgram, "aNormal");
        gl.enableVertexAttribArray(this.shaderProgram.vertexNormalAttribute);
        this.shaderProgram.vertexTexcoordAttribute = gl.getAttribLocation(this.shaderProgram, "aTexcoord");
        gl.enableVertexAttribArray(this.shaderProgram.vertexTexcoordAttribute);
        this.shaderProgram.vertexColorAttribute = gl.getAttribLocation(this.shaderProgram, "aVertexColor");
        gl.enableVertexAttribArray(this.shaderProgram.vertexColorAttribute);
        this.shaderProgram.jointsAttribute = gl.getAttribLocation(this.shaderProgram, "aJoints");
        gl.enableVertexAttribArray(this.shaderProgram.jointsAttribute);
        this.shaderProgram.weightsAttribute = gl.getAttribLocation(this.shaderProgram, "aWeights");
        gl.enableVertexAttribArray(this.shaderProgram.weightsAttribute);
        this.shaderProgram.pMatrixUniform = gl.getUniformLocation(this.shaderProgram, "uPMatrix");
        this.shaderProgram.mvMatrixUniform = gl.getUniformLocation(this.shaderProgram, "uMVMatrix");
        this.shaderProgram.light_point = gl.getUniformLocation(this.shaderProgram, "u_light_point");
        this.shaderProgram.texture = gl.getUniformLocation(this.shaderProgram, "u_texture");
        this.shaderProgram.bones_texture = gl.getUniformLocation(this.shaderProgram, "bones_texture");
        this.shaderProgram.boneless = gl.getUniformLocation(this.shaderProgram, "boneless");
        this.shaderProgram.has_texture = gl.getUniformLocation(this.shaderProgram, "u_has_texture");
        this.shaderProgram.alpha_cutoff = gl.getUniformLocation(this.shaderProgram, "u_alpha_cutoff");
        this.shaderProgram.nearness_cutoff = gl.getUniformLocation(this.shaderProgram, "u_nearness_cutoff");
    }

    static onFrame(){

        var r = tools.renderer ;
        //Update FPS label
        r.frame++;
        if(r.frame >= 30){
            var time = new Date().getTime();
            r.framerate = (r.frame*1000/ (time-r.last_time));
            r.last_time = time;
            r.frame = 0 ;
            if(tools && tools.buttons && tools.buttons["fps_label"]){ // TODO not really the responsibility of the general rendering
                tools.buttons["fps_label"].text = "FPS:" + Math.round(r.framerate);
            }
        }
        
        var context = tools.canvas.getContext("2d");
        context.clearRect(0, 0, context.canvas.width, context.canvas.height);
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
            gl.uniformMatrix4fv(r.shaderProgram.pMatrixUniform, false, r.pMatrix);
            tools.current_mode.drawFrame(0);
            gl.finish();
        }
        if(!r.xr_session){ // Stop drawing regular frames when VR sessions is entered
            requestAnimationFrame(Renderer.onFrame);
        }
    }


    // Returns the point at which the ray through the centero fthe screen intersects the ground
    getCameraFocus(ground_height){
        // Get a ray through the center of the screen
        let ray = this.getRay([gl.viewportWidth*0.5, gl.viewportHeight*0.5]) ;
        //intersect it with the ground
        return Renderer.getGroundIntersect(ray, ground_height) ;
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

        this.setLightPosition([this.camera_pos[0], this.camera_pos[1] , this.camera_pos[2]]);

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

    // Sets light position
    //TODO very shader specific, not a fan, needed it to migrate old code
    setLightPosition(light_point){
        this.gl.uniform3fv(this.shaderProgram.light_point, light_point);
        this.light_point = light_point ;
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
        return Renderer.getPixelRay(this.camera_pos, screen_pos, this.pMatrix, this.mvMatrix, this.gl);
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
            this.buffers[id].color = gl.createBuffer();
            this.buffers[id].normal = gl.createBuffer();
            this.buffers[id].tex_coord = gl.createBuffer();
            this.buffers[id].double_sided = false ;
            this.buffers[id].joints = gl.createBuffer();
            this.buffers[id].weights= gl.createBuffer();
            this.buffers[id].has_texture = false;
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

        if(buffer_data.color){
            gl.bindBuffer(gl.ARRAY_BUFFER, this.buffers[id].color );
            gl.bufferData(gl.ARRAY_BUFFER, buffer_data.color, gl.STATIC_DRAW);
            this.buffers[id].color.itemSize = 4;
            this.buffers[id].color.numItems = num_vertices;
        }

        if(buffer_data.normal){
            gl.bindBuffer(gl.ARRAY_BUFFER, this.buffers[id].normal );
            gl.bufferData(gl.ARRAY_BUFFER, buffer_data.normal, gl.STATIC_DRAW);
            this.buffers[id].normal.itemSize = 3;
            this.buffers[id].normal.numItems = num_vertices;
        }

        if(buffer_data.tex_coord){
            gl.bindBuffer(gl.ARRAY_BUFFER, this.buffers[id].tex_coord );
            gl.bufferData(gl.ARRAY_BUFFER, buffer_data.tex_coord, gl.STATIC_DRAW);
            this.buffers[id].tex_coord.itemSize = 2;
            this.buffers[id].tex_coord.numItems = num_vertices;
        }

        if(buffer_data.weights){
            gl.bindBuffer(gl.ARRAY_BUFFER, this.buffers[id].weights );
            gl.bufferData(gl.ARRAY_BUFFER, buffer_data.weights, gl.STATIC_DRAW);
            this.buffers[id].weights.itemSize = 4;
            this.buffers[id].weights.numItems = num_vertices;
        }

        if(buffer_data.joints){
            
            gl.bindBuffer(gl.ARRAY_BUFFER, this.buffers[id].joints );
            gl.bufferData(gl.ARRAY_BUFFER, buffer_data.joints, gl.STATIC_DRAW);
            this.buffers[id].joints.itemSize = 4;
            this.buffers[id].joints.numItems = num_vertices;
        }

        if(buffer_data.material){
            //console.log("Javascript preparebuffer got material:\n");
            let mat = buffer_data.material ;
            //console.log(mat);
            this.buffers[id].double_sided = (mat.double_sided == 1) ;
            if(mat.has_texture == 1){
                this.buffers[id].texture = gl.createTexture();
                gl.bindTexture(gl.TEXTURE_2D, this.buffers[id].texture);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

                if(mat.image_channels == 3){
                    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, 
                        mat.image_width, mat.image_height, 
                        0, gl.RGB, gl.UNSIGNED_BYTE, mat.image_data);
                }else if(mat.image_channels == 4){
                    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 
                        mat.image_width, mat.image_height, 
                        0, gl.RGBA, gl.UNSIGNED_BYTE, mat.image_data);
                }else{
                    console.log("Failed to load texture because of number of channels (" + mat.image_channels +")");
                }

                this.buffers[id].has_texture = true;
                gl.activeTexture(gl.TEXTURE0 + 2);
                gl.bindTexture(gl.TEXTURE_2D, this.buffers[id].texture );
                gl.uniform1i(this.shaderProgram.texture, 2 );
                
            }
        }

        this.buffers[id].bones = buffer_data.bones ;
        this.buffers[id].boneless = buffer_data.boneless
        this.buffers[id].ready = true;
    }

    drawModel(buffer, bones = null){
        //console.log(buffer);
        let gl = this.gl ;
        if(buffer.ready){
            let position_buffer = buffer.position;
            let color_buffer = buffer.color;
            let normal_buffer = buffer.normal;
            let tex_coord_buffer = buffer.tex_coord;
            let joints_buffer = buffer.joints;
            let weights_buffer = buffer.weights;
            gl.bindBuffer(gl.ARRAY_BUFFER, position_buffer);
            gl.vertexAttribPointer(this.shaderProgram.vertexPositionAttribute, position_buffer.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, normal_buffer);
            gl.vertexAttribPointer(this.shaderProgram.vertexNormalAttribute, normal_buffer.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, tex_coord_buffer);
            gl.vertexAttribPointer(this.shaderProgram.vertexTexcoordAttribute, tex_coord_buffer.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, color_buffer);
            gl.vertexAttribPointer(this.shaderProgram.vertexColorAttribute, color_buffer.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, joints_buffer);
            gl.vertexAttribPointer(this.shaderProgram.jointsAttribute, joints_buffer.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, weights_buffer);
            gl.vertexAttribPointer(this.shaderProgram.weightsAttribute, weights_buffer.itemSize, gl.FLOAT, false, 0, 0);

            if(!bones){
                bones = buffer.bones ;
            }
            if(bones){
                gl.uniform1i(this.shaderProgram.boneless, buffer.boneless);
                if(buffer.boneless != 1){
                    if(!this.bone_tex){
                        this.bone_tex = gl.createTexture();
                    }
                    gl.activeTexture(gl.TEXTURE0 + 1);
                    gl.bindTexture(gl.TEXTURE_2D, this.bone_tex);
                    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
                    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
                    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
                    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
                    
                    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA32F, 32, 32, 0, gl.RGBA, gl.FLOAT, bones);
                    gl.uniform1i(this.shaderProgram.bones_texture, 1);
                }
                
            }

            if(buffer.has_texture){
                gl.activeTexture(gl.TEXTURE0 + 2 );
                gl.bindTexture(gl.TEXTURE_2D, buffer.texture );
                gl.uniform1i(this.shaderProgram.texture, 2 );

                gl.uniform1i(this.shaderProgram.has_texture, 1 );
            }else{
                gl.uniform1i(this.shaderProgram.has_texture, 0 );
            }

            gl.uniform1f(this.shaderProgram.alpha_cutoff, 0.5 );
            gl.uniform1f(this.shaderProgram.nearness_cutoff, 0.2*0.2 );

            if(buffer.double_sided){
                gl.disable(gl.CULL_FACE);
            }else{
                gl.enable(gl.CULL_FACE);
            }

            gl.drawArrays(gl.TRIANGLES, 0, position_buffer.numItems);
        }
    }

    drawMesh(mesh_name, transform = null, bones = null){
        if(!transform){
            transform = mat4.create();
            mat4.identity(transform);
        }
        let M = mat4.create();
        mat4.multiply(M,this.mvMatrix, transform);
        this.gl.uniformMatrix4fv(this.shaderProgram.mvMatrixUniform, false, M);
        if(mesh_name in this.buffer_lookup){ // cache mesh_name to material buffers mapping
            for(let buffer_name of this.buffer_lookup[mesh_name]){
                this.drawModel(this.buffers[buffer_name],bones);
            }
        }else{
            this.buffer_lookup[mesh_name] = [];
            for(let buffer_name in this.buffers){
                //console.log(buffer_name);
                //console.log(buffer_name.substring(0,mesh_name.length));
                if(buffer_name.substring(0,mesh_name.length+3) == mesh_name+"-m="){
                    this.drawModel(this.buffers[buffer_name],bones);
                    this.buffer_lookup[mesh_name].push(buffer_name);
                }
            }
        }
    }

    hasMesh(mesh_name){
        for(let buffer_name in this.buffers){
            //console.log(buffer_name);
            //console.log(buffer_name.substring(0,mesh_name.length));
            if(buffer_name.substring(0,mesh_name.length+3) == mesh_name+"-m="){
                return true ;
            }
        }
        return false;
    }

    removeMesh(mesh_name){
        let to_delete = [];
        for(let buffer_name in this.buffers){
            if(buffer_name.substring(0,mesh_name.length) == mesh_name){
                to_delete.push(buffer_name);
            }
        }
        for(let d of to_delete){
            delete this.buffers[d] ;
        }
        delete this.buffer_lookup[mesh_name] ;
    }

    setMeshDoubleSided(mesh_name, double_sided){
        if(mesh_name in this.buffer_lookup){ // cache mesh_name to material buffers mapping
            for(let buffer_name of this.buffer_lookup[mesh_name]){
                this.buffers[buffer_name].double_sided = double_sided ;
            }
        }else{
            for(let buffer_name in this.buffers){
                if(buffer_name.substring(0,mesh_name.length) == mesh_name){
                    this.buffers[buffer_name].double_sided = double_sided ;
                }
            }
        }
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
            gl.enable(gl.CULL_FACE);
            gl.cullFace(gl.BACK);
            gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
            
            let frame_id = 0 ;
            for (let view of tools.renderer.head_pose.views) {
                let viewport = glLayer.getViewport(view);
                gl.viewport(viewport.x, viewport.y,
                            viewport.width, viewport.height);

                gl.uniformMatrix4fv(tools.renderer.shaderProgram.pMatrixUniform, false, view.projectionMatrix);

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