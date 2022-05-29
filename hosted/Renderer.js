class Renderer{

    gl; // WebGl instance
    shaderProgram; 

    camera_pos;
    mvMatrix = mat4.create(); // Model view matrix
    pMatrix = mat4.create();  // Perspective matrix

    meshes_per_batch = 200; // Number of meshes to put in each openGL buffer
    buffers = {} ; // current active mesh buffers maps id to object with position,color, and normal
    buffer_batches = []; // array of maps from index to mesh buffer
    changed_buffer = {}; // which batched buffers have changed sincde the last frame

    start_camera ; // A clone of the camera mvMatrix made when a camera operation starts
    action_focus = [0,0,0]; // the focal point of the current camera operation
    start_pointer ; // Mouse or touch pointers saved whena camera operation starts

    maxPanSpeed = 5;
    xOrbitSpeed=-0.004;
    yOrbitSpeed=0.007;
    rotate_speed=0.01;
    
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
        this.gl.clearColor(0.0, 0.0, 0.0, 1.0);
        this.gl.enable(this.gl.DEPTH_TEST);
        this.gl.enable(this.gl.CULL_FACE);
        this.gl.cullFace(this.gl.BACK);
        console.log(this.gl.getParameter(this.gl.VERSION));
        console.log(this.gl.getParameter(this.gl.SHADING_LANGUAGE_VERSION));
        console.log(this.gl.getParameter(this.gl.VENDOR));
        this.gl.getExtension('OES_texture_float');
        this.setLightPosition([0,0,250]);

        mat4.perspective(this.pMatrix, 45, this.gl.viewportWidth / this.gl.viewportHeight, 0.1, 3000.0);
        this.camera_pos = [20,20,20];
        mat4.lookAt(this.mvMatrix, this.camera_pos, [0,0,0], [0,1,0] );
    }

    // Initialize webGL on a canvas
    initGL(canvas){
        try {
            this.gl = canvas.getContext("webgl2");
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
    //TODO this is matched to a specific simple sahder, should be generalized
    initShaderProgram(fragment_shader_id, vertex_shader_id) {
        var fragmentShader = this.getShader(this.gl, fragment_shader_id);
        var vertexShader = this.getShader(this.gl, vertex_shader_id);

        this.shaderProgram = this.gl.createProgram();
        this.gl.attachShader(this.shaderProgram, vertexShader);
        this.gl.attachShader(this.shaderProgram, fragmentShader);
        this.gl.linkProgram(this.shaderProgram);

        if (!this.gl.getProgramParameter(this.shaderProgram, this.gl.LINK_STATUS)) {
            console.error("Could not initialize shaders!");
        }

        this.gl.useProgram(this.shaderProgram);
        this.shaderProgram.vertexPositionAttribute = this.gl.getAttribLocation(this.shaderProgram, "aVertexPosition");
        this.gl.enableVertexAttribArray(this.shaderProgram.vertexPositionAttribute);
        this.shaderProgram.vertexNormalAttribute = this.gl.getAttribLocation(this.shaderProgram, "aNormal");
        this.gl.enableVertexAttribArray(this.shaderProgram.vertexNormalAttribute);
        this.shaderProgram.vertexColorAttribute = this.gl.getAttribLocation(this.shaderProgram, "aVertexColor");
        this.gl.enableVertexAttribArray(this.shaderProgram.vertexColorAttribute);
        this.shaderProgram.pMatrixUniform = this.gl.getUniformLocation(this.shaderProgram, "uPMatrix");
        this.shaderProgram.mvMatrixUniform = this.gl.getUniformLocation(this.shaderProgram, "uMVMatrix");
        this.shaderProgram.light_point = this.gl.getUniformLocation(this.shaderProgram, "light_point");
    }

    // Push current matrices to the shader
    setMatrixUniforms() {
        this.gl.uniformMatrix4fv(this.shaderProgram.pMatrixUniform, false, this.pMatrix);
        this.gl.uniformMatrix4fv(this.shaderProgram.mvMatrixUniform, false, this.mvMatrix);
    }

    // Clear the viewport
    clearViewport(){
        this.gl.viewport(0, 0, this.gl.viewportWidth, this.gl.viewportHeight);
        this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);
    }

    // Returns the point at which the ray through the centero fthe screen intersects the ground
    getCameraFocus(ground_height){
        // Get a ray through the center of the screen
        let ray = this.getRay([this.gl.viewportWidth*0.5, this.gl.viewportHeight*0.5]) ;
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
    }

    moveCamera(move){
        mat4.translate(this.mvMatrix, this.mvMatrix, [-1*move[0], -1*move[1], -1*move[2]]);
        this.camera_pos[0] += move[0];
        this.camera_pos[1] += move[1];
        this.camera_pos[2] += move[2];
    }

    setZoom(zoom, ground_height){
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
    }

    drawMeshes(){
        this.setMatrixUniforms();
        for(let id in this.buffers){
            if(this.changed_buffer[id]){
                this.prepareBatchBuffer(id);
                this.changed_buffer[id] = false;
            }
            this.drawModel(this.buffers[id]);
        }
    }

    finishFrame(){
        this.gl.finish();
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
            return [(np[0]+1) * this.gl.viewportWidth * 0.5, (-np[1]+1) * this.gl.viewportHeight * 0.5];
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

    // Binds a webGl buffer to the buffer data provided and puts it in buffers[id]
    //TODO make more general for different shaders
    prepareBuffer(id, buffer_data){
        if(!(id in this.buffers)){ // New buffer
            this.buffers[id] = {};
            this.buffers[id].position = this.gl.createBuffer();
            this.buffers[id].color = this.gl.createBuffer();
            this.buffers[id].normal = this.gl.createBuffer();
        }
        let num_vertices = buffer_data.vertices ;
        if(num_vertices == 0){
            this.buffers[id].ready = false;
            return ;
        }
        if(buffer_data.position){
            this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffers[id].position );
            this.gl.bufferData(this.gl.ARRAY_BUFFER, buffer_data.position, this.gl.STATIC_DRAW);
            this.buffers[id].position.itemSize = 3;
            this.buffers[id].position.numItems = num_vertices;
        }

        if(buffer_data.color){
            this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffers[id].color );
            this.gl.bufferData(this.gl.ARRAY_BUFFER, buffer_data.color, this.gl.STATIC_DRAW);
            this.buffers[id].color.itemSize = 3;
            this.buffers[id].color.numItems = num_vertices;
        }

        if(buffer_data.normal){
            this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffers[id].normal );
            this.gl.bufferData(this.gl.ARRAY_BUFFER, buffer_data.normal, this.gl.STATIC_DRAW);
            this.buffers[id].normal.itemSize = 3;
            this.buffers[id].normal.numItems = num_vertices;
        }

        this.buffers[id].ready = true;
    }

    drawModel(buffer){
        if(buffer.ready){
            let position_buffer = buffer.position;
            let color_buffer = buffer.color;
            let normal_buffer = buffer.normal;
            this.gl.bindBuffer(this.gl.ARRAY_BUFFER, position_buffer);
            this.gl.vertexAttribPointer(this.shaderProgram.vertexPositionAttribute, position_buffer.itemSize, this.gl.FLOAT, false, 0, 0);
            this.gl.bindBuffer(this.gl.ARRAY_BUFFER, normal_buffer);
            this.gl.vertexAttribPointer(this.shaderProgram.vertexNormalAttribute, normal_buffer.itemSize, this.gl.FLOAT, false, 0, 0);
            this.gl.bindBuffer(this.gl.ARRAY_BUFFER, color_buffer);
            this.gl.vertexAttribPointer(this.shaderProgram.vertexColorAttribute, color_buffer.itemSize, this.gl.FLOAT, false, 0, 0);
            this.gl.drawArrays(this.gl.TRIANGLES, 0, position_buffer.numItems);
        }
    }

}