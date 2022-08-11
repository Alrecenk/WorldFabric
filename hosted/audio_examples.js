<html>

// buffer chunks to be played in sequence
<script>
"use strict";
class SoundBuffer {
    constructor(ctx, sampleRate, bufferSize = 6, debug = true) {
        this.ctx = ctx;
        this.sampleRate = sampleRate;
        this.bufferSize = bufferSize;
        this.debug = debug;
        this.chunks = [];
        this.isPlaying = false;
        this.startTime = 0;
        this.lastChunkOffset = 0;
    }
    createChunk(chunk) {
        var audioBuffer = this.ctx.createBuffer(2, chunk.length, this.sampleRate);
        audioBuffer.getChannelData(0).set(chunk);
        var source = this.ctx.createBufferSource();
        source.buffer = audioBuffer;
        source.connect(this.ctx.destination);
        source.onended = (e) => {
            this.chunks.splice(this.chunks.indexOf(source), 1);
            if (this.chunks.length == 0) {
                this.isPlaying = false;
                this.startTime = 0;
                this.lastChunkOffset = 0;
            }
        };
        return source;
    }
    log(data) {
        if (this.debug) {
            console.log(new Date().toUTCString() + " : " + data);
        }
    }
    addChunk(data) {
        if (this.isPlaying && (this.chunks.length > this.bufferSize)) {
            this.log("chunk discarded");
            return; // throw away
        }
        else if (this.isPlaying && (this.chunks.length <= this.bufferSize)) { // schedule & add right now
            this.log("chunk accepted");
            let chunk = this.createChunk(data);
            chunk.start(this.startTime + this.lastChunkOffset);
            this.lastChunkOffset += chunk.buffer.duration;
            this.chunks.push(chunk);
        }
        else if ((this.chunks.length < (this.bufferSize / 2)) && !this.isPlaying) { // add & don't schedule
            this.log("chunk queued");
            let chunk = this.createChunk(data);
            this.chunks.push(chunk);
        }
        else { // add & schedule entire buffer
            this.log("queued chunks scheduled");
            this.isPlaying = true;
            let chunk = this.createChunk(data);
            this.chunks.push(chunk);
            this.startTime = this.ctx.currentTime;
            this.lastChunkOffset = 0;
            for (let i = 0; i < this.chunks.length; i++) {
                let chunk = this.chunks[i];
                chunk.start(this.startTime + this.lastChunkOffset);
                this.lastChunkOffset += chunk.buffer.duration;
            }
        }
    }
}
</script>


// record streaming chunks from microphone and play back
<script>
var start_time = new Date().getTime();

const recordAudio = () =>
  new Promise(async resolve => {
    const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
    const mediaRecorder = new MediaRecorder(stream);
    const audioChunks = [];

    mediaRecorder.addEventListener("dataavailable", event => {
        if(event.data.size >0){
            console.log("chunk:" + (new Date().getTime() - start_time));
            console.log(event.data);
            const audioUrl = URL.createObjectURL(event.data);
            const audio = new Audio(audioUrl);
            audio.play();
            audioChunks.push(event.data);

        }
    });

    const start = () => mediaRecorder.start();

    const request = () => mediaRecorder.requestData();

    const stop = () =>
      new Promise(resolve => {
        mediaRecorder.addEventListener("stop", () => {
          const audioBlob = new Blob(audioChunks, { type: "audio/mpeg" });
          console.log("blob:");
          console.log(audioBlob);
          const audioUrl = URL.createObjectURL(audioBlob);
          const audio = new Audio(audioUrl);
          console.log("audio:");
          console.log(audio);
          const play = () => audio.play();
          resolve({ audioBlob, audioUrl, play });
        });

        mediaRecorder.stop();
      });

    resolve({ start, stop, request });
  });

const sleep = time => new Promise(resolve => setTimeout(resolve, time));

const handleAction = async () => {
  const recorder = await recordAudio();
  const actionButton = document.getElementById("action");
  actionButton.disabled = true;
  recorder.start();
  start_time = new Date().getTime();
  for(let k=0;k<100;k++){

    await sleep(30);
    recorder.request();
  }
  const audio = await recorder.stop();
  audio.play();
  await sleep(3000);
  actionButton.disabled = false;
};

</script>


// generate sound from a waveform and play it
<script type="text/javascript">
const kSampleRate = 44100; // Other sample rates might not work depending on the your browser's AudioContext
const kNumSamples = 16834;
const kFrequency  = 440;
const kPI_2       = Math.PI * 2;

function play_buffersource() {
    if (!window.AudioContext) {
        if (!window.webkitAudioContext) {
            alert("Your browser sucks because it does NOT support any AudioContext!");
            return;
        }
        window.AudioContext = window.webkitAudioContext;
    }

    var ctx = new AudioContext();

    var buffer = ctx.createBuffer(1, kNumSamples, kSampleRate);
    var buf    = buffer.getChannelData(0);
    for (i = 0; i < kNumSamples; ++i) {
        buf[i] = Math.sin(kFrequency * kPI_2 * i / kSampleRate);
    }

    var node = ctx.createBufferSource(0);
    node.buffer = buffer;
    node.connect(ctx.destination);
    node.noteOn(ctx.currentTime + 0.5);

    node = ctx.createBufferSource(0);
    node.buffer = buffer;
    node.connect(ctx.destination);
    node.noteOn(ctx.currentTime + 2.0);
}

//decode file audio and play it
</script>

<script>
window.onload = init;
var context;    // Audio context
var buf;        // Audio buffer

function init() {
    if (!window.AudioContext) {
    if (!window.webkitAudioContext) {
        alert("Your browser does not support any AudioContext and cannot play back this audio.");
        return;
    }
        window.AudioContext = window.webkitAudioContext;
    }

    context = new AudioContext();
}

function playByteArray(byteArray) {

    var arrayBuffer = new ArrayBuffer(byteArray.length);
    var bufferView = new Uint8Array(arrayBuffer);
    for (i = 0; i < byteArray.length; i++) {
      bufferView[i] = byteArray[i];
    }

    context.decodeAudioData(arrayBuffer, function(buffer) {
        buf = buffer;
        play();
    });
}

// Play the loaded file
function play() {
    // Create a source node from the buffer
    var source = context.createBufferSource();
    source.buffer = buf;
    // Connect to the final output node (the speakers)
    source.connect(context.destination);
    // Play immediately
    source.start(0);
}

</script>
</html>