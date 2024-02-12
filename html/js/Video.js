canvas = document.createElement("canvas");
ctx = canvas.getContext("2d");

class Video {
  constructor(root) {
    this.root = root;
    this.isLoaded = false;
    this.hasBuffered = false;
    this.isBuffering = false;
    this.OnFrameBuffered = null;
    this.OnFrameEncoded = null;
    this.OnVideoEncoded = null;
    this.Video = null;
  }
  VideoReady(src) {
    if (this.OnVideoEncoded) this.OnVideoEncoded(src);
  }
  // Function to convert data URL to RGBA frame data
  dataURLtoFrame(dataURL) {
    const img = new Image();
    img.src = dataURL;

    return new Promise((resolve) => {
      img.onload = function () {
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

  async CreateVideo() {
    var video = this;
    // Create a capturer that exports a WebM video
    var capturer = new CCapture({
      format: "webm",
      framerate: 30,
      //verbose: true,
    });
    capturer.start();
    for (const f in this.frames) {
      //console.log("f: ", this.frames[f].imageData);
      var i = await this.dataURLtoFrame(this.frames[f].imageData);
      //console.log("add: ", i);
      capturer.capture(i);
      if(this.OnFrameEncoded)
        this.OnFrameEncoded(f / (video.frames.length - 1));
    }
    capturer.stop();

    // default save, will download automatically a file called {name}.extension (webm/gif/tar)
    //capturer.save();

    // custom save, will get a blob in the callback
    capturer.save(function (blob) {
      console.log("Callback called:", blob);
      video.Video = URL.createObjectURL(blob);
      video.VideoReady(video.Video);
    });
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
  Load() {
    if (this.isBuffering)
        return; // OnVideoEncoded will be called later
    if (this.hasBuffered) {
      if (this.OnVideoEncoded) this.OnVideoEncoded(this.Video);
    } else {
      this.LoadMeta().then(() => {
        this.BeginBuffering();
      });
    }
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
      this.isBuffering = false;
      this.CreateVideo();
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
        console.log("Fetched meta:", meta_info);
        this.frames = [];
        for (
          var i = meta_info.startFrameIndex;
          i <= meta_info.endFrameIndex;
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
