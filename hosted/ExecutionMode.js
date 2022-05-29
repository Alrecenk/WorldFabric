
/* An abstract class that defines the functions expected to be implemented for interface modes.
    This is pure Javascript, so ths isn't really necessary, but maybe I'll Typescript all this at some point?
*/
class ExecutionMode{

    // Tools is an object with string keys that may include things such as the canvas,
    // API WASM Module, an Interface manager, and/or a mesh manager for shared webGL functionality
    constructor(tools){
        this.tools = tools ;
    }

    // Called when the mode is becoming active (previous mode will have already exited)
    enter(previous_mode){

    }

    // Called when a mode is becoming inactive (prior to calling to enter on that mode)
    exit(next_mode){

    }

    // Called at regular intervals by the main app when the mode is active (30 or 60fps probably but not guarsnteed)
    timer(){

    }

    // Called when the app should be redrawn
    // Note: the elements to draw onto or with should be included in the tools on construction and saved for the duration of the mode
    draw(){

    }

    // Pointers is an array with x,y,button for each pointer currently down
    // button = 10 for touch events
    pointerDown(pointers){

    }

    pointerUp(pointers){

    }

	pointerMove(pointers){

    }

    mouseUpListener(event){

    }

    // These event listeners follow the standard paradigm for event listeners
    mouseWheelListener(event){

    }

	keyDownListener(event){

    }

	keyUpListener(event){

    }
}