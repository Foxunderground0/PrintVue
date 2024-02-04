
canvas = document.createElement("canvas");
ctx = canvas.getContext("2d");

class Video {
  constructor(root) {
    this.root = root;
    this.isLoaded = false;
    this.hasBuffered = false;
    this.isBuffering = false;
    this.OnFrameBuffered = null;
    this.OnFrameLoaded = null;
    this.currentIndex = 0;
    this.isPlaying = false;
    this.LoadingTimeout = null;
    this.OnVideoStopped = null;
    this.OnVideoLoading = null;
    this.OnVideoPlaying = null;
    this.OnVideoPaused = null;
    this.period = 200;
    this.waitTimeOut = 1000;
    this.ForcedFrame = null;
    this.loop = false;    
  }
  // Function to convert data URL to RGBA frame data
  dataURLtoFrame(dataURL) {
    const img = new Image();
    img.src = dataURL;

    return new Promise((resolve) => {
      img.onload = function () {
        console.log(canvas);
        canvas.width = img.width;
        canvas.height = img.height;
        ctx.drawImage(img, 0, 0);

        // Get RGBA frame data
        //const imageData = ctx.getImageData(0, 0, img.width, img.height).data;

        // Convert to Whammy.js compatible format (array of Uint8Arrays)
        //const whammyFrame = new Uint8Array(imageData.buffer);
        resolve(canvas);
      };
    });
  }
  GetImageData(dataURL) {
    return new Promise((resolve, reject) => {
      const img = new Image();
      img.src = dataURL;
      img.onload = () => {
        const canvas = document.createElement("canvas");
        canvas.width = img.width;
        canvas.height = img.height;
        const ctx = canvas.getContext("2d");
        ctx.drawImage(img, 0, 0);
        const imageData = ctx.getImageData(0, 0, img.width, img.height);

        // Convert to Whammy.js compatible format (array of Uint8Arrays)
        //  const whammyFrame = new Uint8Array(imageData.buffer);
        resolve(canvas);
        //resolve(imageData);
        //resolve(img);
      };
      img.src = dataURL;
    });
  }
  downloadURI(uri, name) {
    var link = document.createElement("a");
    link.download = name;
    link.href = uri;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    //delete link;
  }
  async Download() {
    var encoder = new Whammy.Video(30, 1);
    for (const f in this.frames) {
      //console.log("f: ", this.frames[f].imageData);
      var i = await this.dataURLtoFrame(this.frames[f].imageData);
      //console.log("add: ", i);
      encoder.add(i);
    }
    encoder.compile(false, (blob) => {
      console.log("Callback called:", blob);

      // Create a download link
      const downloadLink = document.createElement("a");
      downloadLink.href = URL.createObjectURL(blob);
      downloadLink.download = "output-video.webm";

      document.getElementById("vPlayer").src = URL.createObjectURL(blob);
      // Trigger the download
      downloadLink.click();

      // Clean up
      URL.revokeObjectURL(downloadLink.href);
    });

    console.log("The end");
  }
  async Download2() {
    var frames = [];
    for (var f in this.frames) {
      frames.push(await this.GetImageData(this.frames[f].imageData));
    }

    // Create an in-memory canvas
    const canvas = document.createElement("canvas");
    canvas.width = frames[0].width;
    canvas.height = frames[0].height;
    const ctx = canvas.getContext("2d");

    // if you want to manually trigger the frames, that can be useful if you struggle to create a real-time animation:
    var videoStream = canvas.captureStream(30);

    var mediaRecorder = new MediaRecorder(videoStream);

    var chunks = [];
    mediaRecorder.ondataavailable = function (e) {
      console.log("pushing chunk:", e.data);
      chunks.push(e.data);
    };

    mediaRecorder.onstop = function (e) {
      console.log("Stopped");
      var blob = new Blob(chunks, { type: "video/mp4" }); // other types are available such as 'video/webm' for instance, see the doc for more info
      chunks = [];
      var videoURL = URL.createObjectURL(blob);

      document.getElementById("vPlayer").src = URL.createObjectURL(blob);
      console.log("Video data url ready", videoURL);

      function downloadURI(uri, name) {
        var link = document.createElement("a");
        link.download = name;
        link.href = uri;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        //delete link;
      }

      downloadURI(videoURL, "MyVideo.mp4");
    };

    mediaRecorder.start();

    function pushFrame(index) {
      console.log("Push", index);
      console.log("Isolation", videoStream.getVideoTracks()[0].readyState);
      if (index >= frames.length) {
        console.log("Stopping recorder");
        mediaRecorder.stop();
        return;
      }
      ctx.drawImage(frames[index], 0, 0);
      videoStream.getVideoTracks()[0].requestFrame();
      setTimeout(() => {
        pushFrame(index + 1);
      }, 100);
    }
    pushFrame(0);
    console.log("The end");
  }

  //Play Logic
  GetOrFetchFrame(index) {
    return new Promise((resolve, reject) => {
      if (this.frames[index].imageData) {
        //console.log("Returning cached frame: ", index);
        resolve(this.frames[index]);
      } else {
        // all we can do is wait for now.
        console.log("waiting on:", index);
        if (this.LoadingTimeout) clearTimeout(this.LoadingTimeout);
        this.LoadingTimeout = setTimeout(() => {
          this.LoadingTimeout = null;
          if (this.OnVideoLoading) this.OnVideoLoading();
        }, this.waitTimeOut);
        this.frames[index].OnDataLoaded = () => {
          if (this.LoadingTimeout) clearTimeout(this.LoadingTimeout);
          this.LoadingTimeout = null;
          console.log("Wait ended on:", index);
          resolve(this.frames[index]);
        };
      }
    });
  }
  LoadThumbnail() {
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        this.LoadMeta().then(() => {
          fetch(this.frames[0].file).then(async (resp) => {
            if (!resp.ok) {
              throw new Error("Network response was not ok");
            }
            const blob = await resp.blob();
            const objectURL = URL.createObjectURL(blob);

            // cache the data for future use
            this.frames[0].imageData = objectURL;
            this.frames[0].blob = blob;
            console.log("Cached thumb");
            resolve(this.frames[0].imageData);
          });
        });
      }, 0); // simulate ethumbnail download
    });
  }
  Seek(fraction) {
    var index = Math.round(fraction * (this.frames.length - 1));
    if (index < 0) index = 0;
    if (index >= this.frames.length) index = this.frames.length - 1;
    console.log("Seek", index);
    // seek now
    // we need to stop on
    if (this.isPlaying) {
      this.ForcedFrame = index;
    } else
      this.GetOrFetchFrame(index).then((frame) => {
        this.currentIndex = index;
        this.OnFrameLoaded(frame, index / (this.frames.length - 1));
      });
  }
  SendFrame(index) {
    //console.log("SendFrame:", index);
    this.GetOrFetchFrame(index).then((frame) => {
      if (!this.isPlaying) {
        // playback was stopped
        return;
      }
      if (this.OnFrameLoaded) {
        this.currentIndex = index;
        this.OnFrameLoaded(frame, index / (this.frames.length - 1));
      }
      setTimeout(() => {
        if (index + 1 >= this.frames.length) {
          // video has ended
          if (this.loop) this.SendFrame(0);
          else {
            if (this.OnVideoStopped) this.OnVideoStopped();
            this.currentIndex = 0; // for the next time
            this.isPlaying = false;
          }
        } else {
          if (this.ForcedFrame) {
            // seek operation was requested
            index = this.ForcedFrame - 1;
            this.ForcedFrame = null;
          }

          this.SendFrame(index + 1);
        }
      }, this.period);
    });
  }
  Pause() {
    if (!this.isPlaying) return;
    this.isPlaying = false;
    if (this.OnVideoPaused) this.OnVideoPaused();
  }
  Play() {
    console.log("Play while:", this.isPlaying);
    if (this.isPlaying) return;
    this.isPlaying = true;
    if (this.OnVideoPlaying) this.OnVideoPlaying();
    this.LoadMeta().then(() => {
      this.BeginBuffering();
      this.SendFrame(this.currentIndex);
    });
  }
  // Buffer Logic
  downloadFrame(index) {
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        fetch(this.frames[index].file).then(async (resp) => {
          if (!resp.ok) {
            throw new Error("Network response was not ok");
          }
          const blob = await resp.blob();
          const objectURL = URL.createObjectURL(blob);

          // cache the data for future use
          this.frames[index].imageData = objectURL;
          this.frames[index].blob = blob;
          //console.log("Cached:", index);
          if (this.frames[index].OnDataLoaded)
            this.frames[index].OnDataLoaded();
          resolve();
        });
      }, 0); // simulate a delay
    });
  }
  bufferFrame(index) {
    if (!this.isBuffering) return;
    if (index >= this.frames.length) {
      this.hasBuffered = true;
      return;
    }
    if (this.frames[index].imageData) {
      this.bufferFrame(index + 1);
      return;
    }
    this.downloadFrame(index).then(() => {
      this.OnFrameBuffered(index / (this.frames.length - 1));
      this.bufferFrame(index + 1);
    });
  }
  BeginBuffering() {
    if (this.isBuffering) {
      return;
    }
    this.isBuffering = true;
    this.LoadMeta().then(() => {
      this.bufferFrame(0);
    });
  }
  PauseBuffering() {
    this.isBuffering = false;
  }
  LoadMeta() {
    return new Promise((resolve, reject) => {
      if (this.isLoaded) {
        console.log("Already laoded");
        resolve();
        return;
      }
      fetch(this.root + "seq_info.json").then(async (res) => {
        if (!res.ok) reject();
        var meta_info = await res.json();
        this.meta = meta_info;
        //console.log("Fetched meta:", meta_info);
        this.frames = [];
        for (
          var i = meta_info.startFrameIndex;
          i < meta_info.endFrameIndex;
          i++
        ) {
          this.frames.push({
            file: this.root + meta_info.seed + i + meta_info.extension,
            imageData: null,
            index: i,
            OnDataLoaded: null,
          });
        }
        //console.log("Init Frames:", this.frames);
        this.isLoaded = true;
        resolve();
      });
    });
  }
}
